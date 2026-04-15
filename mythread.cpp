#include "mythread.h"
#include <QMessageBox>
#include <QDebug>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <vector>

// 🔥🔥🔥 自定义选框函数 - 鼠标松开自动确认，不需要按回车 🔥🔥🔥
namespace {
struct MouseCallbackData {
    cv::Mat image;
    cv::Point startPoint;
    cv::Point endPoint;
    bool drawing;
    bool finished;
    cv::Rect2d selectedROI;
};

static void onMouseSelectROI(int event, int x, int y, int, void* userdata) {
    MouseCallbackData* data = static_cast<MouseCallbackData*>(userdata);

    if (event == cv::EVENT_LBUTTONDOWN) {
        data->startPoint = cv::Point(x, y);
        data->endPoint = cv::Point(x, y);
        data->drawing = true;
        data->finished = false;
    }
    else if (event == cv::EVENT_MOUSEMOVE && data->drawing) {
        data->endPoint = cv::Point(x, y);
    }
    else if (event == cv::EVENT_LBUTTONUP && data->drawing) {
        data->endPoint = cv::Point(x, y);
        data->drawing = false;
        data->finished = true;

        int x1 = std::min(data->startPoint.x, data->endPoint.x);
        int y1 = std::min(data->startPoint.y, data->endPoint.y);
        int width = std::abs(data->endPoint.x - data->startPoint.x);
        int height = std::abs(data->endPoint.y - data->startPoint.y);

        data->selectedROI = cv::Rect2d(x1, y1, width, height);
    }
}

static cv::Rect2d selectROIAutoConfirm(const std::string& windowName, const cv::Mat& img) {
    MouseCallbackData data;
    data.image = img.clone();
    data.drawing = false;
    data.finished = false;
    data.selectedROI = cv::Rect2d(0, 0, 0, 0);

    cv::setMouseCallback(windowName, onMouseSelectROI, &data);

    while (!data.finished) {
        cv::Mat display = data.image.clone();

        if (data.drawing) {
            cv::rectangle(display, data.startPoint, data.endPoint, cv::Scalar(0, 255, 0), 2);
        }

        cv::imshow(windowName, display);

        int key = cv::waitKey(10);
        if (key == 27) {  // ESC键取消
            data.selectedROI = cv::Rect2d(0, 0, 0, 0);
            break;
        }
    }

    cv::setMouseCallback(windowName, nullptr, nullptr);
    return data.selectedROI;
}
}  // namespace



/**
 * @brief MyThread构造函数
 * @param parent 父对象
 */
MyThread::MyThread(QObject *parent)
    : QThread{parent},
      myImage(new QImage()),
      zhuizong(nullptr),
      cameraPtr(nullptr),
      imagePtr(nullptr),
      angle1(0),
      colorc1(0), // 🔥 初始化颜色通道为0
      m_stopRequested(false),
      m_tracking(false)
{
    multiTracker = MultiTracker::create();
    zhuizong = new Zhuizong;
    // receivedata 已在头文件中初始化为 "300"
    lastDetectionTime = std::chrono::steady_clock::now();
    presetDetectionBox = cv::Rect2d(0, 0, 0, 0);
    presetTrackingBox = cv::Rect2d(0, 0, 0, 0);


    qDebug() << "MyThread constructed";
}

/**
 * @brief 析构函数 - 清理资源
 */
MyThread::~MyThread()
{
    qDebug() << "MyThread destructor called";

    // 确保线程已停止
    if (isRunning()) {
        requestStop();
        wait(1000);
        if (isRunning()) {
            terminate();
            wait();
        }
    }

    delete myImage;
    if (cameraPtr != nullptr) {
        delete cameraPtr;
    }
    if (imagePtr != nullptr) {
        delete imagePtr;
    }
    delete zhuizong;

    qDebug() << "MyThread destroyed";
}

/**
 * @brief 停止相机并关闭窗口
 */
void MyThread::stop()
{
    qDebug() << "MyThread::stop() called";
    m_stopRequested.store(true);
    m_tracking.store(false);

    // ✅ 关键修复：立即关闭所有OpenCV窗口（包括selectROI窗口）
    qDebug() << "Destroying all OpenCV windows to interrupt selectROI...";
    try {
        cv::destroyAllWindows();
        QThread::msleep(50);
        cv::waitKey(1);  // 刷新事件队列
    } catch (...) {
        qDebug() << "Exception destroying windows in stop()";
    }

    // 关闭特定窗口
    try {
        cv::destroyWindow("MultiTracker");
    } catch (...) {
        qDebug() << "Exception destroying specific windows";
    }

    if (cameraPtr) {
        try {
            cameraPtr->Close();
        } catch (...) {
            qDebug() << "Exception closing camera";
        }
    }

    // 再次确保所有OpenCV窗口都关闭
    cv::destroyAllWindows();

    qDebug() << "MyThread::stop() completed";
}

/**
 * @brief 请求停止线程（线程安全）
 */
void MyThread::requestStop()
{
    qDebug() << "MyThread::requestStop() called";
    m_stopRequested.store(true);
    m_tracking.store(false);
//    try {
//        cv::destroyWindow("MultiTracker");
//    } catch (...) {
//        qDebug() << "Exception destroying window";
//    }
}

void MyThread::setPresetBoxes(const cv::Rect2d& detBox, const cv::Rect2d& trackBox)
{
    presetDetectionBox = detBox;
    presetTrackingBox = trackBox;
    usePresetBoxes = true;

    qDebug() << "MyThread: 预设框已设置";
    qDebug() << "检测框:" << detBox.x << detBox.y << detBox.width << detBox.height;
    qDebug() << "跟踪框:" << trackBox.x << trackBox.y << trackBox.width << trackBox.height;
}

void MyThread::clearPresetBoxes()
{
    usePresetBoxes = false;
    presetDetectionBox = cv::Rect2d(0, 0, 0, 0);
    presetTrackingBox = cv::Rect2d(0, 0, 0, 0);
    qDebug() << "MyThread: 预设框已清除";
}

/**
 * @brief 接收图像旋转角度设置
 * @param a 旋转角度标志 (0:无旋转, 1:顺时针90°, 2:逆时针90°, 3:180°)
 */
void MyThread::receiveangle(int a)
{
    angle1 = a;
    qDebug() << "Received rotation angle:" << angle1;
}

/**
 * @brief 接收图像颜色通道设置
 * @param c 颜色通道标志 (0:彩色, 1:红色, 2:绿色, 3:蓝色)
 */
void MyThread::receivecolorchannel1(int c)
{
    colorc1 = c;
    qDebug() << "Received colorchannel:" << colorc1;
}

/**
 * @brief 获取相机指针
 * @param camera 相机对象指针
 */
void MyThread::getCameraPtr(CMvCamera *camera)
{
    cameraPtr = camera;
    qDebug() << "Camera pointer set";
}

/**
 * @brief 获取图像指针
 * @param image 图像Mat指针
 */
void MyThread::getImagePtr(cv::Mat *image)
{
    imagePtr = image;
    qDebug() << "Image pointer set";
}

/**
 * @brief 线程主运行函数
 */
void MyThread::run()
{
    qDebug() << "=== MyThread::run() started ===";

    if (cameraPtr == nullptr || imagePtr == nullptr) {
        qDebug() << "ERROR: Camera or image pointer is null, cannot start thread";
        return;
    }

    const int DISPLAY_WIDTH = 640;
    const int DISPLAY_HEIGHT = 480;
    double scaleX = 1.0;
    double scaleY = 1.0;

    m_stopRequested.store(false);
    m_tracking.store(false);

    Rect2d detectionBox;
    Rect2d initialDetectionBox;
    Rect2d trackingBox;
    Rect2d initialTrackingBox;

    bool detectionBoxSet = false;
    bool trackingBoxSet = false;

    // 🔥 新增：检查是否使用预设框
    if (usePresetBoxes &&
        presetDetectionBox.width > 0 && presetDetectionBox.height > 0 &&
        presetTrackingBox.width > 0 && presetTrackingBox.height > 0) {

        qDebug() << "使用预设框，跳过选框步骤";

        // 直接使用预设框
        detectionBox = presetDetectionBox;
        initialDetectionBox = presetDetectionBox;
        detectionBoxSet = true;

        trackingBox = presetTrackingBox;
        initialTrackingBox = presetTrackingBox;
        trackingBoxSet = true;

        qDebug() << "预设框已应用，等待第一帧图像以初始化追踪器...";
    }

    vector<Scalar> colors;
    zhuizong->getRandomColors(colors, 2);

    lastDetectionTime = std::chrono::steady_clock::now();

    qDebug() << "MyThread loop starting...";
    if (usePresetBoxes) {
        qDebug() << "使用预设框模式，无需手动选择";
    } else {
        qDebug() << "手动选框模式，程序将在MultiTracker窗口中自动选择框，按C取消，ESC退出";
    }

    // 🔥 新增：标记是否需要初始化追踪器
    bool needInitTracker = (usePresetBoxes && detectionBoxSet && trackingBoxSet);

    while (cameraPtr && !m_stopRequested.load()) {

        if (m_stopRequested.load()) {
            qDebug() << "Stop requested at loop start, breaking";
            break;
        }

        try {
            cameraPtr->CommandExecute("TriggerSoftware");
            *imagePtr = cameraPtr->timesGetImage();

            // 图像旋转处理
            if (angle1 == 1) {
                cv::Mat rotatedImg;
                cv::rotate(*imagePtr, rotatedImg, cv::ROTATE_90_CLOCKWISE);
                *imagePtr = rotatedImg;
            }
            else if (angle1 == 2) {
                cv::Mat rotatedImg;
                cv::rotate(*imagePtr, rotatedImg, cv::ROTATE_90_COUNTERCLOCKWISE);
                *imagePtr = rotatedImg;
            }
            else if (angle1 == 3) {
                cv::Mat rotatedImg;
                cv::rotate(*imagePtr, rotatedImg, cv::ROTATE_180);
                *imagePtr = rotatedImg;
            }

            if (imagePtr->empty()) {
                qDebug() << "Image is empty, breaking loop";
                break;
            }


            // 逻辑：如果选择了特定通道(colorc1 > 0)，则提取单通道灰度图
            if (colorc1 > 0) {
                std::vector<cv::Mat> channels;
                cv::split(*imagePtr, channels);

                // 确保是3通道图像才进行操作
                if (channels.size() >= 3) {
                    switch (colorc1) {
                    case 1: // 红色通道 (OpenCV BGR顺序: 0=B, 1=G, 2=R)
                        *imagePtr = channels[2];
                        break;
                    case 2: // 绿色通道
                        *imagePtr = channels[1];
                        break;
                    case 3: // 蓝色通道
                        *imagePtr = channels[0];
                        break;
                    }
                }
            }
            // 🔥🔥🔥【新增结束】🔥🔥🔥

            if (m_stopRequested.load()) {
                qDebug() << "Stop requested after image acquisition, breaking";
                break;
            }

            // 🔥 新增：如果使用预设框且追踪器未初始化
            if (needInitTracker && !m_tracking.load() && detectionBoxSet && trackingBoxSet) {
                qDebug() << "第一帧图像已到达，初始化追踪器...";

                cv::Rect imageRect(0, 0, imagePtr->cols, imagePtr->rows);
                cv::Rect trackBoxInt(trackingBox.x, trackingBox.y,
                                     trackingBox.width, trackingBox.height);

                if ((trackBoxInt & imageRect) == trackBoxInt) {
                    multiTracker->add(cv::TrackerMOSSE::create(), *imagePtr, trackingBox);
                    m_tracking.store(true);
                    needInitTracker = false;
                    lastDetectionTime = std::chrono::steady_clock::now();

                    // 🔥 预设框初始化成功后也发射信号
                    emit signal_boxesSelected(detectionBox, trackingBox);

                    qDebug() << "track already";
                }  else {
                    qDebug() << "wrong box is over image";
                    qDebug() << "imagesize:" << imagePtr->cols << "x" << imagePtr->rows;

                    // 清除预设框
                    usePresetBoxes = false;
                    detectionBoxSet = false;
                    trackingBoxSet = false;
                    needInitTracker = false;
                }
            }

            scaleX = static_cast<double>(DISPLAY_WIDTH) / imagePtr->cols;
            scaleY = static_cast<double>(DISPLAY_HEIGHT) / imagePtr->rows;
            cv::Mat displayImage;
            cv::resize(*imagePtr, displayImage, cv::Size(DISPLAY_WIDTH, DISPLAY_HEIGHT));

            if (m_tracking.load()) {
                bool updateSuccess = multiTracker->update(*imagePtr);

                if (updateSuccess && multiTracker->getObjects().size() > 0) {
                    Rect2d updatedTrackingBox = multiTracker->getObjects()[0];
                    Point2d displacement = Point2d(
                        updatedTrackingBox.x - initialTrackingBox.x,
                        updatedTrackingBox.y - initialTrackingBox.y
                    );
                    detectionBox.x = initialDetectionBox.x + displacement.x;
                    detectionBox.y = initialDetectionBox.y + displacement.y;

                    auto now = std::chrono::steady_clock::now();
                    bool ok;
                    int detectionInterval = receivedata.toInt(&ok);
                    if (!ok) {
                        detectionInterval = 300;
                    }

                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                now - lastDetectionTime).count();

                    if (elapsed >= detectionInterval) {
                        emit signal_cleanlabel();
                        emit signal_sendForDetection(imagePtr->clone(), detectionBox);
                        lastDetectionTime = now;
                        qDebug() << "Detection signal sent (interval:" << detectionInterval << "ms)";
                    }

                    Rect2d scaledDetectionBox(
                        detectionBox.x * scaleX,
                        detectionBox.y * scaleY,
                        detectionBox.width * scaleX,
                        detectionBox.height * scaleY
                    );
                    rectangle(displayImage, scaledDetectionBox, colors[0], 2, 1);

                    Rect2d scaledTrackingBox(
                        updatedTrackingBox.x * scaleX,
                        updatedTrackingBox.y * scaleY,
                        updatedTrackingBox.width * scaleX,
                        updatedTrackingBox.height * scaleY
                    );
                    rectangle(displayImage, scaledTrackingBox, colors[1], 2, 1);

                } else {
                    qDebug() << "Tracking update failed or no objects";
                }
            }
            else if (detectionBoxSet || trackingBoxSet) {
                if (detectionBoxSet) {
                    Rect2d scaledBox(
                        detectionBox.x * scaleX,
                        detectionBox.y * scaleY,
                        detectionBox.width * scaleX,
                        detectionBox.height * scaleY
                    );
                    rectangle(displayImage, scaledBox, colors[0], 2, 1);
                }
                if (trackingBoxSet) {
                    Rect2d scaledBox(
                        trackingBox.x * scaleX,
                        trackingBox.y * scaleY,
                        trackingBox.width * scaleX,
                        trackingBox.height * scaleY
                    );
                    rectangle(displayImage, scaledBox, colors[1], 2, 1);
                }
            }

            if (m_stopRequested.load()) {
                qDebug() << "Stop requested before display, breaking";
                break;
            }

            cv::imshow("MultiTracker", displayImage);
            emit signal_messImage(imagePtr->clone());

            char key = (char)waitKey(1);

            if (m_stopRequested.load()) {
                qDebug() << "Stop requested after waitKey, breaking";
                break;
            }

            try {
                if (cv::getWindowProperty("MultiTracker", cv::WND_PROP_AUTOSIZE) < 0) {
                    qDebug() << "Window closed by user, breaking";
                    break;
                }
            } catch (const cv::Exception& e) {
                qDebug() << "OpenCV exception checking window:" << e.what();
                break;
            } catch (...) {
                qDebug() << "Unknown exception checking window, breaking";
                break;
            }

            if (key == 27) {
                qDebug() << "ESC pressed, exiting";
                break;
            }
            else if (key == 'c' || key == 'C') {
                qDebug() << "C pressed, cancelling current setup";
                detectionBoxSet = false;
                trackingBoxSet = false;
                m_tracking.store(false);
                usePresetBoxes = false;  // 🔥 新增
                needInitTracker = false;  // 🔥 新增
                multiTracker = cv::MultiTracker::create();
                qDebug() << "Setup cancelled, ready to select new boxes";
            }

            // 🔥 修改：只有在非预设框模式下才自动选框
            if (!usePresetBoxes) {
                if (!detectionBoxSet && !m_tracking.load()) {
                    if (m_stopRequested.load()) {
                        break;
                    }

                    qDebug() << "Auto-selecting detection box...";
                    try {
                        Rect2d scaledBox = selectROIAutoConfirm("MultiTracker", displayImage);

                        if (m_stopRequested.load()) {
                            qDebug() << "Stop requested after selectROI";
                            break;
                        }

                        if (scaledBox.width > 0 && scaledBox.height > 0) {
                            detectionBox.x = scaledBox.x / scaleX;
                            detectionBox.y = scaledBox.y / scaleY;
                            detectionBox.width = scaledBox.width / scaleX;
                            detectionBox.height = scaledBox.height / scaleY;

                            initialDetectionBox = detectionBox;
                            detectionBoxSet = true;
                            qDebug() << "Detection box set (original coords):"
                                     << detectionBox.x << "," << detectionBox.y
                                     << "size:" << detectionBox.width << "x" << detectionBox.height;
                        } else {
                            qDebug() << "Detection box selection cancelled";
                        }
                    } catch (const cv::Exception& e) {
                        qDebug() << "selectROI exception:" << e.what();
                        break;
                    } catch (...) {
                        qDebug() << "selectROI interrupted";
                        break;
                    }
                }
                else if (detectionBoxSet && !trackingBoxSet && !m_tracking.load()) {
                    if (m_stopRequested.load()) {
                        break;
                    }

                    qDebug() << "Auto-selecting tracking box...";
                    try {
                        Rect2d scaledBox = selectROIAutoConfirm("MultiTracker", displayImage);

                        if (m_stopRequested.load()) {
                            qDebug() << "Stop requested after selectROI";
                            break;
                        }

                        if (scaledBox.width > 0 && scaledBox.height > 0) {
                            trackingBox.x = scaledBox.x / scaleX;
                            trackingBox.y = scaledBox.y / scaleY;
                            trackingBox.width = scaledBox.width / scaleX;
                            trackingBox.height = scaledBox.height / scaleY;

                            initialTrackingBox = trackingBox;
                            trackingBoxSet = true;

                            multiTracker->add(cv::TrackerMOSSE::create(), *imagePtr, trackingBox);
                            m_tracking.store(true);

                            // 🔥 新增：框选择完成后，发射信号传递框坐标
                            emit signal_boxesSelected(detectionBox, trackingBox);

                            qDebug() << "Tracking box set (original coords):"
                                     << trackingBox.x << "," << trackingBox.y
                                     << "size:" << trackingBox.width << "x" << trackingBox.height;
                            qDebug() << "Tracking started automatically";

                            lastDetectionTime = std::chrono::steady_clock::now();
                        } else {
                            qDebug() << "Tracking box selection cancelled";
                        }
                    } catch (const cv::Exception& e) {
                        qDebug() << "selectROI exception:" << e.what();
                        break;
                    } catch (...) {
                        qDebug() << "selectROI interrupted";
                        break;
                    }
                }
            }  // 🔥 结束：非预设框模式

        } catch (const cv::Exception& e) {
            qDebug() << "OpenCV exception in run loop:" << e.what();
            break;
        } catch (const std::exception& e) {
            qDebug() << "Standard exception in run loop:" << e.what();
            break;
        } catch (...) {
            qDebug() << "Unknown exception in run loop";
            break;
        }

        msleep(100);

        if (m_stopRequested.load()) {
            qDebug() << "Stop requested at loop end, breaking";
            break;
        }
    }

    qDebug() << "Cleaning up MyThread resources...";
    try {
        cv::destroyAllWindows();
    } catch (...) {
        qDebug() << "Exception destroying windows in cleanup";
    }

    qDebug() << "=== MyThread::run() finished ===";
}

/**
 * @brief 开始跟踪（外部调用）
 */
void MyThread::startTracking()
{
    m_tracking.store(true);
    lastDetectionTime = std::chrono::steady_clock::now();
    qDebug() << "Tracking started externally";
}

/**
 * @brief 检查上升沿（用于硬件触发）
 * @return true 检测到上升沿，false 未检测到
 */
bool MyThread::CheckRisingEdge()
{
    static bool previousState = false;
    bool currentState = false;

    if (!cameraPtr) {
        qDebug() << "ERROR: Camera pointer is null in CheckRisingEdge";
        return false;
    }

    bool inputStatus;
    memset(&inputStatus, 0, sizeof(bool));

    // 获取Line状态
    int nRet = cameraPtr->GetBoolValue("LineStatus", &inputStatus);
    if (nRet != MV_OK) {
        qDebug() << "Failed to get LineStatus. Error code:" << nRet;
        return false;
    }

    currentState = (inputStatus == 1);
    bool risingEdgeDetected = (!previousState && currentState);
    previousState = currentState;

    return risingEdgeDetected;
}

/**
 * @brief 接收检测间隔设置
 * @param data 检测间隔时间（毫秒）
 */
void MyThread::received(QString data)
{
    receivedata = data;
    qDebug() << "Received detection interval:" << data << "ms";
}

/**
 * @brief 停止跟踪
 * 重置所有跟踪相关的变量和对象
 */
void MyThread::stopTracking()
{
    qDebug() << "MyThread::stopTracking() called";

    try {
        cv::destroyAllWindows();
    } catch (...) {
        qDebug() << "Exception destroying windows in stopTracking";
    }

    try {
        multiTracker = cv::MultiTracker::create();
    } catch (...) {
        qDebug() << "Exception creating new MultiTracker";
    }

    m_tracking.store(false);
    qDebug() << "Tracking stopped and reset";
}
