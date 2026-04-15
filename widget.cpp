/**
 * @file widget.cpp
 * @brief 工业视觉识别系统主窗口实现文件
 * @details 实现图像采集、OCR识别、模板匹配、PLC通信等核心功能
 * @author 优化版本
 * @date 2024
 */

#include "widget.h"
#include "ui_widget.h"
#include "SerialPort.h"
#include "waitting.h"
#include "databasesetting.h"
#include "enlarge.h"
#include "choosebarcodedialog.h"
#include "snap7.h"


// Qt核心组件
#include <QTimer>
#include <QThread>
#include <QFileDialog>
#include <QImageReader>
#include <QLabel>
#include <QPainter>
#include <QLineEdit>
#include <QMetaType>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QDebug>
#include <QFile>
#include <QString>
#include <QPixmap>
#include <QMessageBox>
#include <QDesktopServices>
#include <QDateTime>
#include <QApplication>
#include <QTranslator>
#include <QIcon>
#include <QCamera>
#include <QCameraInfo>
#include <QDesktopWidget>
#include <QSettings>
#include <QSplashScreen>
#include <QTextCodec>
#include <QElapsedTimer>
#include <QDir>
#include <QInputDialog>

// Qt串口和SQL
#include <QtSerialPort/QtSerialPort>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QVariantList>
#include <QtSql/QSqlDatabase>

// 标准库
#include <windows.h>
#include <iostream>
#include <memory>
#include <queue>
#include <utility>

#pragma execution_character_set("utf-8")
using namespace std;

// OpenCV全局变量
cv::Point pt1, pt2;

struct CVDrawResult {
    cv::Rect rect;
    double score;
};
static std::vector<CVDrawResult> g_lastDrawResults;
static cv::Rect g_lastRoi;
static qint64 g_lastDetectTime = 0;

/**
 * @brief Widget构造函数
 * @param parent 父窗口指针
 * @details 初始化UI、相机、OCR模型、定时器等核心组件
 */
Widget::Widget(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::Widget),
      timer(new QTimer(this)),
      timer1(new QTimer(this)),
      imageIndex(0),
      color(1),
      templatematch(nullptr),
      tracking(true),
      zhuizong(nullptr),
      first(false),
      savefirst(false),
      imageLabel(nullptr)
{
    ui->setupUi(this);

    initStyle();

    // UI 文件中已经是 ImageLabel，直接使用
    imageLabel=ui->image_undetected;

    // 注册Qt元类型，用于跨线程信号传递
    qRegisterMetaType<cv::Mat>("cv::Mat");
    qRegisterMetaType<cv::Mat *>("cv::Mat*");
    qRegisterMetaType<cv::Rect2d>("cv::Rect2d");
    qRegisterMetaType<QString>("QString");

    // 初始化追踪对象（使用智能指针）
    unique_ptr<Zhuizong> zhuizong = make_unique<Zhuizong>();

    // 初始化PLC客户端
    client = new TS7Client;

    // 初始化窗口组件
    initWidget();

    // 加载OCR配置文件
    config = new OCRConfig("config1.txt");
    config->PrintConfigInfo();

    // 初始化检测器（DBNet模型）
    det = new DBDetector(config->det_model_dir, config->use_gpu, config->gpu_id,
                         config->gpu_mem, config->cpu_math_library_num_threads,
                         config->use_mkldnn, config->max_side_len, config->det_db_thresh,
                         config->det_db_box_thresh, config->det_db_unclip_ratio,
                         config->visualize, config->use_tensorrt, config->use_fp16);

    // 初始化分类器（角度分类）
    if (config->use_angle_cls == true)
    {
        cls = new Classifier(config->cls_model_dir, config->use_gpu, config->gpu_id,
                             config->gpu_mem, config->cpu_math_library_num_threads,
                             config->use_mkldnn, config->cls_thresh,
                             config->use_tensorrt, config->use_fp16);
    }

    // 初始化识别器（CRNN模型）
    rec = new CRNNRecognizer(config->rec_model_dir, config->use_gpu, config->gpu_id,
                             config->gpu_mem, config->cpu_math_library_num_threads,
                             config->use_mkldnn, config->char_list_file,
                             config->use_tensorrt, config->use_fp16);

    // 初始化统计变量
    hasValidBoxes = false;
    savedDetectionBox = cv::Rect2d(0, 0, 0, 0);
    savedTrackingBox = cv::Rect2d(0, 0, 0, 0);
    recognitionCompletedFlag = false;
    isCollecting = false;
    totalImages = 0;
    ngImages = 0;
    allResults = "";
    wrongindex = ui->lineEdit_12->text().toInt();

    // 设置文本框自动换行
    ui->dateEdit->setWordWrapMode(QTextOption::WordWrap);

    // 连接定时器信号
    connect(timer, &QTimer::timeout, this, &Widget::rightremove);

    // 设置默认值并加载保存的设置
    setupDefaultValues();
    loadSettings();
    loadLastTemplateConfig(); // 加载模板图像
}

/**
 * @brief Widget析构函数
 * @details 清理所有资源，关闭相机、停止线程、删除临时文件
 */
Widget::~Widget()
{
    qDebug() << "Widget destructor called";

    // 先关闭所有窗口
    try {
        cv::destroyAllWindows();
    } catch (...) {}

    delete ui;
    delete myImage;

    // 关闭相机
    if (m_pcMyCamera)
    {
        m_pcMyCamera->Close();
        delete m_pcMyCamera;
        m_pcMyCamera = NULL;
    }

    // 停止 myThread（不使用terminate）
    if (myThread) {
        if (myThread->isRunning()) {
            myThread->requestStop();
            myThread->stop();
            if (!myThread->wait(2000)) {
                qDebug() << "WARNING: myThread did not stop in destructor";
                // 不调用 terminate，让它自然结束
            }
        }
        delete myThread;
    }

    // 停止 cameraThread（不使用terminate）
    if (cameraThread) {
        if (cameraThread->isRunning()) {
            cameraThread->requestStop();
            if (!cameraThread->wait(2000)) {
                qDebug() << "WARNING: cameraThread did not stop in destructor";
                // 不调用 terminate
            }
        }
        delete cameraThread;
    }

    delete templatematch;

    QString filePath = "muban.png";
    QFile file(filePath);
    if (file.exists())
    {
        file.remove();
    }

    qDebug() << "Widget destroyed";
}
/**
 * @brief 初始化Widget组件
 * @details 创建图像保存文件夹、初始化图像对象、创建工作线程、连接信号槽
 */
void Widget::initWidget()
{
    // 初始化设备打开标志
    m_bOpenDevice = false;

    // 创建图像保存文件夹
    QString imagePath = QDir::currentPath() + "/myImage/";
    QDir dstDir(imagePath);
    if (!dstDir.exists())
    {
        if (!dstDir.mkdir(imagePath))
        {
            qDebug() << "创建Image文件夹失败！";
        }
    }

    // 初始化图像指针
    myImage = new Mat();

    // 创建工作线程
    myThread = new MyThread();

    // 创建模板匹配对象
    templatematch = new TemplateMatch();

    // 连接线程信号槽 - 图像显示
    connect(myThread, &MyThread::signal_messImage, this, [this](cv::Mat img) {
        this->slot_displayAndDetect(&img);
    }, Qt::QueuedConnection);

    // 连接线程信号槽 - 图像检测（根据检测模式选择不同的处理函数）
    QObject::connect(myThread, &MyThread::signal_sendForDetection, this, [this](cv::Mat img, Rect2d rect) {
        if (ui->comboBox_4->currentIndex() == 2) {
            this->slot_readAndDetect(&img, rect);
        } else if(ui->comboBox_4->currentIndex() == 0) {
            this->slot_readAndDetect3(&img, rect);
        } else if(ui->comboBox_4->currentIndex() == 1){
            this->slot_readAndDetect4(&img, rect);
        }
    });


    // 连接其他信号槽
    connect(myThread, SIGNAL(signal_cleanlabel()), this, SLOT(slot_clearResultLabel()));
    connect(this, &Widget::rotate, myThread, &MyThread::receiveangle);
    connect(this, &Widget::choosechannel,myThread,&MyThread::receivecolorchannel1);
    connect(this, &Widget::sendDataTo, myThread, &MyThread::received);
    connect(this, &Widget::imgshibie, templatematch, &TemplateMatch::receshibie);
    connect(this, &Widget::caijianchicun, templatematch, &TemplateMatch::caijiansize);
    connect(this, &Widget::kernal, templatematch, &TemplateMatch::kernel);
    connect(this, &Widget::ssim, templatematch, &TemplateMatch::ssimvalue);


    //开机直接连接PLC
    QByteArray ad(ui->lineEdit->text().toUtf8());
    Address = ad.data();

    int tmp = client->ConnectTo(Address, 0, 1);

    if (tmp != 0)
    {
        QMessageBox::critical(this, "error", "PLC连接失败");
    }


}



/**
 * @brief QImage转换为cv::Mat
 * @param image 输入的QImage对象
 * @return cv::Mat 转换后的OpenCV Mat对象
 */
cv::Mat QImage2cvMat(QImage image)
{
    cv::Mat mat;
    switch (image.format())
    {
    case QImage::Format_ARGB32:
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32_Premultiplied:
    {
        cv::Mat mat_temp = cv::Mat(image.height(), image.width(), CV_8UC4,
                                   (void *)image.constBits(), image.bytesPerLine());
        cvtColor(mat_temp, mat, cv::COLOR_BGRA2BGR);
        break;
    }
    case QImage::Format_RGB888:
        mat = cv::Mat(image.height(), image.width(), CV_8UC3,
                      (void *)image.constBits(), image.bytesPerLine());
        break;
    case QImage::Format_Indexed8:
        mat = cv::Mat(image.height(), image.width(), CV_8UC1,
                      (void *)image.constBits(), image.bytesPerLine());
        break;
    }
    return mat;
}

/**
 * @brief QString转换为std::string
 * @param qstr 输入的QString
 * @return std::string 转换后的标准字符串
 */
string Widget::qstr2str(const QString qstr)
{
    QByteArray cdata = qstr.toLocal8Bit();
    return std::string(cdata);
}

/**
 * @brief 正确剔除操作（发送信号给PLC）
 * @details 向PLC写入0值，表示产品合格，停止定时器
 */
void Widget::rightremove()
{
    if (!client->Connected())
    {
        return;
    }

    uint8_t value = 0;
    byte remove_data[1] = {0};
    remove_data[0] = (unsigned char)(0xFF & value);

    // 写入DB1.1033位置，1个字节
    int tmp2 = client->WriteArea(S7AreaDB, 1, 1033, 1, S7WLByte, remove_data);
    if (tmp2 != 0)
    {
        QMessageBox::warning(this, "error", "设置失败");
    }
    timer->stop();
}

/**
 * @brief 错误剔除操作（发送信号给PLC）
 * @details 向PLC写入49值，表示产品不合格，需要剔除，100ms后恢复
 */
void Widget::wrongremove()
{
    if (!client->Connected())
    {
        return;
    }

    uint8_t value = 49;
    byte remove_data[1] = {0};
    remove_data[0] = (unsigned char)(0xFF & value);

    // 写入DB1.1033位置，1个字节
    int tmp2 = client->WriteArea(S7AreaDB, 1, 1033, 1, S7WLByte, remove_data);
    if (tmp2 != 0)
    {
        QMessageBox::warning(this, "error", "设置失败");
    }
    else
    {
        // 100ms后调用rightremove恢复信号
        timer->start(100);
    }
}

/**
 * @brief 保存ui图像（带选择框裁剪）
 * @param format 图像格式（如jpg、png、bmp）
 * @param savePath 保存路径
 * @details 根据用户绘制的选择框裁剪图像并保存为模板
 */
void Widget::saveImage(QString format, QString savePath)
{
    // 检查是否有图像可保存
    if (ui->image_undetected->pixmap() == nullptr)
    {
        QMessageBox::warning(this, "警告", "保存失败,未采集到图像!");
        return;
    }

    // 获取QLabel中显示的图像
    QPixmap pixmap = *(ui->image_undetected->pixmap());
    QImage img = pixmap.toImage();
    Mat* image = QImageToMat(img);  // 假设QImageToMat是正确的转换函数
    if (image == nullptr || image->empty())
    {
        QMessageBox::warning(this, "警告", "图像转换失败!");
        return;
    }

    // 处理格式字符串（去除可能的点号）
    if (format.startsWith("."))
    {
        format = format.mid(1);
    }

    // 确保保存目录存在
    QDir dir;
    if (!dir.mkpath(savePath))
    {
        qDebug() << "目录创建失败!";
        QMessageBox::warning(this, "警告", "保存失败,无法创建目录!");
        delete image;
        return;
    }

    // 获取用户在QLabel上绘制的选择框
    QRect selectionRect = imageLabel->getSelectionRect();
    if (selectionRect.isNull() || selectionRect.width() <= 0 || selectionRect.height() <= 0)
    {
        QMessageBox::warning(this, "警告", "未选择有效区域!");
        delete image;
        return;
    }

    // 关键修复：计算图像在QLabel中的实际显示尺寸和偏移（解决缩放/留白问题）
    // 1. 获取原始图像尺寸（OpenCV Mat: cols=宽, rows=高）
    QSize originalImageSize(image->cols, image->rows);
    // 2. 获取QLabel的显示尺寸
    QSize labelSize = imageLabel->size();
    if (labelSize.width() <= 0 || labelSize.height() <= 0)
    {
        QMessageBox::warning(this, "警告", "图像显示区域无效!");
        delete image;
        return;
    }

    // 3. 计算图像在QLabel中的实际缩放尺寸（按QLabel的缩放模式，通常是保持宽高比）
    // 注意：需与QLabel的实际缩放模式一致（如ui->image_undetected的scaledContents属性）
    QSize scaledImageSize = originalImageSize.scaled(labelSize, Qt::KeepAspectRatio);

    // 4. 计算图像在QLabel中的偏移量（因居中显示导致的留白补偿）
    int xOffset = (labelSize.width() - scaledImageSize.width()) / 2;   // 水平偏移（左留白）
    int yOffset = (labelSize.height() - scaledImageSize.height()) / 2; // 垂直偏移（上留白）

    // 5. 修正用户选择框：排除QLabel的留白区域，只保留图像显示区域内的部分
    QRectF adjustedRect(
                selectionRect.left() - xOffset,    // 减去水平偏移，得到相对于图像显示区域的X坐标
                selectionRect.top() - yOffset,     // 减去垂直偏移，得到相对于图像显示区域的Y坐标
                selectionRect.width(),
                selectionRect.height()
                );

    // 6. 确保修正后的区域完全在图像显示区域内（避免超出显示范围）
    QRectF validImageRect(0, 0, scaledImageSize.width(), scaledImageSize.height());
    adjustedRect = adjustedRect.intersected(validImageRect);
    if (adjustedRect.isNull() || adjustedRect.width() <= 0 || adjustedRect.height() <= 0)
    {
        QMessageBox::warning(this, "警告", "选择区域超出图像范围!");
        delete image;
        return;
    }

    // 7. 计算正确的缩放比例（原始图像尺寸 / 显示尺寸）
    double xRatio = static_cast<double>(originalImageSize.width()) / scaledImageSize.width();
    double yRatio = static_cast<double>(originalImageSize.height()) / scaledImageSize.height();

    // 8. 将修正后的区域转换为原始图像的ROI（OpenCV坐标）
    cv::Rect roi(
                static_cast<int>(adjustedRect.left() * xRatio),    // 原始图像中的X起点
                static_cast<int>(adjustedRect.top() * yRatio),     // 原始图像中的Y起点
                static_cast<int>(adjustedRect.width() * xRatio),   // 原始图像中的宽度
                static_cast<int>(adjustedRect.height() * yRatio)   // 原始图像中的高度
                );

    // 9. 最终校验：确保ROI在原始图像边界内
    roi &= cv::Rect(0, 0, image->cols, image->rows);
    if (roi.width <= 0 || roi.height <= 0)
    {
        QMessageBox::warning(this, "警告", "无效的裁剪区域!");
        delete image;
        return;
    }

    // 裁剪图像
    cv::Mat croppedImage = (*image)(roi);

    // 保存裁剪后的模板图像
    muban = &croppedImage;  // 注意：这里是指针引用，需确保croppedImage生命周期有效
    std::string savename = "muban.png";
    if (cv::imwrite(savename, croppedImage))
    {
        QMessageBox::information(this, "提示", "模板muban.png保存成功!");
    }
    else
    {
        QMessageBox::warning(this, "警告", "保存失败!可能不支持该图像格式或路径错误。");
    }

    // 释放资源
    delete image;
}

/**
 * @brief 保存当前显示的图像（无裁剪）
 * @param format 图像格式
 * @param savePath 保存路径
 * @details 保存完整的检测图像到指定目录，用于OK/NG样本收集
 */
//异步保存相机图像 减少耗时
void Widget::saveImage2Async(QString format, QString savePath)
{
    // 捕获当前相机状态和相关参数，避免异步过程中相机状态变化
    if (!m_pcMyCamera || !m_bOpenDevice)
    {
        qDebug() << "保存失败，相机对象无效或未打开";
        return;
    }

    // 确保格式正确
    if (format.startsWith("."))
    {
        format = format.mid(1);
    }

    if (format.isEmpty())
    {
        format = "png";
    }

    // 确保目录存在
    QDir dir;
    if (!dir.mkpath(savePath))
    {
        qDebug() << "目录创建失败！路径：" << savePath;
        return;
    }

    // 确保路径以斜杠结尾
    if (!savePath.endsWith("/") && !savePath.endsWith("\\"))
    {
        savePath += "/";
    }

    // 生成文件名
    QString curDate = QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss-zzz");
    QString saveName = savePath + curDate + "." + format;

    // 使用QtConcurrent在后台线程执行图像获取和保存操作
    QtConcurrent::run([this, saveName, format]() {
        try {
            // 方法1：使用ReadBuffer代替GetImageBuffer
            cv::Mat capturedImage;
            int result = m_pcMyCamera->ReadBuffer(capturedImage);

            // 如果ReadBuffer失败，尝试使用GetImage（基于回调的方法）
            if (result != 0 || capturedImage.empty()) {
                qDebug() << "ReadBuffer失败，尝试使用GetImage...";
                capturedImage = m_pcMyCamera->GetImage();
            }

            // 检查图像是否有效
            if (capturedImage.empty()) {
                qDebug() << "获取图像失败：空图像";
                return;
            }

            // 在后台线程中进行图像转换和保存
            QImage qImage;
            if (capturedImage.channels() == 3) {
                cv::Mat rgbImage;
                cv::cvtColor(capturedImage, rgbImage, cv::COLOR_BGR2RGB);
                qImage = QImage(rgbImage.data, rgbImage.cols, rgbImage.rows,
                                static_cast<int>(rgbImage.step), QImage::Format_RGB888).copy();
            }
            else if (capturedImage.channels() == 1) {
                qImage = QImage(capturedImage.data, capturedImage.cols, capturedImage.rows,
                                static_cast<int>(capturedImage.step), QImage::Format_Grayscale8).copy();
            }
            else {
                qDebug() << "不支持的图像通道数：" << capturedImage.channels();
                return;
            }

            // 保存图像
            if (!qImage.save(saveName, format.toUpper().toStdString().c_str())) {
                qDebug() << "保存图像失败！";
            }
            else {
                qDebug() << "异步保存图像成功：" << saveName;
            }
        }
        catch(const std::exception& e) {
            qDebug() << "异步保存过程中发生异常:" << e.what();
        }
        catch(...) {
            qDebug() << "异步保存过程中发生未知异常";
        }
    });
}



//// 在 Widget 或需要保存图像的地方调用
//void Widget::saveImageByMVS(QString savePath, QString format)
//{
//    int nRet = MV_OK;

//    // 相机状态校验
//    if (!m_pcMyCamera || !m_bOpenDevice) {
//        qDebug() << "保存失败：相机未打开";
//        return;
//    }

//    // 1. 格式预处理（对齐第二个函数：仅去前缀、判空，保留原白名单校验增强兼容性）
//    format = format.trimmed();
//    if (format.startsWith(".")) {
//        format = format.mid(1);
//    }
//    if (format.isEmpty()) {
//        format = "png";
//    }
//    // 保留原白名单校验（避免无效格式，比第二个函数更严谨）
//    QStringList validFormats = {"bmp", "jpeg", "png", "tiff"};
//    if (!validFormats.contains(format.toLower())) {
//        qDebug() << "unsupport format" << format << "，默认使用 png";
//        format = "png";
//    }

//    // 2. 确保目录存在（对齐第二个函数：先处理目录，再补全路径）
//    QDir dir;
//    if (!dir.mkpath(savePath)) {
//        qDebug() << "path create fail" << savePath;
//        return;
//    }

//    // 3. 补全路径分隔符（对齐第二个函数：目录创建后补全）
//    if (!savePath.endsWith("/") && !savePath.endsWith("\\")) {
//        savePath += "/";
//    }

//    // 4. 生成完整路径（与第二个函数完全一致：时间戳格式、命名规则）
//    QString curDate = QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss-zzz");
//    QString fullSavePath = savePath + curDate + "." + format;

//    // 关键修正：先声明 saveParam，再用其成员做路径长度检查
//    MV_SAVE_IMG_TO_FILE_PARAM saveParam;
//    memset(&saveParam, 0, sizeof(MV_SAVE_IMG_TO_FILE_PARAM)); // 整体清零，避免枚举类型初始化错误

//    // 路径长度检查（保留原严谨性，避免缓冲区溢出）
//    if (fullSavePath.toLocal8Bit().length() >= sizeof(saveParam.pImagePath)) {
//        qDebug() << "path too long" << fullSavePath;
//        return;
//    }

//    // 定义帧信息结构体
//    MV_FRAME_OUT stFrameOut;
//    memset(&stFrameOut, 0, sizeof(MV_FRAME_OUT)); // 整体清零，包含嵌套子结构体

//    // 获取图像
//    nRet = m_pcMyCamera->GetImageBuffer(&stFrameOut, 1000);
//    if (nRet != MV_OK) {
//        qDebug() << "get image fail wrong info：" << nRet;
//        return;
//    }

//    // 检查帧数据有效性
//    if (!stFrameOut.pBufAddr) {
//        qDebug() << "get image fail data is empty";
//        m_pcMyCamera->FreeImageBuffer(&stFrameOut);
//        return;
//    }

//    // 填充 saveParam 其他参数
//    saveParam.enPixelType = static_cast<enum MvGvspPixelType>(stFrameOut.stFrameInfo.enPixelType);
//    saveParam.pData = stFrameOut.pBufAddr;
//    saveParam.nDataLen = stFrameOut.stFrameInfo.nFrameLen;
//    saveParam.nWidth = stFrameOut.stFrameInfo.nWidth;
//    saveParam.nHeight = stFrameOut.stFrameInfo.nHeight;

//    // 映射保存格式
//    QString lowerFormat = format.toLower();
//    if (lowerFormat == "bmp") saveParam.enImageType = MV_Image_Bmp;
//    else if (lowerFormat == "jpeg") saveParam.enImageType = MV_Image_Jpeg;
//    else if (lowerFormat == "png") saveParam.enImageType = MV_Image_Png;
//    else if (lowerFormat == "tiff") saveParam.enImageType = MV_Image_Tif;

//    // 编码质量
//    if (lowerFormat == "jpeg") saveParam.nQuality = 60;
//    else if (lowerFormat == "png") saveParam.nQuality = 5;
//    else saveParam.nQuality = 0;

//    // 保存路径（与原逻辑一致，确保安全拷贝）
//    const char* filePath = fullSavePath.toLocal8Bit().data();
//    strncpy_s(saveParam.pImagePath, filePath, sizeof(saveParam.pImagePath) - 1);
//    saveParam.pImagePath[sizeof(saveParam.pImagePath) - 1] = '\0';

//    // 插值方法
//    saveParam.iMethodValue = 1;

//    // 保存图像
//    nRet = m_pcMyCamera->SaveImageToFile(&saveParam);
//    if (nRet == MV_OK) {
//        qDebug() << "save success：" << fullSavePath;
//    } else {
//        qDebug() << "save fail wrong info：" << nRet;
//    }

//    // 释放缓冲区
//    m_pcMyCamera->FreeImageBuffer(&stFrameOut);
//}



////异步保存ui上的图像
void Widget::saveImage2(QString format, QString savePath)
{
    // 1. 主线程中先校验UI图像和参数（避免跨线程访问UI）
    const QPixmap* curPixmap = ui->image_undetected->pixmap();
    if (!curPixmap) {
        QMessageBox::warning(this, "警告", "保存失败,未采集到图像！");
        return;
    }

    // 复制UI图像到主线程局部变量（避免跨线程访问UI控件）
    QPixmap pixmap = *curPixmap;
    QImage img = pixmap.toImage();

    // 处理文件格式
    if (format.startsWith(".")) {
        format = format.mid(1);
    }
    if (format.isEmpty()) {
        format = "png"; // 默认格式
    }

    // 确保目录存在
    QDir dir;
    if (!dir.mkpath(savePath)) {
        qDebug() << "目录创建失败！路径：" << savePath;
        return;
    }

    // 确保路径以斜杠结尾
    if (!savePath.endsWith("/") && !savePath.endsWith("\\")) {
        savePath += "/";
    }

    // 生成带时间戳的文件名（主线程生成，避免线程安全问题）
    QString curDate = QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss.zzz");
    QString saveName = savePath + curDate + "." + format;

    // 2. 使用QtConcurrent在后台线程执行保存操作（核心异步逻辑）
    QtConcurrent::run([=]() { // 捕获复制后的局部变量，避免跨线程访问UI
        try {
            // 后台线程中执行保存（QImage是可重入的，支持跨线程操作）
            if (img.save(saveName, format.toUpper().toStdString().c_str())) {
                qDebug() << "UI图像异步保存成功：" << saveName;
            } else {
                qDebug() << "UI图像异步保存失败！路径：" << saveName;
            }
        } catch (const std::exception& e) {
            qDebug() << "UI图像异步保存异常:" << e.what();
        } catch (...) {
            qDebug() << "UI图像异步保存发生未知异常";
        }
    });
}

/**
 * @brief 显示图像槽函数
 * @param image OpenCV Mat图像指针
 * @details 将OpenCV图像转换为QPixmap并显示在UI上
 */
void Widget::slot_displayAndDetect(cv::Mat *image)
{
    // 1. 校验图像有效性
    if (!image || image->empty()) return;

    // 2. 深拷贝原图，准备作为画板
    cv::Mat displayImg;
    if (image->channels() == 3) {
        displayImg = image->clone();
    } else if (image->channels() == 1) {
        cv::cvtColor(*image, displayImg, cv::COLOR_GRAY2BGR);
    } else {
        return;
    }

    // 3. 核心重绘机制：只要在检测有效期内（比如最近 2000 毫秒），每次来新原图，都把框重新印上去！
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - g_lastDetectTime < 2000 && !g_lastDrawResults.empty()) {
        for (const auto& res : g_lastDrawResults) {
            cv::Rect rect = res.rect;
            rect.x += g_lastRoi.x; // 绝对坐标映射还原
            rect.y += g_lastRoi.y;

            cv::rectangle(displayImg, rect, cv::Scalar(0, 255, 0), 2);

            std::string scoreText = std::to_string(static_cast<int>(res.score * 100));
            int baseline = 0;
            double fontScale = 0.8;
            cv::Size textSize = cv::getTextSize(scoreText, cv::FONT_HERSHEY_SIMPLEX, fontScale, 2, &baseline);

            cv::Point boxCenter(rect.x + rect.width / 2, rect.y);
            int textX = boxCenter.x - textSize.width / 2;
            int textY = boxCenter.y - baseline - 2;

            // 边界保护，防止文字框画到图片外面
            textX = std::max(0, std::min(textX, displayImg.cols - textSize.width));
            textY = std::max(textSize.height, std::min(textY, displayImg.rows));

            cv::rectangle(displayImg,
                cv::Point(textX - 2, textY - textSize.height),
                cv::Point(textX + textSize.width + 2, textY + 0.5),
                cv::Scalar(255, 255, 255), cv::FILLED);

            cv::putText(displayImg, scoreText, cv::Point(textX, textY),
                cv::FONT_HERSHEY_SIMPLEX, fontScale, cv::Scalar(255, 0, 0), 2);
        }
    }

    // 4. OpenCV Mat 转 Qt QImage
    QImage img((const uchar *)displayImg.data, displayImg.cols, displayImg.rows, displayImg.step, QImage::Format_RGB888);
    img = img.rgbSwapped();

    // 5. 强制拉伸图像铺满Label
    QSize labelSize = ui->image_undetected->size();
    QPixmap pixmap = QPixmap::fromImage(img);
    QPixmap scaledPixmap = pixmap.scaled(labelSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    // 6. 在Label上显示
    ui->image_undetected->setPixmap(scaledPixmap);
    ui->image_undetected->setAlignment(Qt::AlignCenter);
}

/**
 * @brief OCR识别检测槽函数
 * @param image 输入图像指针
 * @param diffbox 检测区域
 * @details 使用PaddleOCR进行文字识别，支持中英文、数字识别
 */

//ui缩放版
//void Widget::slot_readAndDetect(cv::Mat *image, Rect2d diffbox)
//{
//    // 检查延迟剔除队列
//    if (!removalQueue.empty() && totalImages >= removalQueue.front().second - 1)
//    {
//        qDebug() << "wrongindex" << wrongindex;
//        wrongremove();
//        removalQueue.pop();
//    }

//    currentImagesSnapshot = totalImages; // 每次循环中记录当前 totalImages 的快照
//    auto start = std::chrono::high_resolution_clock::now();
//    if (!image || image->empty())
//    {
//        qDebug() << "Error: Invalid input image.";
//        return;
//    }
//    cv::Mat croppedImage;
//    // vector<QRect> detectedRects;
//    QRect detRect(0, 0, 0, 0);
//    QRect detRect1(0, 0, 0, 0);
//    // 1红色2绿色3蓝色
//    imageLabel->setColor(color);
//    // imageLabel->clearGreenRects();
//    if (judge)
//    {
//        j = 1;
//        x++;
//        judge = false;
//    }
//    if ((j - 1) % x == 0)
//    {
//        imageLabel->clearGreenRects();
//        detectedRects.clear();
//        string1.clear();
//    }
//    if (!first)
//    {
//        double scaleX = static_cast<double>(ui->image_undetected->width()) / image->cols;
//        double scaleY = static_cast<double>(ui->image_undetected->height()) / image->rows;
//        int offsetX = (ui->image_undetected->width() - scaleX * image->cols) / 2;
//        int offsetY = (ui->image_undetected->height() - scaleY * image->rows) / 2;
//        QRect selectionRect1(
//            static_cast<int>(diffbox.x * scaleX + offsetX),
//            static_cast<int>(diffbox.y * scaleY + offsetY),
//            static_cast<int>(diffbox.width * scaleX),
//            static_cast<int>(diffbox.height * scaleY));
//        selectionRect = selectionRect1;
//    }
//    else
//    {
//        QRect selectionRect1(
//            static_cast<int>(diffbox.x),
//            static_cast<int>(diffbox.y),
//            static_cast<int>(diffbox.width),
//            static_cast<int>(diffbox.height));
//        selectionRect = selectionRect1;
//    }
//    // 确保有有效的选择区域
//    if (selectionRect.isNull())
//    {
//        //        QMessageBox::warning(this, "警告", "没有选择区域！");
//        return;
//    }
//    xRatio = static_cast<double>(image->cols) / imageLabel->width();
//    yRatio = static_cast<double>(image->rows) / imageLabel->height();
//    roi = cv::Rect(
//        static_cast<int>(selectionRect.left() * xRatio),
//        static_cast<int>(selectionRect.top() * yRatio),
//        static_cast<int>(selectionRect.width() * xRatio),
//        static_cast<int>(selectionRect.height() * yRatio)

//    );
//    roi &= cv::Rect(0, 0, image->cols, image->rows);
//    if (roi.width <= 0 || roi.height <= 0)
//    {
//        QMessageBox::warning(this, "警告", "选择区域无效！");
//        return;
//    }
//    else
//    {
//        // 提取 ROI
//        croppedImage = (*image)(roi);
//    }
//    if (croppedImage.type() != CV_8UC3)
//    {
//        cv::Mat temp;
//        cv::cvtColor(croppedImage, temp, cv::COLOR_GRAY2BGR);
//        croppedImage = temp;
//    }
//    QString target_qstring = setdatetime();
//    std::string target_string = target_qstring.toStdString();
//    ui->imagenum->setText(QString::number(totalImages));
//    std::vector<std::vector<std::vector<int>>> boxes;
//    det->Run(croppedImage, boxes);
//    cv::imshow("croppedimage", croppedImage);
//    vector<std::pair<std::string, cv::Rect>> str_res = rec->RunOCR(boxes, croppedImage, cls);
//    string str;
//    std::sort(str_res.begin(), str_res.end(), [](const auto &a, const auto &b)
//              {
//                  return a.second.y < b.second.y; // 按 Y 坐标升序排序（顶部元素在前）
//              });

//    // 清空之前的结果
//    allResults.clear();
//    detectedRects.clear();

//    qDebug()<<"11111";
//    // 现在按排序后的顺序处理结果
//    for (size_t i = 0; i < str_res.size(); i++)
//    {
//        auto &res = str_res[i];
//        // 1. 输出【原始OCR识别结果】（清理前）
//            qDebug() << "number" << i+1 << "text：" << QString::fromStdString(res.first);


//        // 清理文本：移除非字母数字和中文字符
//        res.first.erase(std::remove_if(res.first.begin(), res.first.end(), [this](char c)
//                                       { return !isAlnumOrChinese(c); }),
//                        res.first.end());

//        // 添加处理后的文本（只有一行时不加换行符）
//        if (i < str_res.size() - 1)
//        {
//            allResults += res.first + '\n'; // 不是最后一行，添加换行符
//        }
//        else
//        {
//            allResults += res.first; // 最后一行，不加换行符
//        }
//        qDebug() <<"allresults"<< QString::fromStdString(allResults.c_str());

//        // 处理检测区域
//        detRect1.setX(res.second.x);
//        detRect1.setY(res.second.y);
//        detRect1.setWidth(res.second.width);
//        detRect1.setHeight(res.second.height);

//        // 转换为原始图像坐标
//        detRect.setX((detRect1.topLeft().x() + roi.x) / xRatio);
//        detRect.setY((detRect1.topLeft().y() + roi.y) / yRatio);
//        detRect.setWidth(detRect1.width() / xRatio);
//        detRect.setHeight(detRect1.height() / yRatio);
//        detectedRects.push_back(detRect);
//    }

//    qDebug()<<"2222";
//    qDebug() << "allresults" << QString::fromStdString(allResults);

//    // 显示 OCR 结果
//    ui->resultlabel_2->setText(QString::fromStdString(allResults));
//    ui->resultlabel_2->setWordWrap(true);

//    // 增大字体
//    QFont font = ui->resultlabel_2->font();
//    font.setPointSize(16);
//    ui->resultlabel_2->setFont(font);

//    // 添加检测框
//    for (const auto &rect : detectedRects)
//    {
//        imageLabel->addSelectionRect(rect, 2);
//    }
//    imageLabel->update();

//    // 判断识别结果
//    if (j % x == 0)
//    {
//        if (allResults.empty())
//        {
//            // 识别失败：保存NG图像
//            if ((ui->comboBox->currentIndex() == 1) || (ui->comboBox->currentIndex() == 3))
//            {
//                QString saveDir = selectedDir + "/ng/";
//                saveImage2("jpg", saveDir);
//            }

//            ngImages++;
//            totalImages++;
//            ui->resultlabel->setText(QString("<font size='10' color='red'>错误！</font>"));

//            // 剔除逻辑
//            if (wrongindex == 0)
//            {
//                qDebug() << "wrongindex=0, calling wrongremove immediately";
//                wrongremove();
//            }
//            else
//            {
//                qDebug() << "wrongindex=" << wrongindex << ", adding to queue for delayed removal";
//                removalQueue.push(std::make_pair(totalImages, totalImages + wrongindex));
//            }
//        }
//        else
//        {
//            if (allResults == target_string)
//            {
//                // 识别成功：保存OK图像
//                totalImages++;
//                if ((ui->comboBox->currentIndex() == 2) || (ui->comboBox->currentIndex() == 3))
//                {
//                    QString saveDir = selectedDir + "/ok/";
//                    saveImage2("jpg", saveDir);
//                }
//                ui->resultlabel->setText(QString("<font size='10' color='SpringGreen'>正确！</font><br>"));
//                rightremove();
//            }
//            else
//            {
//                // 识别内容不匹配：保存NG图像
//                if ((ui->comboBox->currentIndex() == 1) || (ui->comboBox->currentIndex() == 3))
//                {
//                    QString saveDir = selectedDir + "/ng/";
//                    saveImage2("jpg", saveDir);
//                }

//                ngImages++;
//                totalImages++;
//                ui->resultlabel->setText(QString("<font size='10' color='red'>错误！</font>"));

//                if (wrongindex == 0)
//                {
//                    qDebug() << "wrongindex=0, calling wrongremove immediately";
//                    wrongremove();
//                }
//                else
//                {
//                    qDebug() << "wrongindex=" << wrongindex << ", adding to queue for delayed removal";
//                    removalQueue.push(std::make_pair(totalImages, totalImages + wrongindex));
//                }
//            }
//        }
//    }

//    // 更新统计信息
//    double hegerate = (1 - static_cast<double>(ngImages) / totalImages) * 100;
//    QString str1 = QString::number(hegerate, 'f', 1);
//    ui->lineBoxIndex_6->setText(str1);
//    ui->ngnum->setText(QString("%1").arg(ngImages));
//    ui->imagenum->setText(QString("%1").arg(totalImages));

//    // 显示检测耗时
//    auto end = std::chrono::high_resolution_clock::now();
//    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
//    ui->speedLabel->setText(QString("检测耗时 %1 毫秒").arg(duration));

//    j++;
//}

//原始图像版
void Widget::slot_readAndDetect(cv::Mat *image, Rect2d diffbox)
{
    // 检查延迟剔除队列
    if (!removalQueue.empty() && totalImages >= removalQueue.front().second - 1)
    {
        qDebug() << "wrongindex" << wrongindex;
        wrongremove();
        removalQueue.pop();
    }

    currentImagesSnapshot = totalImages; // 每次循环中记录当前 totalImages 的快照
    auto start = std::chrono::high_resolution_clock::now();
    if (!image || image->empty())
    {
        qDebug() << "Error: Invalid input image.";
        return;
    }
    cv::Mat croppedImage;
    QRect detRect(0, 0, 0, 0);  // 用于存储转换后的原始图像坐标检测框
    cv::Rect detRect1(0, 0, 0, 0); // 改为cv::Rect，直接对应原始图像坐标

    // 1红色2绿色3蓝色
    imageLabel->setColor(color);

    if (judge)
    {
        j = 1;
        x++;
        judge = false;
    }
    if ((j - 1) % x == 0)
    {
        imageLabel->clearGreenRects();
        detectedRects.clear();
        string1.clear();
    }

    // ===================== 核心修改：完全基于原始图像坐标 =====================
    // 1. 直接使用传入的diffbox作为原始图像上的选择区域（diffbox需是原始图像坐标）
    // 转换Rect2d为cv::Rect（取整，因像素坐标为整数）
    cv::Rect rawSelectionRect(
                static_cast<int>(diffbox.x),
                static_cast<int>(diffbox.y),
                static_cast<int>(diffbox.width),
                static_cast<int>(diffbox.height));

    // 2. 确保选择区域在原始图像范围内（避免越界）
    if (rawSelectionRect.empty())
    {
        qDebug() << "Error: Empty selection area in raw image.";
        return;
    }
    // 裁剪选择区域，确保不超出原始图像边界
    cv::Rect roi = rawSelectionRect & cv::Rect(0, 0, image->cols, image->rows);
    if (roi.width <= 0 || roi.height <= 0)
    {
        QMessageBox::warning(this, "警告", "选择区域无效（超出原始图像范围）！");
        return;
    }

    // 3. 直接从原始图像裁剪ROI，生成croppedImage
    croppedImage = (*image)(roi);
    // =========================================================================

    // 图像类型统一（与原逻辑一致，确保后续OCR兼容）
    if (croppedImage.type() != CV_8UC3)
    {
        cv::Mat temp;
        cv::cvtColor(croppedImage, temp, cv::COLOR_GRAY2BGR);
        croppedImage = temp;
    }

    // 目标字符串生成（与原逻辑一致）
    QString target_qstring = setdatetime();
    std::string target_string = target_qstring.toStdString();
    ui->imagenum->setText(QString::number(totalImages));

    // OCR检测（与原逻辑一致，输入为裁剪后的croppedImage）
    std::vector<std::vector<std::vector<int>>> boxes;
    det->Run(croppedImage, boxes);
    vector<std::pair<std::string, cv::Rect>> str_res = rec->RunOCR(boxes, croppedImage, cls);

    // 按Y坐标排序（与原逻辑一致，基于croppedImage内的坐标排序）
    std::sort(str_res.begin(), str_res.end(), [](const auto &a, const auto &b)
    {
        return a.second.y < b.second.y; // 按 Y 坐标升序排序（顶部元素在前）
    });

    // 清空之前的结果
    allResults.clear();
    detectedRects.clear();
    // 处理OCR结果（坐标逻辑简化：直接映射回原始图像）
    for (size_t i = 0; i < str_res.size(); i++)
    {
        auto &res = str_res[i];
        // 输出原始OCR识别结果
        qDebug() << "number" << i+1 << "text：" << QString::fromStdString(res.first);

        // 清理文本（与原逻辑一致）
        res.first.erase(std::remove_if(res.first.begin(), res.first.end(), [this](char c)
        { return !isAlnumOrChinese(c); }),
                        res.first.end());

        // 拼接结果文本（与原逻辑一致）
        if (i < str_res.size() - 1)
        {
            allResults += res.first + '\n';
        }
        else
        {
            allResults += res.first;
        }


        // ===================== 坐标处理简化：直接关联原始图像 =====================
        // 1. res.second是OCR在croppedImage内的局部坐标，转换为原始图像坐标
        detRect1.x = res.second.x + roi.x;    // croppedImage内x + roi在原始图像的x偏移
        detRect1.y = res.second.y + roi.y;    // croppedImage内y + roi在原始图像的y偏移
        detRect1.width = res.second.width;    // 宽度与croppedImage内一致（原始像素尺寸）
        detRect1.height = res.second.height;  // 高度与croppedImage内一致

        // 2. 转换为QRect（用于UI绘制检测框，仅此处涉及UI坐标映射，不影响核心逻辑）
        // 注：此处仍需映射到UI坐标，因imageLabel显示的是缩放后的图像，确保框位置准确
        double uiScaleX = static_cast<double>(imageLabel->width()) / image->cols;
        double uiScaleY = static_cast<double>(imageLabel->height()) / image->rows;
        detRect.setX(static_cast<int>(detRect1.x * uiScaleX));
        detRect.setY(static_cast<int>(detRect1.y * uiScaleY));
        detRect.setWidth(static_cast<int>(detRect1.width * uiScaleX));
        detRect.setHeight(static_cast<int>(detRect1.height * uiScaleY));
        detectedRects.push_back(detRect);
        // =========================================================================
    }

    // OCR结果显示（与原逻辑一致）
    ui->resultlabel_7->setText(QString::fromStdString(allResults));
    ui->resultlabel_7->setWordWrap(true);
    QFont font = ui->resultlabel_7->font();
    font.setPointSize(16);
    ui->resultlabel_7->setFont(font);

    // 绘制检测框（与原逻辑一致，基于转换后的UI坐标）
    for (const auto &rect : detectedRects)
    {
        imageLabel->addSelectionRect(rect, 2);
    }
    imageLabel->update();

    // 识别结果判定与后续逻辑（与原逻辑完全一致）
    if (j % x == 0)
    {
        if (allResults.empty())
        {
            if ((ui->comboBox->currentIndex() == 1) || (ui->comboBox->currentIndex() == 3))
            {
                QString saveDir = selectedDir + "/ng/";
                saveImage2Async("jpg", saveDir);
            }
            ngImages++;
            totalImages++;
            ui->resultlabel->setText(QString("<font size='10' color='red'>错误！</font>"));
            if (wrongindex == 0)
            {
                qDebug() << "wrongindex=0, calling wrongremove immediately";
                wrongremove();
            }
            else
            {
                qDebug() << "wrongindex=" << wrongindex << ", adding to queue for delayed removal";
                removalQueue.push(std::make_pair(totalImages, totalImages + wrongindex));
            }
        }
        else
        {
            if (allResults == target_string)
            {
                totalImages++;
                if ((ui->comboBox->currentIndex() == 2) || (ui->comboBox->currentIndex() == 3))
                {
                    QString saveDir = selectedDir + "/ok/";
                    saveImage2Async("jpg", saveDir);
                }
                ui->resultlabel->setText(QString("<font size='10' color='SpringGreen'>正确！</font><br>"));
                rightremove();
            }
            else
            {
                if ((ui->comboBox->currentIndex() == 1) || (ui->comboBox->currentIndex() == 3))
                {
                    QString saveDir = selectedDir + "/ng/";
                    saveImage2Async("jpg", saveDir);
                }
                ngImages++;
                totalImages++;
                ui->resultlabel->setText(QString("<font size='10' color='red'>错误！</font>"));
                if (wrongindex == 0)
                {
                    qDebug() << "wrongindex=0, calling wrongremove immediately";
                    wrongremove();
                }
                else
                {
                    qDebug() << "wrongindex=" << wrongindex << ", adding to queue for delayed removal";
                    removalQueue.push(std::make_pair(totalImages, totalImages + wrongindex));
                }
            }
        }
    }

    // 更新统计信息（与原逻辑一致）
    double hegerate = (1 - static_cast<double>(ngImages) / totalImages) * 100;
    QString str1 = QString::number(hegerate, 'f', 1);
    ui->lineBoxIndex_6->setText(str1);
    ui->ngnum->setText(QString("%1").arg(ngImages));
    ui->imagenum->setText(QString("%1").arg(totalImages));

    // 显示检测耗时（与原逻辑一致）
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    ui->speedLabel->setText(QString("检测耗时 %1 毫秒").arg(duration));

    j++;
}

/**
 * @brief 模板匹配检测槽函数
 * @param image 输入图像指针
 * @param diffbox 检测区域
 * @details 使用SSIM算法进行模板相似度匹配
 */

//原始图像版
void Widget::slot_readAndDetect3(cv::Mat *image, Rect2d diffbox)
{
    if (digitTemplates.empty()) {
        // 弹窗提示错误信息（父窗口为当前Widget，标题"错误"，内容说明问题）
        QMessageBox::warning(this, "错误", "模板为空，无法进行模板匹配！");
        return;  // 终止函数，不执行后续逻辑
    }

    // 检查延迟剔除队列
    if (!removalQueue.empty() && totalImages >= removalQueue.front().second - 1)
    {
        qDebug() << "wrongindex" << wrongindex;
        wrongremove();
        removalQueue.pop();
    }

    currentImagesSnapshot = totalImages;
    auto start = std::chrono::high_resolution_clock::now();

    if (!image || image->empty())
    {
        qDebug() << "Error: Invalid input image.";
        return;
    }

    cv::Mat croppedImage;

    imageLabel->setColor(color);

    if (judge)
    {
        j = 1;
        x++;
        judge = false;
    }

    if ((j - 1) % x == 0)
    {
        imageLabel->clearGreenRects();
        detectedRects.clear();
        string1.clear();
    }

    // ===================== 核心修改：完全基于原始图像坐标 =====================
    // 1. 直接使用传入的diffbox作为原始图像上的选择区域（diffbox需是原始图像坐标）
    // 转换Rect2d为cv::Rect（取整，因像素坐标为整数）
    cv::Rect rawSelectionRect(
                static_cast<int>(diffbox.x),
                static_cast<int>(diffbox.y),
                static_cast<int>(diffbox.width),
                static_cast<int>(diffbox.height));

    // 2. 确保选择区域在原始图像范围内（避免越界）
    if (rawSelectionRect.empty())
    {
        qDebug() << "Error: Empty selection area in raw image.";
        return;
    }
    // 裁剪选择区域，确保不超出原始图像边界
    cv::Rect roi = rawSelectionRect & cv::Rect(0, 0, image->cols, image->rows);
    if (roi.width <= 0 || roi.height <= 0)
    {
        QMessageBox::warning(this, "警告", "选择区域无效（超出原始图像范围）！");
        return;
    }

    // 3. 直接从原始图像裁剪ROI，生成croppedImage
    croppedImage = (*image)(roi);
    // =========================================================================

    // 图像类型统一（与原逻辑一致）
    if (croppedImage.type() != CV_8UC3)
    {
        cv::Mat temp;
        cv::cvtColor(croppedImage, temp, cv::COLOR_GRAY2BGR);
        croppedImage = temp;
    }

    // 发射裁剪后的图像信号（与原逻辑一致）
    emit imgshibie(&croppedImage);

    // 更新图像计数显示（与原逻辑一致）
    ui->imagenum->setText(QString::number(totalImages));

    // 执行模板匹配（与原逻辑一致）
    QString text = ui->lineEdit_yuzhi->text();
    int number = text.toInt();  // 修正：阈值应为整数，使用toInt更合理
    int ssimvalue = 100 * templatematch->run1(digitTemplates);
    ui->lineBoxIndex->setText(QString::number(ssimvalue));

    // 判断是否达到用户阈值（与原逻辑一致）
    QString judgeResult = (ssimvalue > number ? "yes" : "no");  // 变量名修改，避免与全局judge冲突

    // ===================== 检测框绘制：基于原始图像坐标映射到UI =====================
    // 清空旧检测框（补充逻辑：确保每次绘制的是当前ROI对应的框）
    detectedRects.clear();
    // 生成当前ROI在原始图像中的矩形框（用于UI绘制）
    cv::Rect rawDetectRect = roi;  // 模板匹配的检测框即ROI区域本身
    // 转换为UI坐标（仅用于显示，不影响核心逻辑）
    double uiScaleX = static_cast<double>(imageLabel->width()) / image->cols;
    double uiScaleY = static_cast<double>(imageLabel->height()) / image->rows;
    QRect uiDetectRect(
                static_cast<int>(rawDetectRect.x * uiScaleX),
                static_cast<int>(rawDetectRect.y * uiScaleY),
                static_cast<int>(rawDetectRect.width * uiScaleX),
                static_cast<int>(rawDetectRect.height * uiScaleY)
                );
    detectedRects.push_back(uiDetectRect);
    // =========================================================================

    // 绘制检测框（与原逻辑一致）
    for (const auto &rect : detectedRects)
    {
        imageLabel->addSelectionRect(rect, 2);
    }
    imageLabel->update();

    // 识别结果判定与后续逻辑（与原逻辑一致，修正变量名冲突）
    if (j % x == 0)
    {
        if (judgeResult == "no")  // 使用修改后的变量名
        {
            if ((ui->comboBox->currentIndex() == 1) || (ui->comboBox->currentIndex() == 3))
            {
                QString saveDir = selectedDir + "/ng/";
                saveImage2Async("jpg", saveDir);
            }

            ngImages++;
            totalImages++;
            ui->resultlabel->setText(QString("<font size='10' color='red'>错误！</font>"));

            if (wrongindex == 0)
            {
                qDebug() << "wrongindex=0, calling wrongremove immediately";
                wrongremove();
            }
            else
            {
                qDebug() << "wrongindex=" << wrongindex << ", adding to queue for delayed removal";
                removalQueue.push(std::make_pair(totalImages, totalImages + wrongindex));
            }
        }
        else
        {
            totalImages++;
            if ((ui->comboBox->currentIndex() == 2) || (ui->comboBox->currentIndex() == 3))
            {
                QString saveDir = selectedDir + "/ok/";
                saveImage2Async("jpg", saveDir);
            }
            ui->resultlabel->setText(QString("<font size='10' color='SpringGreen'>正确！</font><br>"));
            rightremove();
        }
    }

    // 更新统计信息（与原逻辑一致）
    double hegerate = (1 - static_cast<double>(ngImages) / totalImages) * 100;
    QString str1 = QString::number(hegerate, 'f', 1);
    ui->lineBoxIndex_6->setText(str1);
    ui->ngnum->setText(QString("%1").arg(ngImages));
    ui->imagenum->setText(QString("%1").arg(totalImages));

    // 显示检测耗时（与原逻辑一致）
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    ui->speedLabel->setText(QString("检测耗时 %1 毫秒").arg(duration));

    j++;
}

//根据ui缩放版
//void Widget::slot_readAndDetect3(cv::Mat *image, Rect2d diffbox)
//{
//    // 检查延迟剔除队列
//    if (!removalQueue.empty() && totalImages >= removalQueue.front().second - 1)
//    {
//        qDebug() << "wrongindex" << wrongindex;
//        wrongremove();
//        removalQueue.pop();
//    }

//    currentImagesSnapshot = totalImages;
//    auto start = std::chrono::high_resolution_clock::now();

//    if (!image || image->empty())
//    {
//        qDebug() << "Error: Invalid input image.";
//        return;
//    }

//    cv::Mat croppedImage;

//    imageLabel->setColor(color);

//    if (judge)
//    {
//        j = 1;
//        x++;
//        judge = false;
//    }

//    if ((j - 1) % x == 0)
//    {
//        imageLabel->clearGreenRects();
//        detectedRects.clear();
//        string1.clear();
//    }

//    // 计算缩放和偏移
//    double scaleX = static_cast<double>(ui->image_undetected->width()) / image->cols;
//    double scaleY = static_cast<double>(ui->image_undetected->height()) / image->rows;
//    int offsetX = (ui->image_undetected->width() - scaleX * image->cols) / 2;
//    int offsetY = (ui->image_undetected->height() - scaleY * image->rows) / 2;

//    // 构建选择区域
//    if (first)
//    {
//        QRect selectionRect1(
//            static_cast<int>(diffbox.x * scaleX + offsetX),
//            static_cast<int>(diffbox.y * scaleY + offsetY),
//            static_cast<int>(diffbox.width * scaleX),
//            static_cast<int>(diffbox.height * scaleY));
//        selectionRect = selectionRect1;
//    }
//    else
//    {
//        QRect selectionRect1(
//            static_cast<int>(diffbox.x),
//            static_cast<int>(diffbox.y),
//            static_cast<int>(diffbox.width),
//            static_cast<int>(diffbox.height));
//        selectionRect = selectionRect1;
//    }

//    if (selectionRect.isNull())
//    {
//        QMessageBox::warning(this, "警告", "没有选择区域！");
//        return;
//    }

//    xRatio = static_cast<double>(image->cols) / imageLabel->width();
//    yRatio = static_cast<double>(image->rows) / imageLabel->height();

//    roi = cv::Rect(
//        static_cast<int>(selectionRect.left() * xRatio),
//        static_cast<int>(selectionRect.top() * yRatio),
//        static_cast<int>(selectionRect.width() * xRatio),
//        static_cast<int>(selectionRect.height() * yRatio));

//    roi &= cv::Rect(0, 0, image->cols, image->rows);

//    if (roi.width <= 0 || roi.height <= 0)
//    {
//        QMessageBox::warning(this, "警告", "选择区域无效！");
//        return;
//    }

//    croppedImage = (*image)(roi);

//    if (croppedImage.type() != CV_8UC3)
//    {
//        cv::Mat temp;
//        cv::cvtColor(croppedImage, temp, cv::COLOR_GRAY2BGR);
//        croppedImage = temp;
//    }

//    emit imgshibie(&croppedImage);

//    ui->imagenum->setText(QString::number(totalImages));

//    // 执行模板匹配
//    QString text = ui->lineEdit_yuzhi->text();
//    int number = text.toDouble();
//    int ssimvalue = 100 * templatematch->run1(digitTemplates);
//    ui->lineBoxIndex->setText(QString::number(ssimvalue));

//    // 判断是否达到用户阈值
//    QString judge = (ssimvalue > number ? "yes" : "no");

//    for (const auto &rect : detectedRects)
//    {
//        imageLabel->addSelectionRect(rect, 2);
//    }
//    imageLabel->update();

//    if (j % x == 0)
//    {
//        if (judge == "no")
//        {
//            if ((ui->comboBox->currentIndex() == 1) || (ui->comboBox->currentIndex() == 3))
//            {
//                QString saveDir = selectedDir + "/ng/";
//                saveImage2("jpg", saveDir);
//            }

//            ngImages++;
//            totalImages++;
//            ui->resultlabel->setText(QString("<font size='10' color='red'>错误！</font>"));

//            if (wrongindex == 0)
//            {
//                qDebug() << "wrongindex=0, calling wrongremove immediately";
//                wrongremove();
//            }
//            else
//            {
//                qDebug() << "wrongindex=" << wrongindex << ", adding to queue for delayed removal";
//                removalQueue.push(std::make_pair(totalImages, totalImages + wrongindex));
//            }
//        }
//        else
//        {
//            totalImages++;
//            if ((ui->comboBox->currentIndex() == 2) || (ui->comboBox->currentIndex() == 3))
//            {
//                QString saveDir = selectedDir + "/ok/";
//                saveImage2("jpg", saveDir);
//            }
//            ui->resultlabel->setText(QString("<font size='10' color='SpringGreen'>正确！</font><br>"));
//            rightremove();
//        }
//    }

//    // 更新统计信息
//    double hegerate = (1 - static_cast<double>(ngImages) / totalImages) * 100;
//    QString str1 = QString::number(hegerate, 'f', 1);
//    ui->lineBoxIndex_6->setText(str1);
//    ui->ngnum->setText(QString("%1").arg(ngImages));
//    ui->imagenum->setText(QString("%1").arg(totalImages));

//    auto end = std::chrono::high_resolution_clock::now();
//    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
//    ui->speedLabel->setText(QString("检测耗时 %1 毫秒").arg(duration));

//    j++;
//}

/**
 * @brief 字库匹配检测槽函数
 * @param image 输入图像指针
 * @param diffbox 检测区域
 * @details 使用字符模板库进行字符数量匹配检测
 */


void Widget::slot_readAndDetect4(cv::Mat *image, Rect2d diffbox)
{
//    if (digitTemplates.empty()) {
//        // 弹窗提示错误信息（父窗口为当前Widget，标题"错误"，内容说明问题）
//        QMessageBox::warning(this, "错误", "未选择字库，无法进行模板匹配！");
//        return;  // 终止函数，不执行后续逻辑
//    }

    // 检查延迟剔除队列
    if (!removalQueue.empty() && totalImages >= removalQueue.front().second - 1)
    {
        qDebug() << "wrongindex" << wrongindex;
        wrongremove();
        removalQueue.pop();
    }

    currentImagesSnapshot = totalImages;
    auto start = std::chrono::high_resolution_clock::now();

    if (!image || image->empty())
    {
        qDebug() << "Error: Invalid input image.";
        return;
    }

    cv::Mat croppedImage;

    imageLabel->setColor(color);

    if (judge)
    {
        j = 1;
        x++;
        judge = false;
    }

    if ((j - 1) % x == 0)
    {
        imageLabel->clearGreenRects();
        detectedRects.clear();
        string1.clear();
    }

    // ===================== 完全基于原始图像坐标 =====================
    // 1. 直接使用传入的diffbox作为原始图像上的选择区域
    cv::Rect rawSelectionRect(
                static_cast<int>(diffbox.x),
                static_cast<int>(diffbox.y),
                static_cast<int>(diffbox.width),
                static_cast<int>(diffbox.height));

    // 2. 确保选择区域在原始图像范围内（避免越界）
    if (rawSelectionRect.empty())
    {
        qDebug() << "Error: Empty selection area in raw image.";
        return;
    }
    // 裁剪选择区域至原始图像边界内
    cv::Rect roi = rawSelectionRect & cv::Rect(0, 0, image->cols, image->rows);
    if (roi.width <= 0 || roi.height <= 0)
    {
        QMessageBox::warning(this, "警告", "选择区域无效（超出原始图像范围）！");
        return;
    }

    // 3. 直接从原始图像裁剪ROI生成croppedImage
    croppedImage = (*image)(roi);
    // =========================================================================

    // 图像类型统一（灰度图转彩色图）
    if (croppedImage.type() != CV_8UC3)
    {
        cv::Mat temp;
        cv::cvtColor(croppedImage, temp, cv::COLOR_GRAY2BGR);
        croppedImage = temp;
    }

    // 发射裁剪后图像信号
    emit imgshibie(&croppedImage);

    // 更新图像计数显示
    ui->imagenum->setText(QString::number(totalImages));

    // 获取目标字符串长度
    QString targetString = ui->dateEdit->toPlainText();
    int targetNum = 0;

    // 使用正则表达式解析字库变体
    // 升级正则表达式：允许 [数字、字母、中文] 后面跟带括号的数字作为一个整体
    QRegularExpression regex(R"(([\d[A-Za-z\x{4e00}-\x{9fa5}]\(\d+\))|(\d)|([A-Za-z])|([\x{4e00}-\x{9fa5}]))");
    QRegularExpressionMatchIterator matchIt = regex.globalMatch(targetString);

    while (matchIt.hasNext()) {
    matchIt.next();
    targetNum++; // 每匹配到一个有效字符（数字/英文/中文），目标数 +1
    }

    // 如果正则匹配失败， fallback 到字符串长度（兼容旧逻辑）
    if (targetNum == 0 && !targetString.isEmpty()) {
        targetNum = targetString.length();
    }

    qDebug() << "targetnum" << targetNum << "(parsed from:" << targetString << ")";

    // 运行字库匹配
    int detectNum = templatematch->run3(digitTemplates);
    QString judgeResult = (detectNum == targetNum ? "ok" : "no");


    // ===================== 核心修改：将结果缓存到全局变量，并触发重绘 =====================
    // 1. 清空上一轮的旧数据
    g_lastDrawResults.clear();

    // 2. 将 run3 算出来的最新匹配结果存入全局记忆结构中
    for (const auto& match : templatematch->lastMatchResults) {
        CVDrawResult res;
        res.rect = std::get<0>(match);
        res.score = std::get<1>(match);
        g_lastDrawResults.push_back(res);
    }

    // 3. 记录当前的检测 ROI 区域（用于全图坐标还原）以及时间戳（用于 2 秒后自动消散）
    g_lastRoi = roi;
    g_lastDetectTime = QDateTime::currentMSecsSinceEpoch();

    // 4. 剥夺旧版 imageLabel 自己画框的权力，清理底层虚线框数据，防止重影
    detectedRects.clear();
    imageLabel->clearGreenRects();

    // 5. 强制调用一次界面刷新。
    // 注意：此时传入的是原图指针，新版的 slot_displayAndDetect 内部会执行 clone 并盖上绿框
    slot_displayAndDetect(image);
    // =========================================================================


    // 结果判定与后续逻辑
    if (j % x == 0)
    {
        if (judgeResult == "no")
        {
            if ((ui->comboBox->currentIndex() == 1) || (ui->comboBox->currentIndex() == 3))
            {
                QString saveDir = selectedDir + "/ng/";
                // 此时保存的图像，因为上一步调用了 slot_displayAndDetect，已经完美带有绿框和分数了
                saveImage2("png", saveDir);
            }

            ngImages++;
            totalImages++;
            ui->resultlabel->setText(QString("<font size='10' color='red'>错误！</font>"));

            if (wrongindex == 0)
            {
                qDebug() << "wrongindex=0, calling wrongremove immediately";
                wrongremove();
            }
            else
            {
                qDebug() << "wrongindex=" << wrongindex << ", adding to queue for delayed removal";
                removalQueue.push(std::make_pair(totalImages, totalImages + wrongindex));
            }
        }
        else
        {
            totalImages++;
            if ((ui->comboBox->currentIndex() == 2) || (ui->comboBox->currentIndex() == 3))
            {
                QString saveDir = selectedDir + "/ok/";
                // 此时保存的图像，同样带有绿框和分数
                saveImage2("png", saveDir);
            }
            ui->resultlabel->setText(QString("<font size='10' color='SpringGreen'>正确！</font><br>"));
            rightremove();
        }
    }

    // 更新统计信息
    double hegerate = (1 - static_cast<double>(ngImages) / totalImages) * 100;
    QString str1 = QString::number(hegerate, 'f', 1);
    ui->lineBoxIndex_6->setText(str1);
    ui->ngnum->setText(QString("%1").arg(ngImages));
    ui->imagenum->setText(QString("%1").arg(totalImages));

    // 显示检测耗时
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    ui->speedLabel->setText(QString("检测耗时 %1 毫秒").arg(duration));

    j++;
}

////根据ui缩放版
//void Widget::slot_readAndDetect4(cv::Mat *image, Rect2d diffbox)
//{
//    // 检查延迟剔除队列
//    if (!removalQueue.empty() && totalImages >= removalQueue.front().second - 1)
//    {
//        qDebug() << "wrongindex" << wrongindex;
//        wrongremove();
//        removalQueue.pop();
//    }

//    currentImagesSnapshot = totalImages;
//    auto start = std::chrono::high_resolution_clock::now();

//    if (!image || image->empty())
//    {
//        qDebug() << "Error: Invalid input image.";
//        return;
//    }

//    cv::Mat croppedImage;

//    imageLabel->setColor(color);

//    if (judge)
//    {
//        j = 1;
//        x++;
//        judge = false;
//    }

//    if ((j - 1) % x == 0)
//    {
//        imageLabel->clearGreenRects();
//        detectedRects.clear();
//        string1.clear();
//    }

//    double scaleX = static_cast<double>(ui->image_undetected->width()) / image->cols;
//    double scaleY = static_cast<double>(ui->image_undetected->height()) / image->rows;
//    int offsetX = (ui->image_undetected->width() - scaleX * image->cols) / 2;
//    int offsetY = (ui->image_undetected->height() - scaleY * image->rows) / 2;

//    if (first)
//    {
//        QRect selectionRect1(
//            static_cast<int>(diffbox.x * scaleX + offsetX),
//            static_cast<int>(diffbox.y * scaleY + offsetY),
//            static_cast<int>(diffbox.width * scaleX),
//            static_cast<int>(diffbox.height * scaleY));
//        selectionRect = selectionRect1;
//    }
//    else
//    {
//        QRect selectionRect1(
//            static_cast<int>(diffbox.x),
//            static_cast<int>(diffbox.y),
//            static_cast<int>(diffbox.width),
//            static_cast<int>(diffbox.height));
//        selectionRect = selectionRect1;
//    }

//    if (selectionRect.isNull())
//    {
//        QMessageBox::warning(this, "警告", "没有选择区域！");
//        return;
//    }

//    xRatio = static_cast<double>(image->cols) / imageLabel->width();
//    yRatio = static_cast<double>(image->rows) / imageLabel->height();

//    roi = cv::Rect(
//        static_cast<int>(selectionRect.left() * xRatio),
//        static_cast<int>(selectionRect.top() * yRatio),
//        static_cast<int>(selectionRect.width() * xRatio),
//        static_cast<int>(selectionRect.height() * yRatio));

//    roi &= cv::Rect(0, 0, image->cols, image->rows);

//    if (roi.width <= 0 || roi.height <= 0)
//    {
//        QMessageBox::warning(this, "警告", "选择区域无效！");
//        return;
//    }

//    croppedImage = (*image)(roi);

//    if (croppedImage.type() != CV_8UC3)
//    {
//        cv::Mat temp;
//        cv::cvtColor(croppedImage, temp, cv::COLOR_GRAY2BGR);
//        croppedImage = temp;
//    }

//    emit imgshibie(&croppedImage);

//    ui->imagenum->setText(QString::number(totalImages));

//    // 执行字库匹配
//    QString text = ui->lineEdit_yuzhi->text();
//    int number = text.toDouble();
//    QString totalnum = ui->lineEdit_14->text();
//    int number1 = totalnum.toInt();

//    int detectnum = templatematch->run3(digitTemplates);
//    QString judge = (detectnum == number1 ? "ok" : "no");
//    ui->lineBoxIndex_5->setText(QString::number(detectnum));

//    for (const auto &rect : detectedRects)
//    {
//        imageLabel->addSelectionRect(rect, 2);
//    }
//    imageLabel->update();

//    if (j % x == 0)
//    {
//        if (judge == "no")
//        {
//            if ((ui->comboBox->currentIndex() == 1) || (ui->comboBox->currentIndex() == 3))
//            {
//                QString saveDir = selectedDir + "/ng/";
//                saveImage2("jpg", saveDir);
//            }

//            ngImages++;
//            totalImages++;
//            ui->resultlabel->setText(QString("<font size='10' color='red'>错误！</font>"));

//            if (wrongindex == 0)
//            {
//                qDebug() << "wrongindex=0, calling wrongremove immediately";
//                wrongremove();
//            }
//            else
//            {
//                qDebug() << "wrongindex=" << wrongindex << ", adding to queue for delayed removal";
//                removalQueue.push(std::make_pair(totalImages, totalImages + wrongindex));
//            }
//        }
//        else
//        {
//            totalImages++;
//            if ((ui->comboBox->currentIndex() == 2) || (ui->comboBox->currentIndex() == 3))
//            {
//                QString saveDir = selectedDir + "/ok/";
//                saveImage2("jpg", saveDir);
//            }
//            ui->resultlabel->setText(QString("<font size='10' color='SpringGreen'>正确！</font><br>"));
//            rightremove();
//        }
//    }

//    double hegerate = (1 - static_cast<double>(ngImages) / totalImages) * 100;
//    QString str1 = QString::number(hegerate, 'f', 1);
//    ui->lineBoxIndex_6->setText(str1);
//    ui->ngnum->setText(QString("%1").arg(ngImages));
//    ui->imagenum->setText(QString("%1").arg(totalImages));

//    auto end = std::chrono::high_resolution_clock::now();
//    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
//    ui->speedLabel->setText(QString("检测耗时 %1 毫秒").arg(duration));

//    j++;
//}

/**
 * @brief 软触发拍照按钮点击槽函数
 * @details 发送软触发信号给相机，采集一张图像并进行识别
 */
void Widget::on_VideoShoot_clicked()
{
    if (!m_bOpenDevice)
    {
        QMessageBox::warning(this, "警告", "采集失败,请打开设备！");
        return;
    }

    int exposureValue = ui->spinBox->value();
    qDebug() << "SetExposureTime:" <<exposureValue<<m_pcMyCamera->SetFloatValue("ExposureTime", exposureValue);
    //    QMessageBox::information(this, "提示", "相机曝光设置成功！");

    std::unique_ptr<Mat> image = make_unique<Mat>();


    try {
        // 确保相机处于触发模式
        m_pcMyCamera->SetEnumValue("TriggerMode", 1);     // 触发模式ON
        m_pcMyCamera->SetEnumValue("TriggerSource", 7);   // 软触发源


    } catch (...) {
        QMessageBox::warning(this, "警告", "相机配置失败！");
        return;
    }

    // 发送软触发
    int nRet = m_pcMyCamera->CommandExecute("TriggerSoftware");
    if (MV_OK != nRet)
    {
        QMessageBox::warning(this, "警告", "软触发执行失败！");
        return;
    }


    // 等待图像就绪
    int exposureTime = ui->spinBox->value();
    int waitTime = exposureTime / 1000 + 200; // 曝光时间(us→ms) + 200ms缓冲
    QThread::msleep(waitTime);

    // 获取图像
    *image = m_pcMyCamera->GetImage();

    // 验证图像
    if (image->empty())
    {
        QMessageBox::warning(this, "警告", "未能获取有效图像！请检查相机连接。");
        return;
    }



    // 处理旋转
    int rotationIndex = ui->comboBox_2->currentIndex();
    if (rotationIndex == 1)
    {
        cv::Mat rotatedImg;
        cv::rotate(*image, rotatedImg, cv::ROTATE_90_CLOCKWISE);
        *image = rotatedImg;
        qDebug() << "Image rotated 90° clockwise";
    }
    else if (rotationIndex == 2)
    {
        cv::Mat rotatedImg;
        cv::rotate(*image, rotatedImg, cv::ROTATE_90_COUNTERCLOCKWISE);
        *image = rotatedImg;
        qDebug() << "Image rotated 90° counter-clockwise";
    }
    else if (rotationIndex == 3)
    {
        cv::Mat rotatedImg;
        cv::rotate(*image, rotatedImg, cv::ROTATE_180);
        *image = rotatedImg;
        qDebug() << "Image rotated 180°";
    }

    // 显示和识别
    first = true;
    QRect cvDiffbox = imageLabel->getSelectionRect();
    cv::Rect2d diffbox(cvDiffbox.x(), cvDiffbox.y(),
                       cvDiffbox.width(), cvDiffbox.height());


    slot_displayAndDetect(image.get());
    slot_readAndDetect(image.get(), diffbox);


}

/**
 * @brief 连续拍照按钮点击槽函数
 * @details 启动工作线程，进入连续采集识别模式
 */
//void Widget::on_ReShoot_clicked()
//{
//    qDebug() << "=== on_ReShoot_clicked() called ===";

//    if (!m_bOpenDevice) {
//        QMessageBox::warning(this, "警告", "采集失败,请打开设备！");
//        return;
//    }

//    int exposureValue = ui->spinBox->value();
//    m_pcMyCamera->SetFloatValue("ExposureTime", exposureValue);
//    if((ui->comboBox_4->currentIndex() == 0)||(ui->comboBox_4->currentIndex() == 1))
//    {
//        if (digitTemplates.empty()) {
//            QMessageBox::warning(this, "警告", "模板图像为空！ 请确认目标字符");
//            return;
//        }
//    }



//    // ✅ 核心修复：确保线程已正确初始化
//    ensureThreadsReady();

//    // 如果 myThread 还是 null，重新创建
//    if (!myThread) {
//        reinitializeMyThread();
//    }

//    // 设置参数
//    int number = ui->lineEdit_yuzhi->text().toDouble();
//    emit ssim(number);

//    int index = ui->comboBox_2->currentIndex();
//    switch (index) {
//    case 1: angleValue = 1; break;
//    case 2: angleValue = 2; break;
//    case 3: angleValue = 3; break;
//    default: angleValue = 0;
//    }
//    emit rotate(angleValue);

//    QString text = ui->lineEdit_4->text();
//    emit sendDataTo(text);

//    // 设置为软触发模式
//    m_pcMyCamera->SetEnumValue("TriggerSource", 7);

//    // 传递相机和图像指针给线程
//    myThread->getCameraPtr(m_pcMyCamera);
//    myThread->getImagePtr(myImage);

//    // 启动线程
//    if (!myThread->isRunning()) {
//        myThread->start();
//        ui->statusLabel->setText("软触发模式运行中...");
//    }
//    ui->plcbtn->setEnabled(false);  // 禁用按钮，防止重复点击
//    ui->VideoShoot->setEnabled(false);
//    ui->ReShoot->setEnabled(false);
//    qDebug() << "=== on_ReShoot_clicked() completed ===";
//}

/**
 * @brief 曝光值变化槽函数
 * @param value 新的曝光值
 */
void Widget::onSpinBoxValueChanged(int value)
{
    exposureValue = value;
}

/**
 * @brief 显示主窗口
 */
void Widget::showscreen()
{
    // 读取配置文件
    QSettings *configIniRead = new QSettings("D:\\SystemInifiles\\ConfigName.ini", QSettings::IniFormat);
    configIniRead->setIniCodec("GBK");
    delete configIniRead;

    setWindowIcon(QIcon(":/2.png"));
    setWindowTitle(tr("识别系统"));
    this->show();
}

/**
 * @brief 曝光确定按钮点击槽函数
 * @details 设置相机曝光值
 */
void Widget::on_sureButton_clicked()
{
    if (m_bOpenDevice == false)
    {
        QMessageBox::warning(this, "警告", "未打开相机，无法设置曝光！");
        return;
    }
    else
    {
        int exposureValue = ui->spinBox->value();
        qDebug() << "SetExposureTime:" <<exposureValue<<m_pcMyCamera->SetFloatValue("ExposureTime", exposureValue);
        QMessageBox::information(this, "提示", "相机曝光设置成功！");
    }
}

/**
 * @brief 保存模板图像按钮点击槽函数
 * @details 提取当前选择区域，分割字符并保存为模板
 */
void Widget::on_Saveimage_clicked()
{

    if (ui->image_undetected->pixmap() == nullptr)
    {
        QMessageBox::warning(this, "警告", "保存失败,未采集到图像!");
        return;
    }

    // 获取程序运行目录
    QString currentPath = QDir::currentPath();
    qDebug() << "Current Path: " << currentPath;

    // 删除所有.png文件
    QDir dir(currentPath);
    QFileInfoList files = dir.entryInfoList(QStringList() << "*.png" << "*.PNG", QDir::Files);

    foreach (const QFileInfo &fileInfo, files)
    {
        QString filePath = fileInfo.absoluteFilePath();
        qDebug() << "Found file: " << filePath;

        if (QFile::remove(filePath))
        {
            qDebug() << "Successfully removed: " << filePath;
        }
    }

    // 获取图像处理参数
    bool ok1;
    int width_min = ui->lineEdit_5->text().toInt(&ok1);
    int width_max = ui->lineEdit_9->text().toInt(&ok1);
    int height_min = ui->lineEdit_10->text().toInt(&ok1);
    int height_max = ui->lineEdit_11->text().toInt(&ok1);
    int block_size1 = ui->lineEdit_13->text().toInt(&ok1);
    int kernelsize = ui->lineEdit_15->text().toInt(&ok1);
    int horizontalKernel = ui->lineEdit_18->text().toInt(&ok1);
    int verticalKernel = ui->lineEdit_19->text().toInt(&ok1);

    // 统一判断：是否为有效整数 + 均为大于1的奇数
    bool isParamValid = true;
    // 再判断是否都满足「大于1且是奇数」
    if (block_size1 <= 1 || block_size1 % 2 != 1
            || kernelsize <= 1 || kernelsize % 2 != 1
            || horizontalKernel <= 1 || horizontalKernel % 2 != 1
            || verticalKernel <= 1 || verticalKernel % 2 != 1) {
        isParamValid = false;
    }

    // 统一弹窗警告
    if (!isParamValid) {
        QMessageBox::warning(this, "参数错误", "图像处理参数必须均为大于1的奇数，请修正后重试！");
        return;
    }


    // 发送参数给模板匹配对象
    emit caijianchicun(width_min, width_max, height_min, height_max, block_size1,
                       horizontalKernel, verticalKernel);
    emit kernal(kernelsize);

    // 保存模板图像
    saveImage("bmp", QDir::currentPath() + "/myImage/");

    cv::Mat muban = cv::imread("muban.png");
    templatematch->extractDigits(muban, digitTemplates);

    // 保存分割后的字符图像
    for (size_t i = 0; i < digitTemplates.size(); ++i)
    {
        std::stringstream ss;
        ss << (i + 1) << ".png";
        std::string filename = ss.str();
        cv::imwrite(filename, digitTemplates[i]);
    }
}

/**
 * @brief 获取目标字符串
 * @return QString 目标字符串
 */
QString Widget::setdatetime()
{
    QString datetime = ui->dateEdit->toPlainText();
    return datetime;
}

/**
 * @brief PLC连接按钮点击槽函数
 * @details 连接到西门子PLC
 */
void Widget::on_ConnectpushButton_clicked()
{
    QByteArray ad(ui->lineEdit->text().toUtf8());
    Address = ad.data();

    int tmp = client->ConnectTo(Address, 0, 1);

    if (tmp == 0)
    {
        QMessageBox::information(this, "success", "PLC连接成功");
    }
    else
    {
        QMessageBox::critical(this, "error", "PLC连接失败");
    }
}

/**
 * @brief PLC断开按钮点击槽函数
 */
void Widget::on_DisconnectpushButton_clicked()
{
    int tmp = client->Disconnect();

    if (tmp == 0)
    {
        QMessageBox::information(this, "success", "PLC断开成功");
    }
    else
    {
        QMessageBox::critical(this, "error", "PLC断开失败");
    }
}

/**
 * @brief 写入拍照时间按钮点击槽函数
 * @details 向PLC DB1.924写入DWORD值（拍照时间）
 */
void Widget::on_WriteVDpushButton_clicked()
{
    if (!client->Connected())
    {
        return;
    }

    uint32_t value = ui->lineEdit_6->text().toUInt();
    byte v_data[4] = {0};

    // 大小端转换
    v_data[3] = (unsigned char)(0xFF & value);
    v_data[2] = (unsigned char)((0xFF00 & value) >> 8);
    v_data[1] = (unsigned char)((0xFF0000 & value) >> 16);
    v_data[0] = (unsigned char)((0xFF000000 & value) >> 24);

    // 写入DB1.924
    int tmp = client->WriteArea(S7AreaDB, 1, 924, 4, S7WLDWord, v_data);
    if (tmp != 0)
    {
        QMessageBox::warning(this, "error", "设置失败");
    }
    else
    {
        QMessageBox::information(this, "success", "设置成功");
        qDebug() << "paizhaoshijian" << tmp;
    }
}

/**
 * @brief 写入延时按钮点击槽函数
 * @details 向PLC DB1.920写入DWORD值（延时时间）
 */
void Widget::on_WriteVDpushButton_2_clicked()
{
    if (!client->Connected())
    {
        return;
    }

    uint32_t value2 = ui->lineEdit_7->text().toUInt();
    byte delay_data[4] = {0};

    // 大小端转换
    delay_data[3] = (unsigned char)(0xFF & value2);
    delay_data[2] = (unsigned char)((0xFF00 & value2) >> 8);
    delay_data[1] = (unsigned char)((0xFF0000 & value2) >> 16);
    delay_data[0] = (unsigned char)((0xFF000000 & value2) >> 24);

    // 写入DB1.920
    int tmp2 = client->WriteArea(S7AreaDB, 1, 920, 4, S7WLDWord, delay_data);
    if (tmp2 != 0)
    {
        QMessageBox::warning(this, "error", "设置失败");
    }
    else
    {
        QMessageBox::information(this, "success", "设置成功");
    }
}

/**
 * @brief 写入延时时间按钮点击槽函数
 * @details 向PLC DB1.980写入WORD值（延时时间）
 */
void Widget::on_WriteVDpushButton_3_clicked()
{
    if (!client->Connected())
    {
        return;
    }

    uint16_t value4 = ui->lineEdit_8->text().toUInt();
    byte delay_time[2] = {0};

    // 大小端转换
    delay_time[1] = (unsigned char)(0xFF & value4);
    delay_time[0] = (unsigned char)((0xFF00 & value4) >> 8);

    // 写入DB1.980
    int tmp4 = client->WriteArea(S7AreaDB, 1, 980, 2, S7WLWord, delay_time);
    if (tmp4 != 0)
    {
        QMessageBox::warning(this, "error", "设置失败");
    }
    else
    {
        QMessageBox::information(this, "success", "设置成功");
    }
}

/**
 * @brief 写入批次时间按钮点击槽函数
 * @details 向PLC DB1.982写入WORD值（批次时间）
 */
void Widget::on_pushButton_8_clicked()
{
    if (!client->Connected())
    {
        return;
    }

    uint16_t value5 = ui->lineEdit_20->text().toUInt();
    byte pz_time[2] = {0};

    // 大小端转换
    pz_time[1] = (unsigned char)(0xFF & value5);
    pz_time[0] = (unsigned char)((0xFF00 & value5) >> 8);

    // 写入DB1.982
    int tmp4 = client->WriteArea(S7AreaDB, 1, 982, 2, S7WLWord, pz_time);
    if (tmp4 != 0)
    {
        QMessageBox::warning(this, "error", "设置失败");
    }
    else
    {
        QMessageBox::information(this, "success", "设置成功");
    }
}

/**
 * @brief cv::Mat转换为QImage
 * @param mat 输入的cv::Mat对象
 * @return QImage 转换后的QImage对象
 */
QImage Widget::cvMatToQImage(const cv::Mat &mat)
{
    if (mat.type() == CV_8UC1)
    {
        QImage image(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step),
                     QImage::Format_Grayscale8);
        return image.copy();
    }
    else if (mat.type() == CV_8UC3)
    {
        QImage image(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step),
                     QImage::Format_RGB888);
        return image.rgbSwapped();
    }
    else if (mat.type() == CV_8UC4)
    {
        QImage image(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step),
                     QImage::Format_ARGB32);
        return image.copy();
    }
    else
    {
        qDebug() << "ERROR: Mat could not be converted to QImage.";
        return QImage();
    }
}



void Widget::on_cancel_clicked()
{
    qDebug() << "=== on_cancel_clicked() START ===";

//    // Step 1: 关闭OpenCV窗口
//    try {
//        cv::destroyAllWindows();
//        QThread::msleep(100);
//        cv::waitKey(1);
//    } catch (...) {}

    qDebug()<<"step 1";
    // Step 2: 请求线程停止
    if (myThread) {
        myThread->requestStop();
    }

    qDebug() << "step 2.1";
    if (cameraThread) {
        cameraThread->requestStop();
    }

    qDebug() << "step 2.2";

    // 🔥 Step 3: myThread - 保持原逻辑（不删除，不重启相机）
    bool myThreadWasRunning = false;
    if (myThread && myThread->isRunning()) {
        myThreadWasRunning = true;
        myThread->stop();
        if (!myThread->wait(500)) {
            qDebug() << "WARNING: myThread did not stop";
        }
    }
    qDebug() << "step 3";

    // 🔥 Step 4: cameraThread - 使用旧的plcbtn停止逻辑
    bool needRestartCamera = false;  // 标记是否需要重启相机

    if (cameraThread != nullptr) {
        qDebug() << "Stopping camera thread...";
        needRestartCamera = true;  // cameraThread存在，说明需要重启相机

        // 断开信号槽连接
        disconnect(cameraThread, nullptr, this, nullptr);
        disconnect(this, nullptr, cameraThread, nullptr);

        cameraThread->requestStop();

        // 等待线程结束
        if (!cameraThread->wait(500)) {
            qDebug() << "Camera thread did not stop gracefully, force terminating...";
            cameraThread->terminate();
            cameraThread->wait();
        }

        cameraThread->deleteLater();
        cameraThread = nullptr;
        qDebug() << "Camera thread stopped and scheduled for deletion";
    }

    // 🔥 Step 5: 如果cameraThread运行过，重启相机（和旧plcbtn逻辑一样）
    /*if (needRestartCamera && m_pcMyCamera)*/// 修改后
    if ((needRestartCamera || myThreadWasRunning) && m_pcMyCamera) {
        try {
            qDebug() << "Closing and reopening camera (silent mode)...";

            // 关闭相机
            m_pcMyCamera->Close();
            delete m_pcMyCamera;
            m_pcMyCamera = NULL;
            m_bOpenDevice = false;

            QThread::msleep(100);

            // 🔥 直接重新打开（不弹提示框）
            m_pcMyCamera = new CMvCamera;
            int nRet = m_pcMyCamera->Open(m_stDevList.pDeviceInfo[0]);

            if (MV_OK == nRet) {
                // 设置触发模式
                m_pcMyCamera->SetEnumValue("TriggerMode", 1);
                m_pcMyCamera->SetEnumValue("TriggerSource", 7);    //软触发
                m_pcMyCamera->SetFloatValue("ExposureTime", 500);
                m_pcMyCamera->SetFloatValue("TriggerDelay", 0);
                m_pcMyCamera->RegisterImageCallBack();
                m_pcMyCamera->StartGrabbing();

                m_bOpenDevice = true;
                ui->statusLabel->setText("相机已打开");
                qDebug() << "✓ Camera restarted silently";
            } else {
                delete m_pcMyCamera;
                m_pcMyCamera = nullptr;
                qDebug() << "ERROR: Failed to reopen camera";
            }
        } catch (...) {
            qDebug() << "Exception when restarting camera";
        }
    }

    qDebug()<<"step6";
    // Step 6: 处理事件队列
    QCoreApplication::processEvents(QEventLoop::AllEvents, 1000);

    qDebug()<<"step7";
    // Step 7: 清理UI和变量
    detectedRects.clear();
    selectionRect1 = QRect();

    qDebug()<<"step8";
    if (imageLabel) {
        imageLabel->clearGreenRects();
        imageLabel->setColor(1);
        imageLabel->clearSelection();
//        imageLabel->clear();
    }

    qDebug()<<"step9";
    ui->resultlabel->clear();
    ui->imagenum->clear();
    ui->ngnum->clear();
    ui->resultlabel_7->clear();
    ui->speedLabel->clear();
    ui->lineBoxIndex_6->clear();

    ngImages = 0;
    totalImages = 0;
    first = false;
    x = 1;
    j = 1;
    judge = false;
//    digitTemplates.clear();

    ui->statusLabel->setText("已停止");
    ui->plcbtn->setText("启动");
    ui->plcbtn->setEnabled(true);
    ui->VideoShoot->setEnabled(true);
    ui->pushButton_4->setEnabled(true);
//    ui->ReShoot->setEnabled(true);
    isCollecting = false;

    qDebug() << "=== on_cancel_clicked() COMPLETED ===";
}





bool Widget::isChineseChar(unsigned char c)
{
    return (c & 0x80) != 0;
}

bool Widget::isAlnumOrChinese(char c)
{
    return std::isalnum(static_cast<unsigned char>(c)) || isChineseChar(static_cast<unsigned char>(c));
}

/**
 * @brief 目标字符确定按钮点击槽函数
 */

void Widget::on_textsure_btn_clicked()
{
    if((ui->comboBox_4->currentIndex() == 0)||(ui->comboBox_4->currentIndex() == 1))
    {
        // 1. 检查是否存在有效的模板路径
        if (currentTemplateDirPath.isEmpty()) {
            QMessageBox::information(this, "提示", "请先选择模板文件夹");
            return;
        }
    }

    // 2. 读取当前修改后的目标字符
    QString newMubiaozifu = ui->dateEdit->toPlainText();
    if (newMubiaozifu.isEmpty()) {
        digitTemplates.clear();
        QMessageBox::information(this, "提示", "目标字符为空，已清空模板");
        return;
    }

    // ================== 修复 1：升级正则表达式，加入中文支持 ==================
    QStringList baseNamesToFind;
    // 升级正则表达式：允许 [数字、字母、中文] 后面跟带括号的数字作为一个整体
    QRegularExpression regex(R"(([\d[A-Za-z\x{4e00}-\x{9fa5}]\(\d+\))|(\d)|([A-Za-z])|([\x{4e00}-\x{9fa5}]))");
    QRegularExpressionMatchIterator matchIt = regex.globalMatch(newMubiaozifu);

    while (matchIt.hasNext()) {
        QRegularExpressionMatch match = matchIt.next();
        QString unit;
        if (!match.captured(1).isEmpty()) unit = match.captured(1);
        else if (!match.captured(2).isEmpty()) unit = match.captured(2);
        else if (!match.captured(3).isEmpty()) unit = match.captured(3);
        else if (!match.captured(4).isEmpty()) unit = match.captured(4); // 提取到中文字符

        baseNamesToFind.append(unit.toLower());
    }

    // ================== 修复 2：无视后缀名，建立基础名映射 ==================
    QDir directory(currentTemplateDirPath);
    QMap<QString, QString> filePathMap;
    static const QStringList filters = {"*.jpg", "*.jpeg", "*.png", "*.bmp", "*.tiff"};

    QFileInfoList fileList = directory.entryInfoList(
                filters,
                QDir::Files | QDir::NoDotAndDotDot);

    for (const QFileInfo &fileInfo : fileList) {
        QString baseName = fileInfo.completeBaseName().toLower();
        if (!filePathMap.contains(baseName)) {
            filePathMap.insert(baseName, fileInfo.absoluteFilePath());
        }
    }

    // ================== 修复 3：使用内存流解码解决中文路径 BUG ==================
    std::vector<cv::Mat> tempTemplates;
    bool hasMissing = false;
    QString missingNames;

    for (const QString &searchKey : baseNamesToFind) {
        if (filePathMap.contains(searchKey)) {
            QFile file(filePathMap[searchKey]);
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray data = file.readAll();
                std::vector<uchar> buf(data.begin(), data.end());
                cv::Mat templateImg = cv::imdecode(buf, cv::IMREAD_GRAYSCALE);

                if (templateImg.empty()) {
                    hasMissing = true;
                    missingNames += searchKey + "(读取损坏) ";
                } else {
                    tempTemplates.push_back(templateImg);
                }
            } else {
                hasMissing = true;
                missingNames += searchKey + "(无法打开) ";
            }
        } else {
            hasMissing = true;
            missingNames += searchKey + " ";
        }
    }

    // ================== 修复 4：友好的报警和隔离机制 ==================
    if (hasMissing) {
        // 如果有任何图片读取失败或丢失，绝不更新到全局的 digitTemplates，同时给出严厉警告
        QMessageBox::critical(this, "严重警告",
            "以下字符未在文件夹中找到对应图片，或图片读取失败：\n[ " + missingNames + " ]\n\n请检查模板文件夹内的图片是否存在或是否损坏（支持中文，无需关心后缀和大小写）！\n本次更新已撤销。");
        return;
    }

    // 5. 全部成功后，再更新到全局容器
    digitTemplates = tempTemplates;
    QMessageBox::information(this, "提示", "目标字符确认成功，共加载 " + QString::number(digitTemplates.size()) + " 个模板！");
}





/**
 * @brief QImage转换为cv::Mat指针
 * @param image 输入的QImage对象
 * @return cv::Mat* 转换后的Mat指针
 */
Mat *Widget::QImageToMat(const QImage &image)
{
    cv::Mat *mat = nullptr;
    if (image.isNull())
    {
        qWarning() << "QImage is null";
        return nullptr;
    }

    qDebug() << "QImage format:" << image.format();
    switch (image.format())
    {
    case QImage::Format_RGB32:
    {
        mat = new cv::Mat(image.height(), image.width(), CV_8UC4,
                          const_cast<uchar *>(image.bits()), image.bytesPerLine());
        cv::cvtColor(*mat, *mat, cv::COLOR_BGRA2BGR);
        break;
    }
    case QImage::Format_ARGB32:
    {
        mat = new cv::Mat(image.height(), image.width(), CV_8UC4,
                          const_cast<uchar *>(image.bits()), image.bytesPerLine());
        cv::cvtColor(*mat, *mat, cv::COLOR_BGRA2BGR);
        break;
    }
    case QImage::Format_RGB888:
    {
        mat = new cv::Mat(image.height(), image.width(), CV_8UC3,
                          const_cast<uchar *>(image.bits()), image.bytesPerLine());
        cv::cvtColor(*mat, *mat, cv::COLOR_RGB2BGR);
        break;
    }
    case QImage::Format_Grayscale8:
    {
        mat = new cv::Mat(image.height(), image.width(), CV_8UC1,
                          const_cast<uchar *>(image.bits()), image.bytesPerLine());
        cv::cvtColor(*mat, *mat, cv::COLOR_RGB2BGR);
        break;
    }
    default:
    {
        qWarning() << "QImage format not handled in switch:" << image.format();
        break;
    }
    }

    if (!mat || mat->empty())
    {
        qWarning() << "Failed to convert QImage to cv::Mat";
        return nullptr;
    }
    return mat;
}

/**
 * @brief 延时确定按钮点击槽函数
 * @details 设置相机采集延时
 */
void Widget::on_delayButton_clicked()
{
    QString text = ui->lineEdit_4->text();
    emit sendDataTo(text);
    QMessageBox::information(this, "提示", "相机延时设置成功");
}

/**
 * @brief 清空结果标签槽函数
 */
void Widget::slot_clearResultLabel()
{
    ui->resultlabel_7->clear();
    allResults = "";
}

/**
 * @brief 窗口关闭事件
 * @param event 关闭事件对象
 * @details 关闭时保存设置，销毁所有OpenCV窗口
 */
void Widget::closeEvent(QCloseEvent *event)
{
    cv::destroyAllWindows();
    saveSettings();
    event->accept();
}

/**
 * @brief 图像参数确定按钮点击槽函数
 * @details 设置模板匹配的图像处理参数
 */
void Widget::on_pushButton_clicked()
{
    bool ok1;
    int width_min = ui->lineEdit_5->text().toInt(&ok1);
    int width_max = ui->lineEdit_9->text().toInt(&ok1);
    int height_min = ui->lineEdit_10->text().toInt(&ok1);
    int height_max = ui->lineEdit_11->text().toInt(&ok1);
    int block_size1 = ui->lineEdit_13->text().toInt(&ok1);
    int kernelsize = ui->lineEdit_15->text().toInt(&ok1);
    int horizontalKernel = ui->lineEdit_18->text().toInt(&ok1);
    int verticalKernel = ui->lineEdit_19->text().toInt(&ok1);

    // 统一判断：是否为有效整数 + 均为大于1的奇数
    bool isParamValid = true;
    // 再判断是否都满足「大于1且是奇数」
    if (block_size1 <= 1 || block_size1 % 2 != 1
            || kernelsize <= 1 || kernelsize % 2 != 1
            || horizontalKernel <= 1 || horizontalKernel % 2 != 1
            || verticalKernel <= 1 || verticalKernel % 2 != 1) {
        isParamValid = false;
    }

    // 统一弹窗警告
    if (!isParamValid) {
        QMessageBox::warning(this, "参数错误", "图像处理参数必须均为大于1的奇数，请修正后重试！");
        return;
    }


    emit caijianchicun(width_min, width_max, height_min, height_max, block_size1,
                       horizontalKernel, verticalKernel);
    emit kernal(kernelsize);

    QMessageBox::information(this, "提示", "图像参数设置成功");
}

/**
 * @brief 阈值确定按钮点击槽函数
 * @details 设置相似度判断阈值
 */
void Widget::on_pushButton_3_clicked()
{
    int number = ui->lineEdit_yuzhi->text().toDouble();
    emit ssim(number);
    QMessageBox::information(this, "提示", "阈值设置成功");
}

/**
 * @brief 保存当前图像按钮点击槽函数
 * @details 打开文件保存对话框，保存当前显示的图像
 */
//void Widget::on_pushButton_5_clicked()
//{
//    QString fileName = QFileDialog::getSaveFileName(this, "save image",
//                                                    "C:/Users/Administrator/Desktop/",
//                                                    "JPEG Files (*.jpg);;PNG Files (*.png)");

//    if (fileName.isEmpty())
//    {
//        QMessageBox::information(this, "提示", "未选择保存文件！");
//        return;
//    }

//    QFileInfo fileInfo(fileName);
//    QString format = fileInfo.suffix();
//    QString savePath = fileInfo.path() + "/";

//    saveImage2Async(format, savePath);
//    // 关键：将所有参数保存到用户选择的文件夹中
//    saveSettingsToDir(savePath);

//    // 提示参数已保存
//    QMessageBox::information(this, "提示", "图片和参数已保存到：\n" + savePath);


//}

//无需命名
void Widget::on_pushButton_5_clicked()
{
//    // 只选择文件夹，不涉及文件名
//    QString savePath = QFileDialog::getExistingDirectory(
//        this,
//        "选择保存文件夹",
//        "C:/Users/Administrator/Desktop/",
//        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
//    );

//    if (savePath.isEmpty())
//    {
//        QMessageBox::information(this, "提示", "未选择保存文件夹！");
//        return;
//    }

//    // 确保路径以斜杠结尾
//    if (!savePath.endsWith("/") && !savePath.endsWith("\\"))
//    {
//        savePath += "/";
//    }

    QString parentDir = "D:/muban/";

    bool ok;
    QString newFolderName = QInputDialog::getText(
        this,
        "输入新文件夹名称",
        "请输入要创建的文件夹名称：",
        QLineEdit::Normal,
        "",
        &ok
    );

    if (!ok || newFolderName.isEmpty()) {
        QMessageBox::information(this, "提示", "未输入文件夹名称！");
        return;
    }

    // 安全地拼接路径
    QDir parentPath(parentDir);
    QString savePath = parentPath.absoluteFilePath(newFolderName);

    QDir dir(savePath);

    // 如果文件夹已存在，询问用户是否覆盖
    if (dir.exists()) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "确认覆盖",
                                    "模板 '" + newFolderName + "' 已存在！\n是否覆盖？",
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);

        if (reply == QMessageBox::No) {
            return; // 用户取消操作
        }

        // 使用 Qt 内置的递归删除方法
        if (!QDir(savePath).removeRecursively()) {
            QMessageBox::warning(this, "错误", "无法删除现有模板：" + savePath);
            return;
        }
    }

    // 创建新文件夹
    if (!dir.mkpath(".")) {
        QMessageBox::warning(this, "警告", "创建模板失败：" + savePath);
        return;
    }




    // 后续可以继续使用 savePath 进行操作

    // 固定格式为png
    QString format = "png";

    // 🔥 关键修改：直接使用已保存的框坐标（来自线程信号）
    if (!hasValidBoxes) {
        QMessageBox::warning(this, "警告",
            "当前没有有效的框坐标！\n请先运行采集（点击plcbtn并完成框选择）");
        return;
    }

    qDebug() << "save box:";
    qDebug() << "detectionbox:" << savedDetectionBox.x << savedDetectionBox.y
             << savedDetectionBox.width << savedDetectionBox.height;
    qDebug() << "trackbox:" << savedTrackingBox.x << savedTrackingBox.y
             << savedTrackingBox.width << savedTrackingBox.height;

    // 调用保存函数（文件名由saveImage2Async内部自动生成）
    saveImage2Async(format, savePath);

    currentTemplateDirPath=savePath;

    // 保存参数到该文件夹（包括框坐标）
    saveSettingsToDir(savePath);

    QMessageBox::information(this, "提示",
        "图片和参数已保存到：\n" + savePath);
}


// 先定义一个保存参数到指定文件夹的函数（可放在Widget类中）
void Widget::saveSettingsToDir(const QString &dirPath)
{
    // 确保目标文件夹存在，不存在则创建
    QDir dir(dirPath);
    if (!dir.exists()) {
        dir.mkpath("."); // 创建文件夹（包括多级目录）
    }

    // 配置文件路径：用户选择的文件夹 + "app_settings.appset"
    QString settingsFilePath = dirPath + "/app_settings.appset";
    QSettings settings(settingsFilePath, QSettings::IniFormat); // 强制使用INI格式

    // 保存所有参数（与原逻辑一致，只是路径改为指定文件夹）
    settings.setValue("spinbox_value", ui->spinBox->text());
    settings.setValue("lineEdit_6_value", ui->lineEdit_6->text());
    settings.setValue("lineEdit_7_value", ui->lineEdit_7->text());
    settings.setValue("lineEdit_8_value", ui->lineEdit_8->text());
    settings.setValue("lineEdit_20_value", ui->lineEdit_20->text());
    settings.setValue("lineEdit_12_value", ui->lineEdit_12->text());
    settings.setValue("lineEdit_4_value", ui->lineEdit_4->text());
    settings.setValue("lineEdit_13_value", ui->lineEdit_13->text());
    settings.setValue("lineEdit_15_value", ui->lineEdit_15->text());
    settings.setValue("lineEdit_18_value", ui->lineEdit_18->text());
    settings.setValue("lineEdit_19_value", ui->lineEdit_19->text());
    settings.setValue("lineEdit_yuzhi_value", ui->lineEdit_yuzhi->text());
    settings.setValue("dateEdit_value", ui->dateEdit->toPlainText());
    settings.setValue("comboBox_value", ui->comboBox->currentText());
    settings.setValue("comboBox_2_value", ui->comboBox_2->currentText());
    settings.setValue("comboBox_3_value", ui->comboBox_3->currentText());
    settings.setValue("comboBox_4_value", ui->comboBox_4->currentText());
    // 新增：保存模板路径
    settings.setValue("saveDirPath", selectedDir);
    settings.setValue("TemplateDirPath", currentTemplateDirPath);


    // 🔥 新增：保存框坐标
    if (hasValidBoxes) {
        settings.setValue("detectionBox_x", savedDetectionBox.x);
        settings.setValue("detectionBox_y", savedDetectionBox.y);
        settings.setValue("detectionBox_width", savedDetectionBox.width);
        settings.setValue("detectionBox_height", savedDetectionBox.height);

        settings.setValue("trackingBox_x", savedTrackingBox.x);
        settings.setValue("trackingBox_y", savedTrackingBox.y);
        settings.setValue("trackingBox_width", savedTrackingBox.width);
        settings.setValue("trackingBox_height", savedTrackingBox.height);

        settings.setValue("hasValidBoxes", true);

    } else {
        settings.setValue("hasValidBoxes", false);
    }
}


/**
 * @brief 加载字库按钮点击槽函数
 * @details 支持带括号的字符格式，如"0(1)"表示0字符的第1个变体
 */
// 按钮pushButton_4的点击事件槽函数
// 功能：从用户输入解析模板文件名，选择模板文件夹
void Widget::on_pushButton_4_clicked()
{
    QString dirPath = QFileDialog::getExistingDirectory(nullptr, "选择模板文件夹",
                                                        "D:/muban",
                                                        QFileDialog::ShowDirsOnly);
    if (dirPath.isEmpty()) return;

    currentTemplateDirPath = dirPath;

    saveSettings(); // 保存路径
    loadSettingsFromDir(dirPath);
    wrongindex = ui->lineEdit_12->text().toInt();

    qDebug()<<"currentTemplate"<<currentTemplateDirPath;

    //设置PLC参数
    //判断plc是否连接
    if (!client->Connected())
    {
        return;
    }

    uint32_t value = ui->lineEdit_6->text().toUInt();
    byte v_data[4] = {0};

    // 大小端转换
    v_data[3] = (unsigned char)(0xFF & value);
    v_data[2] = (unsigned char)((0xFF00 & value) >> 8);
    v_data[1] = (unsigned char)((0xFF0000 & value) >> 16);
    v_data[0] = (unsigned char)((0xFF000000 & value) >> 24);

    // 写入DB1.924
    int tmp = client->WriteArea(S7AreaDB, 1, 924, 4, S7WLDWord, v_data);
    if (tmp != 0)
    {
        QMessageBox::warning(this, "error", "设置失败");
    }


    uint32_t value2 = ui->lineEdit_7->text().toUInt();
    byte delay_data[4] = {0};

    // 大小端转换
    delay_data[3] = (unsigned char)(0xFF & value2);
    delay_data[2] = (unsigned char)((0xFF00 & value2) >> 8);
    delay_data[1] = (unsigned char)((0xFF0000 & value2) >> 16);
    delay_data[0] = (unsigned char)((0xFF000000 & value2) >> 24);

    // 写入DB1.920
    int tmp2 = client->WriteArea(S7AreaDB, 1, 920, 4, S7WLDWord, delay_data);
    if (tmp2 != 0)
    {
        QMessageBox::warning(this, "error", "设置失败");
    }

    uint16_t value4 = ui->lineEdit_8->text().toUInt();
    byte delay_time[2] = {0};

    // 大小端转换
    delay_time[1] = (unsigned char)(0xFF & value4);
    delay_time[0] = (unsigned char)((0xFF00 & value4) >> 8);

    // 写入DB1.980
    int tmp4 = client->WriteArea(S7AreaDB, 1, 980, 2, S7WLWord, delay_time);
    if (tmp4 != 0)
    {
        QMessageBox::warning(this, "error", "设置失败");
    }


    uint16_t value5 = ui->lineEdit_20->text().toUInt();
    byte pz_time[2] = {0};

    // 大小端转换
    pz_time[1] = (unsigned char)(0xFF & value5);
    pz_time[0] = (unsigned char)((0xFF00 & value5) >> 8);

    // 写入DB1.982
    int tmp5 = client->WriteArea(S7AreaDB, 1, 982, 2, S7WLWord, pz_time);
    if (tmp5 != 0)
    {
        QMessageBox::warning(this, "error", "设置失败");
    }



    QMessageBox::information(this, "提示", "模板已选择");
}

/**
 * @brief 选择保存文件夹按钮点击槽函数
 */
void Widget::on_pushButton_6_clicked()
{
    selectedDir = QFileDialog::getExistingDirectory(
                this,
                "选择目标文件夹",
                "C:/",
                QFileDialog::ShowDirsOnly);
    qDebug() << "save file path:" << selectedDir;
}



/**
 * @brief 清空总数统计按钮点击槽函数
 */
void Widget::on_cut_cancelButton_2_clicked()
{
    totalImages = 0;
    ngImages = 0;
    ui->ngnum->setText(QString("%1").arg(ngImages));
    ui->imagenum->setText(QString("%1").arg(totalImages));
}

/**
 * @brief 清空NG数统计按钮点击槽函数
 */
void Widget::on_cut_cancelButton_3_clicked()
{
    ngImages = 0;
    ui->ngnum->setText(QString("%1").arg(ngImages));
}

/**
 * @brief 旋转角度确定按钮点击槽函数
 * @details 设置图像旋转角度（0°、90°、180°、270°）
 */
void Widget::on_pushButton_9_clicked()
{
    int index = ui->comboBox_2->currentIndex();
    switch (index)
    {
    case 1:
        angleValue = 1;
        break;
    case 2:
        angleValue = 2;
        break;
    case 3:
        angleValue = 3;
        break;
    default:
        angleValue = 0;
    }

    emit rotate(angleValue);
    QMessageBox::information(this, "提示", "旋转角度设置成功");
}



void Widget::loadSettingsFromDir(const QString &dirPath)
{
    // 配置文件路径：用户选择的文件夹 + "app_settings.ini"
    QString settingsFilePath = dirPath + "/app_settings.appset";
    QSettings settings(settingsFilePath, QSettings::IniFormat); // 对应保存时的INI格式

    // 以下逻辑与原loadSettings完全一致，只是读取路径改为指定文件夹
    if (settings.contains("spinbox_value"))
        ui->spinBox->setValue(settings.value("spinbox_value").toInt());

    if (settings.contains("lineEdit_6_value"))
        ui->lineEdit_6->setText(settings.value("lineEdit_6_value").toString());

    if (settings.contains("lineEdit_7_value"))
        ui->lineEdit_7->setText(settings.value("lineEdit_7_value").toString());

    if (settings.contains("lineEdit_8_value"))
        ui->lineEdit_8->setText(settings.value("lineEdit_8_value").toString());

    if (settings.contains("lineEdit_20_value"))
        ui->lineEdit_20->setText(settings.value("lineEdit_20_value").toString());

    if (settings.contains("lineEdit_12_value"))
        ui->lineEdit_12->setText(settings.value("lineEdit_12_value").toString());

    if (settings.contains("lineEdit_4_value"))
        ui->lineEdit_4->setText(settings.value("lineEdit_4_value").toString());

    if (settings.contains("lineEdit_13_value"))
        ui->lineEdit_13->setText(settings.value("lineEdit_13_value").toString());

    if (settings.contains("lineEdit_15_value"))
        ui->lineEdit_15->setText(settings.value("lineEdit_15_value").toString());

    if (settings.contains("lineEdit_18_value"))
        ui->lineEdit_18->setText(settings.value("lineEdit_18_value").toString());

    if (settings.contains("lineEdit_19_value"))
        ui->lineEdit_19->setText(settings.value("lineEdit_19_value").toString());

    if (settings.contains("lineEdit_yuzhi_value"))
        ui->lineEdit_yuzhi->setText(settings.value("lineEdit_yuzhi_value").toString());

    if (settings.contains("dateEdit_value")) {
        ui->dateEdit->setPlainText(settings.value("dateEdit_value").toString());
    }

    if (settings.contains("comboBox_value"))
    {
        QString value = settings.value("comboBox_value").toString();
        int index = ui->comboBox->findText(value);
        if (index >= 0)
            ui->comboBox->setCurrentIndex(index);
    }

    if (settings.contains("comboBox_2_value"))
    {
        QString value = settings.value("comboBox_2_value").toString();
        int index = ui->comboBox_2->findText(value);
        if (index >= 0)
            ui->comboBox_2->setCurrentIndex(index);
    }

    if (settings.contains("comboBox_3_value"))
    {
        QString value1 = settings.value("comboBox_3_value").toString();
        int index = ui->comboBox_3->findText(value1);
        if (index >= 0)
            ui->comboBox_3->setCurrentIndex(index);
    }

    if (settings.contains("comboBox_4_value"))
    {
        QString value2 = settings.value("comboBox_4_value").toString();
        int index = ui->comboBox_4->findText(value2);
        if (index >= 0)
            ui->comboBox_4->setCurrentIndex(index);
    }

    if (settings.contains("TemplateDirPath")) {
        currentTemplateDirPath = settings.value("TemplateDirPath").toString();
    }

    if (settings.contains("saveDirPath")) {
        selectedDir = settings.value("saveDirPath").toString();
    }


    // 🔥 新增：加载框坐标
       if (settings.contains("hasValidBoxes") && settings.value("hasValidBoxes").toBool()) {
           savedDetectionBox.x = settings.value("detectionBox_x", 0).toDouble();
           savedDetectionBox.y = settings.value("detectionBox_y", 0).toDouble();
           savedDetectionBox.width = settings.value("detectionBox_width", 0).toDouble();
           savedDetectionBox.height = settings.value("detectionBox_height", 0).toDouble();

           savedTrackingBox.x = settings.value("trackingBox_x", 0).toDouble();
           savedTrackingBox.y = settings.value("trackingBox_y", 0).toDouble();
           savedTrackingBox.width = settings.value("trackingBox_width", 0).toDouble();
           savedTrackingBox.height = settings.value("trackingBox_height", 0).toDouble();

           hasValidBoxes = true;
           qDebug() << "box load success";
           qDebug() << "detectionbox:" << savedDetectionBox.x << savedDetectionBox.y
                    << savedDetectionBox.width << savedDetectionBox.height;
           qDebug() << "trackbox:" << savedTrackingBox.x << savedTrackingBox.y
                    << savedTrackingBox.width << savedTrackingBox.height;
       } else {
           hasValidBoxes = false;
           qDebug() << "no usesful box";
       }
}


/**
 * @brief 加载设置
 * @details 从QSettings加载所有参数设置
 */
void Widget::loadSettings()
{
    QSettings settings("YourCompany", "YourApplication");

    if (settings.contains("spinbox_value"))
        ui->spinBox->setValue(settings.value("spinbox_value").toInt());

    if (settings.contains("lineEdit_6_value"))
        ui->lineEdit_6->setText(settings.value("lineEdit_6_value").toString());

    if (settings.contains("lineEdit_7_value"))
        ui->lineEdit_7->setText(settings.value("lineEdit_7_value").toString());

    if (settings.contains("lineEdit_8_value"))
        ui->lineEdit_8->setText(settings.value("lineEdit_8_value").toString());

    if (settings.contains("lineEdit_20_value"))
        ui->lineEdit_20->setText(settings.value("lineEdit_20_value").toString());

    if (settings.contains("lineEdit_12_value"))
        ui->lineEdit_12->setText(settings.value("lineEdit_12_value").toString());

    if (settings.contains("lineEdit_4_value"))
        ui->lineEdit_4->setText(settings.value("lineEdit_4_value").toString());

    if (settings.contains("lineEdit_13_value"))
        ui->lineEdit_13->setText(settings.value("lineEdit_13_value").toString());

    if (settings.contains("lineEdit_15_value"))
        ui->lineEdit_15->setText(settings.value("lineEdit_15_value").toString());

    if (settings.contains("lineEdit_18_value"))
        ui->lineEdit_18->setText(settings.value("lineEdit_18_value").toString());

    if (settings.contains("lineEdit_19_value"))
        ui->lineEdit_19->setText(settings.value("lineEdit_19_value").toString());

    if (settings.contains("lineEdit_yuzhi_value"))
        ui->lineEdit_yuzhi->setText(settings.value("lineEdit_yuzhi_value").toString());

    if (settings.contains("dateEdit_value"))
        ui->dateEdit->setPlainText(settings.value("dateEdit_value").toString());

    if (settings.contains("comboBox_value"))
    {
        QString value = settings.value("comboBox_value").toString();
        int index = ui->comboBox->findText(value);
        if (index >= 0)
            ui->comboBox->setCurrentIndex(index);
    }

    if (settings.contains("comboBox_2_value"))
    {
        QString value = settings.value("comboBox_2_value").toString();
        int index = ui->comboBox_2->findText(value);
        if (index >= 0)
            ui->comboBox_2->setCurrentIndex(index);
    }

    if (settings.contains("comboBox_3_value"))
    {
        QString value1 = settings.value("comboBox_3_value").toString();
        int index = ui->comboBox_3->findText(value1);
        if (index >= 0)
            ui->comboBox_3->setCurrentIndex(index);
    }

    if (settings.contains("comboBox_4_value"))
    {
        QString value2 = settings.value("comboBox_4_value").toString();
        int index = ui->comboBox_4->findText(value2);
        if (index >= 0)
            ui->comboBox_4->setCurrentIndex(index);
    }

    if (settings.contains("comboBox_5_value"))
    {
        QString value3 = settings.value("comboBox_5_value").toString();
        int index = ui->comboBox_5->findText(value3);
        if (index >= 0)
            ui->comboBox_5->setCurrentIndex(index);
    }

    if (settings.contains("TemplateDirPath")) {
        currentTemplateDirPath = settings.value("TemplateDirPath").toString();
    }

    if (settings.contains("saveDirPath")) {
        selectedDir = settings.value("saveDirPath").toString();
    }
}

/**
 * @brief 保存设置
 * @details 保存所有参数设置到QSettings
 */
void Widget::saveSettings()
{
    QSettings settings("YourCompany", "YourApplication");

    settings.setValue("spinbox_value", ui->spinBox->text());
    settings.setValue("lineEdit_6_value", ui->lineEdit_6->text());
    settings.setValue("lineEdit_7_value", ui->lineEdit_7->text());
    settings.setValue("lineEdit_8_value", ui->lineEdit_8->text());
    settings.setValue("lineEdit_20_value", ui->lineEdit_20->text());
    settings.setValue("lineEdit_12_value", ui->lineEdit_12->text());
    settings.setValue("lineEdit_4_value", ui->lineEdit_4->text());
    settings.setValue("lineEdit_13_value", ui->lineEdit_13->text());
    settings.setValue("lineEdit_15_value", ui->lineEdit_15->text());
    settings.setValue("lineEdit_18_value", ui->lineEdit_18->text());
    settings.setValue("lineEdit_19_value", ui->lineEdit_19->text());
    settings.setValue("lineEdit_yuzhi_value", ui->lineEdit_yuzhi->text());
//    settings.setValue("dateEdit_value", ui->dateEdit->toPlainText());
    settings.setValue("comboBox_value", ui->comboBox->currentText());
    settings.setValue("comboBox_2_value", ui->comboBox_2->currentText());
    settings.setValue("comboBox_3_value", ui->comboBox_3->currentText());
    settings.setValue("comboBox_4_value", ui->comboBox_4->currentText());
    settings.setValue("comboBox_5_value", ui->comboBox_5->currentText());
    // 新增：保存模板路径
    settings.setValue("saveDirPath", selectedDir);
    settings.setValue("TemplateDirPath", currentTemplateDirPath);
}

/**
 * @brief 设置默认值
 * @details 为所有UI控件设置初始默认值
 */
void Widget::setupDefaultValues()
{
    ui->lineEdit_6->setText("50");
    ui->lineEdit_7->setText("300");
    ui->lineEdit_8->setText("500");
    ui->lineEdit_20->setText("500");
    ui->lineEdit_18->setText("1");
    ui->lineEdit_19->setText("3");
    ui->lineEdit_12->setText("0");
    ui->lineEdit_4->setText("300");
    ui->lineEdit_13->setText("11");
    ui->lineEdit_15->setText("3");
    ui->lineEdit_yuzhi->setText("70");
    ui->dateEdit->setPlainText("");
    ui->spinBox->setValue(800);
    ui->comboBox->setCurrentText("不保存图像");
    ui->comboBox_4->setCurrentText("字库匹配");
    ui->comboBox_2->setCurrentText("无旋转");
    ui->comboBox_3->setCurrentText("间歇触发模式");
    ui->checkBox->setChecked(true);
}

//关闭相机按钮
void Widget::on_CloseCamera_clicked()
{
    if (myThread->isRunning())
    {
        myThread->requestInterruption();
        myThread->wait();
        myThread->stop();
    }
    if (m_pcMyCamera)
    {
        m_pcMyCamera->Close();
        delete m_pcMyCamera;
        m_pcMyCamera = NULL;
        m_bOpenDevice = false;
    }
    // 清空文本并将文本置0
    ui->resultlabel->clear();
    imageLabel->clear();
    ui->image_undetected->clear();
    ui->imagenum->clear();
    ui->ngnum->clear();
    //    ui->ocrResult->clear();
    ui->resultlabel_7->clear();
    ui->speedLabel->clear();
    ngImages = 0;
    totalImages = 0;
    //    qDebug()<<"totaltime"<<totalTime<<"s";
    //    totalTime=0;
    // 标记相机关闭状态
    m_bOpenDevice = false;
    ui->statusLabel->setText("相机已关闭");
    ui->statusLabel->setStyleSheet("QLabel{color:#e74c3c; font-weight:bold;}");
}


void Widget::on_plcbtn_clicked()
{
    qDebug() << "=== on_plcbtn_clicked() START ===";

    if (!m_bOpenDevice)
    {
        QMessageBox::warning(this, "警告", "采集失败,请打开设备！");
        return;
    }

    // 🔥 检查预设框状态
    if (hasValidBoxes) {
        qDebug() << "检测到预设框，将使用保存的框坐标";
        qDebug() << "检测框:" << savedDetectionBox.x << savedDetectionBox.y
                 << savedDetectionBox.width << savedDetectionBox.height;
        qDebug() << "跟踪框:" << savedTrackingBox.x << savedTrackingBox.y
                 << savedTrackingBox.width << savedTrackingBox.height;
    } else {
        qDebug() << "未检测到预设框，将使用手动选框模式";
    }

    if (ui->checkBox->isChecked())
    {
        // ========================================
        // 执行原有的 plcbtn 逻辑（外部触发模式）
        // ========================================

        // 第一段核心逻辑：单次采集+图像处理
        int exposureValue = ui->spinBox->value();
        qDebug() << "SetExposureTime:" << exposureValue << m_pcMyCamera->SetFloatValue("ExposureTime", exposureValue);

        std::unique_ptr<Mat> image = make_unique<Mat>();

        try {
            m_pcMyCamera->SetEnumValue("TriggerMode", 1);
            m_pcMyCamera->SetEnumValue("TriggerSource", 7);
        } catch (...) {
            QMessageBox::warning(this, "警告", "相机配置失败！");
            return;
        }

        int nRet = m_pcMyCamera->CommandExecute("TriggerSoftware");
        if (MV_OK != nRet)
        {
            QMessageBox::warning(this, "警告", "软触发执行失败！");
            return;
        }

        int waitTime = exposureValue / 1000 + 200;
        QThread::msleep(waitTime);

        *image = m_pcMyCamera->GetImage();
        if (image->empty())
        {
            QMessageBox::warning(this, "警告", "未能获取有效图像！请检查相机连接。");
            return;
        }

        // 图像旋转处理
        int rotationIndex = ui->comboBox_2->currentIndex();
        if (rotationIndex == 1)
        {
            cv::Mat rotatedImg;
            cv::rotate(*image, rotatedImg, cv::ROTATE_90_CLOCKWISE);
            *image = rotatedImg;
            qDebug() << "Image rotated 90° clockwise";
        }
        else if (rotationIndex == 2)
        {
            cv::Mat rotatedImg;
            cv::rotate(*image, rotatedImg, cv::ROTATE_90_COUNTERCLOCKWISE);
            *image = rotatedImg;
            qDebug() << "Image rotated 90° counter-clockwise";
        }
        else if (rotationIndex == 3)
        {
            cv::Mat rotatedImg;
            cv::rotate(*image, rotatedImg, cv::ROTATE_180);
            *image = rotatedImg;
            qDebug() << "Image rotated 180°";
        }

//        // 检测模板图像是否为空
//        if((ui->comboBox_4->currentIndex() == 0)||(ui->comboBox_4->currentIndex() == 1))
//        {
//            if (digitTemplates.empty()) {
//                QMessageBox::warning(this, "警告", "模板图像为空！ 请确认目标字符");
//                return;
//            }
//        }

        if (isCollecting)
        {
            QMessageBox::information(this, "提示", "已在采集中，若要停止请点击【取消识别】按钮");
            return;
        }

        qDebug() << "Starting external trigger collection...";

        // 清空UI显示
        j = 1;
        ui->image_undetected->clear();
        ui->imagenum->clear();
        ui->ngnum->clear();
        ui->resultlabel_7->clear();
        ui->speedLabel->clear();
        ngImages = 0;
        totalImages = 0;

        // 重置相机状态
        if (m_pcMyCamera)
        {
            try {
                qDebug() << "Resetting camera to clean state...";

                m_pcMyCamera->StopGrabbing();
                QThread::msleep(200);

                m_pcMyCamera->SetEnumValue("TriggerMode", 1);
                m_pcMyCamera->SetEnumValue("TriggerSource", 0);
                m_pcMyCamera->SetFloatValue("ExposureTime", exposureValue);
                m_pcMyCamera->SetFloatValue("TriggerDelay", 0);

                m_pcMyCamera->RegisterImageCallBack();
                m_pcMyCamera->StartGrabbing();
                QThread::msleep(100);

                qDebug() << "✓ Camera reset and ready";
            } catch (...) {
                QMessageBox::critical(this, "错误", "相机初始化失败！请重新打开相机。");
                return;
            }
        }

        // 清理旧的cameraThread
        if (cameraThread)
        {
            qDebug() << "Old cameraThread exists, cleaning up...";

            if (cameraThread->isRunning())
            {
                cameraThread->requestStop();

                try {
                    cv::destroyAllWindows();
                    cv::waitKey(1);
                } catch (...) {}

                if (!cameraThread->wait(3000))
                {
                    qDebug() << "WARNING: Old cameraThread timeout";
                }
            }

            disconnect(cameraThread, nullptr, this, nullptr);
            delete cameraThread;
            cameraThread = nullptr;

            QThread::msleep(200);
            QCoreApplication::processEvents();
            qDebug() << "✓ Old cameraThread cleaned up";
        }

        // 创建新的cameraThread
        qDebug() << "Creating new cameraThread...";
            cameraThread = new CameraThread(this, m_pcMyCamera);

            // 🔥 关键修改：如果有预设框，传递给cameraThread
            if (hasValidBoxes) {
                cameraThread->setPresetBoxes(savedDetectionBox, savedTrackingBox);
                qDebug() << "预设框已传递给cameraThread";
            } else {
                cameraThread->clearPresetBoxes();
                qDebug() << "cameraThread将使用手动选框模式";
            }

            // 连接信号槽

            connect(this, &Widget::rotate, cameraThread, &CameraThread::receiveangle1);
            connect(this, &Widget::choosechannel,cameraThread,&CameraThread::receivecolorchannel);
            connect(cameraThread, &CameraThread::signal_cleanlabel,
                    this, &Widget::slot_clearResultLabel, Qt::QueuedConnection);
            connect(cameraThread, &CameraThread::signal_messImage, this, [this](cv::Mat img) {
                this->slot_displayAndDetect(&img);
            }, Qt::QueuedConnection);
            connect(cameraThread, &CameraThread::signal_boxesSelected,
                    this, &Widget::slot_saveBoxesFromThread, Qt::QueuedConnection);
            connect(cameraThread, &CameraThread::signal_sendForDetection, this, [this](cv::Mat img, Rect2d rect) {
                if (!img.empty()) {
                    if (ui->comboBox_4->currentIndex() == 2) {
                        this->slot_readAndDetect(&img, rect);
                    } else if (ui->comboBox_4->currentIndex() == 0) {
                        this->slot_readAndDetect3(&img, rect);
                    } else if (ui->comboBox_4->currentIndex() == 1) {
                        this->slot_readAndDetect4(&img, rect);
                    }
                }
            }, Qt::QueuedConnection);
        // 发送参数
        int index = ui->comboBox_2->currentIndex();
        switch (index)
        {
        case 1: angleValue = 1; break;
        case 2: angleValue = 2; break;
        case 3: angleValue = 3; break;
        default: angleValue = 0;
        }
        emit rotate(angleValue);

        int index1 = ui->comboBox_5->currentIndex();
        switch (index1)
        {
        case 1:
             colorchannel= 1;
            break;
        case 2:
            colorchannel = 2;
            break;
        case 3:
            colorchannel = 3;
            break;
        default:
            colorchannel = 0;
        }

        emit choosechannel(colorchannel);

        QString text = ui->lineEdit_4->text();
        emit sendDataTo(text);

        QString input = ui->dateEdit->toPlainText();
        String targetstring1 = input.toStdString();
        emit jiancestring(targetstring1);

        // 图像处理参数
        bool ok1;
        int width_min = ui->lineEdit_5->text().toInt(&ok1);
        int width_max = ui->lineEdit_9->text().toInt(&ok1);
        int height_min = ui->lineEdit_10->text().toInt(&ok1);
        int height_max = ui->lineEdit_11->text().toInt(&ok1);
        int block_size1 = ui->lineEdit_13->text().toInt(&ok1);
        int kernelsize = ui->lineEdit_15->text().toInt(&ok1);
        int horizontalKernel = ui->lineEdit_18->text().toInt(&ok1);
        int verticalKernel = ui->lineEdit_19->text().toInt(&ok1);

        bool isParamValid = true;
        if (block_size1 <= 1 || block_size1 % 2 != 1
                || kernelsize <= 1 || kernelsize % 2 != 1
                || horizontalKernel <= 1 || horizontalKernel % 2 != 1
                || verticalKernel <= 1 || verticalKernel % 2 != 1) {
            isParamValid = false;
        }

        if (!isParamValid) {
            QMessageBox::warning(this, "参数错误", "图像处理参数必须均为大于1的奇数，请修正后重试！");
            return;
        }

        emit caijianchicun(width_min, width_max, height_min, height_max, block_size1,
                           horizontalKernel, verticalKernel);
        emit kernal(kernelsize);

        int number = ui->lineEdit_yuzhi->text().toDouble();
        emit ssim(number);

        // 启动线程
        qDebug() << "Starting cameraThread...";
        cameraThread->start();

        if (!cameraThread->wait(100))
        {
            qDebug() << "✓ CameraThread started successfully";
            isCollecting = true;
            QDir templateDir(currentTemplateDirPath);
            QString lastFolderName = templateDir.dirName(); // 关键：提取路径的最后一级文件夹名

            // 2. 拼接文本并设置到 statusLabel
            QString statusText = QString("触发模式运行中\n产品模板：%1").arg(lastFolderName);
            ui->statusLabel->setText(statusText);

            ui->plcbtn->setText("采集中...");
            ui->plcbtn->setEnabled(false);
            ui->VideoShoot->setEnabled(false);
            ui->pushButton_4->setEnabled(false);
        }
        else
        {
            qDebug() << "ERROR: CameraThread exited immediately";
            QMessageBox::critical(this, "错误", "相机线程启动失败！");
            isCollecting = false;
            ui->plcbtn->setText("触发采集");
            ui->plcbtn->setEnabled(true);
        }
    }
    else
    {
        // ========================================
        // 执行 ReShoot 逻辑（软触发模式）
        // ========================================

        qDebug() << "=== Executing ReShoot logic ===";

        int exposureValue = ui->spinBox->value();
        m_pcMyCamera->SetFloatValue("ExposureTime", exposureValue);

//        if((ui->comboBox_4->currentIndex() == 0)||(ui->comboBox_4->currentIndex() == 1))
//        {
//            if (digitTemplates.empty()) {
//                QMessageBox::warning(this, "警告", "模板图像为空! 请确认目标字符");
//                return;
//            }
//        }

        ensureThreadsReady();

                if (!myThread) {
                    reinitializeMyThread();
                }

                // 🔥 关键修改：如果有预设框，传递给myThread
                if (hasValidBoxes) {
                    myThread->setPresetBoxes(savedDetectionBox, savedTrackingBox);
                    qDebug() << "预设框已传递给myThread";
                } else {
                    myThread->clearPresetBoxes();
                    qDebug() << "myThread将使用手动选框模式";
                }
                connect(myThread, &MyThread::signal_boxesSelected,
                        this, &Widget::slot_saveBoxesFromThread, Qt::QueuedConnection);


        // 设置参数
        int number = ui->lineEdit_yuzhi->text().toDouble();
        emit ssim(number);

        int index = ui->comboBox_2->currentIndex();
        switch (index) {
        case 1: angleValue = 1; break;
        case 2: angleValue = 2; break;
        case 3: angleValue = 3; break;
        default: angleValue = 0;
        }
        emit rotate(angleValue);

        int index1 = ui->comboBox_5->currentIndex();
        switch (index1)
        {
        case 1:
             colorchannel= 1;
            break;
        case 2:
            colorchannel = 2;
            break;
        case 3:
            colorchannel = 3;
            break;
        default:
            colorchannel = 0;
        }

        emit choosechannel(colorchannel);

        QString text = ui->lineEdit_4->text();
        emit sendDataTo(text);

        m_pcMyCamera->SetEnumValue("TriggerSource", 7);

        myThread->getCameraPtr(m_pcMyCamera);
        myThread->getImagePtr(myImage);

        if (!myThread->isRunning()) {
            myThread->start();

       // 1. 解析 currentTemplateDirPath，获取最后一级文件夹名称
        QDir templateDir(currentTemplateDirPath);
        QString lastFolderName = templateDir.dirName(); // 关键：提取路径的最后一级文件夹名

        // 2. 拼接文本并设置到 statusLabel
        QString statusText = QString("软触发模式运行中\n产品模板：%1").arg(lastFolderName);
        ui->statusLabel->setText(statusText);

        ui->plcbtn->setEnabled(false);
        ui->VideoShoot->setEnabled(false);
        ui->pushButton_4->setEnabled(false);

        qDebug() << "=== ReShoot logic completed ===";
    }

    qDebug() << "=== on_plcbtn_clicked() COMPLETED ===";
}
}


// 检测相机
void Widget::on_HandwareDetect_clicked()
{
    if (m_bOpenDevice)
    {
        QMessageBox::warning(this, "警告", "相机已连接！");
        return;
    }

    // 查找设备
    memset(&m_stDevList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
    int nRet = CMvCamera::EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &m_stDevList);
    if (MV_OK != nRet || m_stDevList.nDeviceNum == 0)
    {
        QMessageBox::warning(this, "警告", "未找到相机设备！");
        return;
    }

    // 打开设备
    m_pcMyCamera = new CMvCamera;
    if (m_pcMyCamera == nullptr)
    {
        return;
    }

    // 假设只有一个相机，直接打开第一个设备
    int nIndex = 0;
    nRet = m_pcMyCamera->Open(m_stDevList.pDeviceInfo[nIndex]);
    //    qDebug() << "Connect:" << nRet;
    if (MV_OK != nRet)
    {
        delete m_pcMyCamera;
        m_pcMyCamera = nullptr;
        QMessageBox::warning(this, "警告", "打开设备失败！");
        return;
    }
    else
    {
        ui->statusLabel->setText("相机已打开");
        ui->statusLabel->setStyleSheet("QLabel{color:#2ecc71; font-weight:bold;}");
        QMessageBox::information(this, "提示", "相机打开成功！");
    }

    // 设置为触发模式
    m_pcMyCamera->SetEnumValue("TriggerMode", 1);
    // 设置触发源为编码器触发
    m_pcMyCamera->SetEnumValue("TriggerSource", 0);
    // 设置默认曝光时间
    m_pcMyCamera->SetFloatValue("ExposureTime", 500);
    m_pcMyCamera->SetFloatValue("TriggerDelay", 0);
    // 开启相机采集
    m_pcMyCamera->RegisterImageCallBack();
    m_pcMyCamera->StartGrabbing();
    //        connect(cameraThread,&CameraThread::threaderror,this,&Widget::onthreaderrormessage);

    myThread->getCameraPtr(m_pcMyCamera);
    myThread->getImagePtr(myImage);

    m_bOpenDevice = true;
}

// PLC模式选择
void Widget::on_plcmodebtn_clicked()
{
    PLCmode = ui->comboBox_3->currentIndex();
    if (!client->Connected())
    { // 未连接则不执行
        QMessageBox::warning(this, "警告", "PLC未连接！");
        return;
    }

    if (PLCmode == 0)
    {
        uint8_t value = 0;

        byte mode_data[1] = {0}; // Buffer to hold the data to write to PLC

        // 设置要写入的字节
        mode_data[0] = (unsigned char)(0xFF & value);

        // 写入DB1的1032位置，写入1个字节
        int tmp2 = client->WriteArea(S7AreaDB, 1, 1032, 1, S7WLByte, mode_data); // 使用S7WLByte确保只写入1个字节
        QMessageBox::information(this, "提示", "连续模式设置成功");
    }
    else if (PLCmode == 1)
    {
        uint8_t value = 1;

        byte mode_data[1] = {0}; // Buffer to hold the data to write to PLC

        // 设置要写入的字节
        mode_data[0] = (unsigned char)(0xFF & value);

        // 写入DB1的1032位置，写入1个字节
        int tmp2 = client->WriteArea(S7AreaDB, 1, 1032, 1, S7WLByte, mode_data); // 使用S7WLByte确保只写入1个字节
        QMessageBox::information(this, "提示", "间歇模式设置成功");
    }
}

// 模板匹配参数设置
void Widget::on_pushButton_2_clicked()
{
    // 确定裁剪尺寸
    bool ok1;
    int width_min = ui->lineEdit_5->text().toInt(&ok1);
    int width_max = ui->lineEdit_9->text().toInt(&ok1);
    int height_min = ui->lineEdit_10->text().toInt(&ok1);
    int height_max = ui->lineEdit_11->text().toInt(&ok1);
    int block_size1 = ui->lineEdit_13->text().toInt(&ok1);
    int kernelsize = ui->lineEdit_15->text().toInt(&ok1);
    int horizontalKernel = ui->lineEdit_18->text().toInt(&ok1);
    int verticalKernel = ui->lineEdit_19->text().toInt(&ok1);


    emit caijianchicun(width_min, width_max, height_min, height_max, block_size1,  horizontalKernel, verticalKernel);
    emit kernal(kernelsize);
    QMessageBox::warning(this, "提示", "模板尺寸设置成功");
}

// 剔除位置设置
void Widget::on_eliminatebutton_clicked()
{
    wrongindex = ui->lineEdit_12->text().toInt();
    QMessageBox::information(this, "提示", "剔除位置设置成功");
}

//剔除队列复位 清空还未发出的剔除信号
void Widget::on_pushButton_10_clicked()
{
    // 清空剔除队列
    while (!removalQueue.empty())
    {
        removalQueue.pop();
    }
    QMessageBox::information(this, "提示", "剔除队列已清空！");
}

/**
 * @brief 重新初始化 myThread
 * @details 完全清理旧的 myThread 并创建新实例，重新连接所有信号槽
 */
void Widget::reinitializeMyThread()
{
    qDebug() << "=== Reinitializing myThread ===";

    // 步骤1: 如果旧线程还存在，先安全清理
    if (myThread) {
        qDebug() << "Cleaning up old myThread...";

        // 停止线程
        if (myThread->isRunning()) {
            myThread->requestStop();
            myThread->stop();

            // 等待线程完全停止
            if (!myThread->wait(2000)) {
                qDebug() << "WARNING: Old myThread did not stop within 2 seconds";
            }
        }

        // 断开所有信号连接
        disconnect(myThread, nullptr, this, nullptr);

        // 删除旧对象
        delete myThread;
        myThread = nullptr;
        qDebug() << "✓ Old myThread cleaned up";
    }

    // 步骤2: 创建新线程
    qDebug() << "Creating new myThread...";
    myThread = new MyThread();

    // 连接信号槽 - 图像显示
    connect(myThread, &MyThread::signal_messImage, this, [this](cv::Mat img) {
        this->slot_displayAndDetect(&img);
    }, Qt::QueuedConnection);
    // 连接信号槽 - 清除标签
    connect(myThread, &MyThread::signal_cleanlabel,
            this, &Widget::slot_clearResultLabel);

    // 🔥 新增：连接框坐标信号（关键！）
    connect(myThread, &MyThread::signal_boxesSelected,
            this, &Widget::slot_saveBoxesFromThread, Qt::QueuedConnection);

    // 连接信号槽 - 图像检测（根据检测模式）
    QObject::connect(myThread, &MyThread::signal_sendForDetection, this, [this](cv::Mat img, Rect2d rect) {
        if (ui->comboBox_4->currentIndex() == 2) {
            this->slot_readAndDetect(&img, rect);
        } else if(ui->comboBox_4->currentIndex() == 0) {
            this->slot_readAndDetect3(&img, rect);
        } else if(ui->comboBox_4->currentIndex() == 1){
            this->slot_readAndDetect4(&img, rect);
        }
    });

    // 步骤6: 连接其他控制信号
    connect(this, &Widget::rotate, myThread, &MyThread::receiveangle);
    connect(this, &Widget::choosechannel,myThread,&MyThread::receivecolorchannel1);
    connect(this, &Widget::sendDataTo, myThread, &MyThread::received);

    // 步骤7: 如果相机已打开，传递相机指针
    if (m_pcMyCamera && m_bOpenDevice) {
        myThread->getCameraPtr(m_pcMyCamera);
        myThread->getImagePtr(myImage);
        qDebug() << "✓ Camera pointers passed to myThread";
    }

    qDebug() << "✓ myThread reinitialized successfully";
}

/**
 * @brief 重新初始化 cameraThread
 * @details 完全清理旧的 cameraThread 并创建新实例，重新连接所有信号槽
 */
void Widget::reinitializeCameraThread()
{
    qDebug() << "=== Reinitializing cameraThread ===";

    // 步骤1: 如果旧线程还存在，先安全清理
    if (cameraThread) {
        qDebug() << "Cleaning up old cameraThread...";

        // 停止线程
        if (cameraThread->isRunning()) {
            cameraThread->requestStop();
            cameraThread->stopTracking();

            // 关闭OpenCV窗口
            try {
                cv::destroyAllWindows();
            } catch (...) {
                qDebug() << "Exception destroying windows";
            }

            // 等待线程完全停止
            if (!cameraThread->wait(3000)) {
                qDebug() << "WARNING: Old cameraThread did not stop within 3 seconds";
            }
        }

        // 断开所有信号连接
        disconnect(cameraThread, nullptr, this, nullptr);

        // 删除旧对象
        delete cameraThread;
        cameraThread = nullptr;
        qDebug() << "✓ Old cameraThread cleaned up";
    }

    // 步骤2: 检查相机是否可用
    if (!m_pcMyCamera) {
        qDebug() << "ERROR: Cannot reinitialize cameraThread - camera is null";
        return;
    }

    // 步骤3: 创建新线程
    qDebug() << "Creating new cameraThread...";
    cameraThread = new CameraThread(this, m_pcMyCamera);

    // 步骤4: 连接信号槽 - 旋转角度 图像颜色通道
    connect(this, &Widget::rotate, cameraThread, &CameraThread::receiveangle1);
    connect(this, &Widget::choosechannel,cameraThread,&CameraThread::receivecolorchannel);

    // 步骤5: 连接信号槽 - 清除标签
    connect(cameraThread, &CameraThread::signal_cleanlabel,
            this, &Widget::slot_clearResultLabel);

    // 步骤6: 连接信号槽 - 图像显示
    connect(cameraThread, &CameraThread::signal_messImage, this, [this](cv::Mat img) {
        this->slot_displayAndDetect(&img);
    }, Qt::QueuedConnection);

    // 步骤7: 连接信号槽 - 图像检测（根据检测模式）
    connect(cameraThread, &CameraThread::signal_sendForDetection, this, [this](cv::Mat img, Rect2d rect) {
        if (!img.empty()) {
            if (ui->comboBox_4->currentIndex() == 2) {
                this->slot_readAndDetect(&img, rect);
            } else if (ui->comboBox_4->currentIndex() == 0) {
                this->slot_readAndDetect3(&img, rect);
            } else if (ui->comboBox_4->currentIndex() == 1) {
                this->slot_readAndDetect4(&img, rect);
            }
        }
    }, Qt::QueuedConnection);

    // 步骤8: 连接模板匹配相关信号
    connect(this, &Widget::jiancestring, templatematch, &TemplateMatch::jianceshibiestr);
    connect(this, &Widget::sendDataTo, cameraThread, &CameraThread::received);
    connect(this, &Widget::imgmuban, templatematch, &TemplateMatch::recemuban);
    connect(this, &Widget::caijianchicun, templatematch, &TemplateMatch::caijiansize);

    qDebug() << "✓ cameraThread reinitialized successfully";
}

/**
 * @brief 确保线程已就绪
 * @details 在启动线程前调用此函数，检查并重新初始化必要的线程
 *          这是防止崩溃的关键函数
 */
void Widget::ensureThreadsReady()
{
    qDebug() << "=== Ensuring threads are ready ===";

    // 检查 myThread
    if (!myThread) {
        qDebug() << "myThread is null, reinitializing...";
        reinitializeMyThread();
    } else if (myThread->isRunning()) {
        qDebug() << "myThread is already running, stopping and reinitializing...";
        reinitializeMyThread();
    } else {
        qDebug() << "✓ myThread is ready";
    }

    // 检查 cameraThread（只在需要时）
    // 注意：cameraThread 通常在 on_plcbtn_clicked 中创建，这里不检查

    // 处理事件队列，确保清理完成
    QCoreApplication::processEvents();

    qDebug() << "✓ Threads readiness check completed";
}

//启动时预加载字符模板图像
void Widget::loadLastTemplateConfig()
{
    if (currentTemplateDirPath.isEmpty()) {
        return; // 无历史路径，直接返回
    }

    QString newMubiaozifu = ui->dateEdit->toPlainText();
    if (newMubiaozifu.isEmpty()) {
        // 若目标字符为空，清空模板列表
        digitTemplates.clear();
        return;
    }

    // ================== 修复 1：升级正则表达式，加入中文支持 ==================
    QStringList baseNamesToFind;
    // 匹配：带括号数字、纯数字、英文字母、中文字符 [\x{4e00}-\x{9fa5}]
    // 升级正则表达式：允许 [数字、字母、中文] 后面跟带括号的数字作为一个整体
    QRegularExpression regex(R"(([\d[A-Za-z\x{4e00}-\x{9fa5}]\(\d+\))|(\d)|([A-Za-z])|([\x{4e00}-\x{9fa5}]))");
    QRegularExpressionMatchIterator matchIt = regex.globalMatch(newMubiaozifu);

    while (matchIt.hasNext()) {
        QRegularExpressionMatch match = matchIt.next();
        QString unit;
        if (!match.captured(1).isEmpty()) unit = match.captured(1);
        else if (!match.captured(2).isEmpty()) unit = match.captured(2);
        else if (!match.captured(3).isEmpty()) unit = match.captured(3);
        else if (!match.captured(4).isEmpty()) unit = match.captured(4); // 提取到中文字符

        // 统一转为小写以实现不区分大小写的匹配（对中文无影响）
        baseNamesToFind.append(unit.toLower());
    }

    // ================== 修复 2：无视后缀名，建立基础名映射 ==================
    QDir directory(currentTemplateDirPath);
    QMap<QString, QString> filePathMap;
    static const QStringList filters = {"*.jpg", "*.jpeg", "*.png", "*.bmp", "*.tiff"};

    QFileInfoList fileList = directory.entryInfoList(
                filters,
                QDir::Files | QDir::NoDotAndDotDot);

    for (const QFileInfo &fileInfo : fileList) {
        // completeBaseName 剥离后缀，如 "A.png" -> "a" 或 "生.jpg" -> "生"
        QString baseName = fileInfo.completeBaseName().toLower();
        if (!filePathMap.contains(baseName)) {
            filePathMap.insert(baseName, fileInfo.absoluteFilePath());
        }
    }

    // ================== 修复 3：使用内存流解码解决中文路径 BUG ==================
    std::vector<cv::Mat> tempTemplates;
    bool hasMissing = false;

    for (const QString &searchKey : baseNamesToFind) {
        if (filePathMap.contains(searchKey)) {
            // 严禁使用 cv::imread 读取中文路径，改用 QFile 读成 byte 后再用 OpenCV 解码
            QFile file(filePathMap[searchKey]);
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray data = file.readAll();
                std::vector<uchar> buf(data.begin(), data.end());
                cv::Mat templateImg = cv::imdecode(buf, cv::IMREAD_GRAYSCALE);

                if (templateImg.empty()) {
                    hasMissing = true; // 图像损坏解码失败
                } else {
                    tempTemplates.push_back(templateImg);
                }
            } else {
                hasMissing = true; // 文件无法打开
            }
        } else {
            hasMissing = true; // 文件夹里压根没这张图
        }
    }

    // ================== 修复 4：防死锁隔离保护 ==================
    if (hasMissing) {
        digitTemplates.clear();
        qDebug() << "[ERROR] 模板文件夹中的图片缺失或读取失败，已清空模板以保护程序！";
    } else {
        digitTemplates = tempTemplates;
        qDebug() << "[INFO] 模板加载成功，数量: " << digitTemplates.size();
    }
}


/**
 * @brief 接收线程发射的框坐标信号并保存
 */
void Widget::slot_saveBoxesFromThread(cv::Rect2d detectionBox, cv::Rect2d trackingBox)
{
    savedDetectionBox = detectionBox;
    savedTrackingBox = trackingBox;
    hasValidBoxes = true;

    qDebug() << "box saved:";
    qDebug() << "  detection box:" << detectionBox.x << detectionBox.y
             << detectionBox.width << detectionBox.height;
    qDebug() << "  track box:" << trackingBox.x << trackingBox.y
             << trackingBox.width << trackingBox.height;
}

//加载UI样式表模板
void Widget::initStyle()
    {
        QFile file(":/qss/1.css");// 淡蓝色风格
        if(file.open(QFile::ReadOnly)){
            QString qss = QLatin1String(file.readAll());

            // 提取主色调用于设置系统调色板
            QString paletteColor = qss.mid(20,7);// 获取QSS中定义的主色
            qApp->setPalette(QPalette(QColor(paletteColor)));

            // 应用样式表（qApp 是全局应用程序对象，作用于所有控件）
            qApp->setStyleSheet(qss);

            file.close();
        }
    }



//设置颜色通道
void Widget::on_pushButton_7_clicked()
{
    int index = ui->comboBox_5->currentIndex();
    switch (index)
    {
    case 1:
         colorchannel= 1;
        break;
    case 2:
        colorchannel = 2;
        break;
    case 3:
        colorchannel = 3;
        break;
    default:
        colorchannel = 0;
    }

    emit choosechannel(colorchannel);
    QMessageBox::information(this, "提示", "颜色通道设置成功");

}
