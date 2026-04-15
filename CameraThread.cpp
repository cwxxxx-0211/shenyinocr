// CameraThread.cpp
// 相机线程类实现 - 负责图像采集、目标跟踪和检测

#include "CameraThread.h"
#include <QMessageBox>
#include <vector> // 确保包含vector

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
 * @brief CameraThread的构造函数
 * @param parent 父对象，用于管理对象的生命周期
 * @param camera CMvCamera对象指针，用于相机操作
 */
CameraThread::CameraThread(QObject *parent, CMvCamera *camera) :
    QThread(parent),
    m_pcMyCamera(camera),
    m_running(true),
    zhuizong(std::make_unique<Zhuizong>()),
    colorc(0) // 🔥 初始化颜色通道为0 (彩色)
{
    multiTracker = MultiTracker::create();
    presetDetectionBox = cv::Rect2d(0, 0, 0, 0);
    presetTrackingBox = cv::Rect2d(0, 0, 0, 0);
}

CameraThread::~CameraThread()
{
    m_running = false;
    wait();  // 等待线程结束
}

//设置预设框
void CameraThread::setPresetBoxes(const cv::Rect2d& detBox, const cv::Rect2d& trackBox)
{
    presetDetectionBox = detBox;
    presetTrackingBox = trackBox;
    usePresetBoxes = true;

    qDebug() << "CameraThread: box has setting";
    qDebug() << "detection box:" << detBox.x << detBox.y << detBox.width << detBox.height;
    qDebug() << "track box:" << trackBox.x << trackBox.y << trackBox.width << trackBox.height;
}

//清除预设框
void CameraThread::clearPresetBoxes()
{
    usePresetBoxes = false;
    presetDetectionBox = cv::Rect2d(0, 0, 0, 0);
    presetTrackingBox = cv::Rect2d(0, 0, 0, 0);
    qDebug() << "CameraThread: 预设框已清除";
}
/**
 * @brief 检查上升沿（用于硬件触发）
 * @return true 检测到上升沿，false 未检测到
 */
bool CameraThread::CheckRisingEdge()
{
    bool previousState = false;
    bool currentState = false;
    bool inputStatus;

    // 获取Line状态
    int nRet = m_pcMyCamera->GetBoolValue("LineStatus", &inputStatus);
    if (nRet != MV_OK) {
        qDebug() << "Failed to get LineStatus. Error code:" << nRet;
        return false;
    }

    currentState = (inputStatus == true);
    bool risingEdgeDetected = (!previousState && currentState);
    previousState = currentState;

    return risingEdgeDetected;
}

/**
 * @brief 停止跟踪
 * 重置所有跟踪相关的变量和对象
 */
void CameraThread::stopTracking()
{
    destroyAllWindows();
    multiTracker = cv::MultiTracker::create();
    tracking = false;
    qDebug() << "Tracking stopped and reset";
}

/**
 * @brief 接收延时设置
 * @param data 延时时间（毫秒）
 */
void CameraThread::received(QString data)
{
    receivedata = data;
    qDebug() << "Received delay setting:" << data << "ms";
}

/**
 * @brief 接收图像旋转角度设置
 * @param a 旋转角度标志 (0:无旋转, 1:顺时针90°, 2:逆时针90°, 3:180°)
 */
void CameraThread::receiveangle1(int a)
{
    angle2 = a;
    qDebug() << "Rotation angle set to:" << a;
}

/**
 * @brief 接收图像颜色通道设置
 * @param c 颜色通道标志 (0:彩色, 1:红色, 2:绿色, 3:蓝色)
 */
void CameraThread::receivecolorchannel(int c)
{
    colorc = c;
    qDebug() << "colorchannel set to:" << c;
}



void CameraThread::forceStop()
{
    qDebug() << "CameraThread::forceStop() called";

    // 请求停止
    requestStop();

    // 关闭 OpenCV 窗口中断阻塞
    qDebug() << "Closing windows to interrupt blocking...";
    try {
        cv::destroyWindow("MultiTracker");
        cv::destroyAllWindows();
    } catch (...) {
        qDebug() << "Exception destroying windows in forceStop";
    }

    // 等待线程结束，增加到 5 秒
    qDebug() << "Waiting for thread to finish (max 5 seconds)...";
    if (!wait(5000)) {
        qDebug() << "WARNING: Thread did not finish in 5 seconds";
        qDebug() << "Will NOT call terminate() to avoid crash";
        qDebug() << "Thread will be abandoned (potential resource leak)";

    } else {
        qDebug() << "CameraThread stopped successfully";
    }
}

/**
 * @brief 改进的 requestStop() 函数
 */
void CameraThread::requestStop()
{
    qDebug() << "CameraThread::requestStop() called";
    m_stopRequested.store(true);
    m_running = false;

    qDebug()<<"1";

//    try {
//        cv::destroyAllWindows();
//        cv::waitKey(1);  // 刷新事件队列
//    } catch (...) {
//        qDebug() << "Exception destroying windows in requestStop";
//    }

    qDebug()<<"2";
    // 通知CMvCamera停止阻塞
    if (m_pcMyCamera) {
        try {
            m_pcMyCamera->requestStop();
        } catch (...) {
            qDebug() << "Exception in camera requestStop";
        }
    }

    qDebug()<<"3";
    // 关闭OpenCV窗口
    try {
        cv::destroyWindow("MultiTracker");
    } catch (...) {
        qDebug() << "Exception destroying window";
    }
}

void CameraThread::run()
{
    std::unique_ptr<Mat> image = std::make_unique<Mat>();

    const int DISPLAY_WIDTH = 640;
    const int DISPLAY_HEIGHT = 480;
    double scaleX = 1.0;
    double scaleY = 1.0;

    Rect2d detectionBox;
    Rect2d initialDetectionBox;
    Rect2d trackingBox;
    Rect2d initialTrackingBox;

    bool detectionBoxSet = false;
    bool trackingBoxSet = false;
    bool tracking = false;

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

    m_pcMyCamera->setnonblocking(true);
    qDebug() << "Camera thread started, ready to get images";

    // 🔥 修改提示信息
    if (usePresetBoxes) {
        qDebug() << "使用预设框模式，无需手动选择";
    } else {
        qDebug() << "手动选框模式，程序将在MultiTracker窗口中自动选择框，按C取消，ESC退出";
    }

    // 🔥 新增：标记是否需要初始化追踪器（仅在使用预设框时）
    bool needInitTracker = (usePresetBoxes && detectionBoxSet && trackingBoxSet);

    while (m_running && !m_stopRequested.load()) {
        if (m_stopRequested.load()) {
            break;
        }

        if (m_pcMyCamera) {
            try {
                *image = m_pcMyCamera->timesGetImage();

                if (image->empty()) {
                    qDebug() << "Got empty image";
                    if (m_stopRequested.load()) {
                        break;
                    }
                    msleep(10);
                    continue;
                }

                // 图像旋转处理（代码不变）
                if (angle2 == 1) {
                    cv::Mat rotatedImg;
                    cv::rotate(*image, rotatedImg, cv::ROTATE_90_CLOCKWISE);
                    *image = rotatedImg;
                }
                else if (angle2 == 2) {
                    cv::Mat rotatedImg;
                    cv::rotate(*image, rotatedImg, cv::ROTATE_90_COUNTERCLOCKWISE);
                    *image = rotatedImg;
                }
                else if (angle2 == 3) {
                    cv::Mat rotatedImg;
                    cv::rotate(*image, rotatedImg, cv::ROTATE_180);
                    *image = rotatedImg;
                }

                // 🔥🔥🔥【新增】颜色通道提取功能 🔥🔥🔥
                // 逻辑：如果选择了特定通道(colorc > 0)且图像非空，则提取单通道灰度图
                // 必须在旋转之后，缩放和显示之前进行
                if (colorc > 0 && !image->empty()) {
                    std::vector<cv::Mat> channels;
                    cv::split(*image, channels);

                    // 确保是3通道图像才进行操作，防止崩溃
                    if (channels.size() >= 3) {
                        switch (colorc) {
                        case 1: // 红色通道 (OpenCV BGR顺序: 0=B, 1=G, 2=R)
                            *image = channels[2];
                            break;
                        case 2: // 绿色通道
                            *image = channels[1];
                            break;
                        case 3: // 蓝色通道
                            *image = channels[0];
                            break;
                        }
                        // 此时 *image 已经变成了单通道灰度图
                    }
                }
                // 🔥🔥🔥【新增结束】🔥🔥🔥

                if (needInitTracker && !tracking && detectionBoxSet && trackingBoxSet) {
                    qDebug() << "first image is arrive";

                    cv::Rect imageRect(0, 0, image->cols, image->rows);
                    cv::Rect trackBoxInt(trackingBox.x, trackingBox.y,
                                         trackingBox.width, trackingBox.height);

                    if ((trackBoxInt & imageRect) == trackBoxInt) {
                        multiTracker->add(cv::TrackerMOSSE::create(), *image, trackingBox);
                        tracking = true;
                        needInitTracker = false;

                        // 🔥 预设框初始化成功后也发射信号
                        emit signal_boxesSelected(detectionBox, trackingBox);

                        qDebug() << "track already";
                        m_pcMyCamera->deferswitchtoblockingafternextframe();
                    } else {
                        qDebug() << "wrong box is over image";
                        qDebug() << "imagesize:" << image->cols << "x" << image->rows;
                        qDebug() << "boxsize:" << trackingBox.x << trackingBox.y
                                 << trackingBox.width << trackingBox.height;

                        // 清除预设框，切换到手动选框模式
                        usePresetBoxes = false;
                        detectionBoxSet = false;
                        trackingBoxSet = false;
                        needInitTracker = false;
                    }
                }

                scaleX = static_cast<double>(DISPLAY_WIDTH) / image->cols;
                scaleY = static_cast<double>(DISPLAY_HEIGHT) / image->rows;

                cv::Mat displayImage;
                cv::resize(*image, displayImage, cv::Size(DISPLAY_WIDTH, DISPLAY_HEIGHT));

                if (tracking) {
                    multiTracker->update(*image);
                    Rect2d updatedTrackingBox = multiTracker->getObjects()[0];
                    Point2d displacement = Point2d(
                        updatedTrackingBox.x - initialTrackingBox.x,
                        updatedTrackingBox.y - initialTrackingBox.y
                    );
                    detectionBox.x = initialDetectionBox.x + displacement.x;
                    detectionBox.y = initialDetectionBox.y + displacement.y;

                    if (m_pcMyCamera->isImageReadyForMain()) {
                        emit signal_cleanlabel();
                        emit signal_sendForDetection(image->clone(), detectionBox);
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
                    break;
                }

                try {
                    cv::imshow("MultiTracker", displayImage);
                    emit signal_messImage(image->clone());
                } catch (const cv::Exception& e) {
                    qDebug() << "OpenCV exception in imshow:" << e.what();
                    break;
                }

                char key = (char)cv::waitKey(1);

                if (m_stopRequested.load()) {
                    break;
                }

                try {
                    if (cv::getWindowProperty("MultiTracker", cv::WND_PROP_AUTOSIZE) < 0) {
                        break;
                    }
                } catch (...) {
                    break;
                }

                if (key == 27 || m_stopRequested.load()) {
                    m_running = false;
                    break;
                }
                else if (key == 'c' || key == 'C') {
                    qDebug() << "C pressed, cancelling current setup";
                    detectionBoxSet = false;
                    trackingBoxSet = false;
                    tracking = false;
                    usePresetBoxes = false;  // 🔥 新增：清除预设框标志
                    needInitTracker = false;  // 🔥 新增：清除初始化标志
                    multiTracker = cv::MultiTracker::create();
                    qDebug() << "Setup cancelled, ready to select new boxes";
                }

                // 🔥 修改：只有在非预设框模式下才自动选框
                if (!usePresetBoxes) {
                    if (!detectionBoxSet && !tracking) {
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
                                         << " " << detectionBox.width << "x" << detectionBox.height;
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
                    else if (detectionBoxSet && !trackingBoxSet && !tracking) {
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

                                multiTracker->add(cv::TrackerMOSSE::create(), *image, trackingBox);
                                tracking = true;

                                // 🔥 新增：框选择完成后，发射信号传递框坐标
                                emit signal_boxesSelected(detectionBox, trackingBox);

                                qDebug() << "Tracking box set (original coords):"
                                         << trackingBox.x << "," << trackingBox.y
                                         << " " << trackingBox.width << "x" << trackingBox.height;
                                qDebug() << "Tracking started automatically";
                                m_pcMyCamera->deferswitchtoblockingafternextframe();
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
                }  // 🔥 结束：非预设框模式的选框逻辑

            } catch (const cv::Exception& e) {
                qDebug() << "OpenCV exception in loop:" << e.what();
                if (m_stopRequested.load()) {
                    break;
                }
                msleep(10);
                continue;
            } catch (...) {
                qDebug() << "Unknown exception in loop";
                if (m_stopRequested.load()) {
                    break;
                }
                msleep(10);
                continue;
            }
        }

        // 线程休眠
        bool ok;
        int sleepTime = receivedata.toInt(&ok);
        if (!ok || sleepTime <= 0) {
            sleepTime = 100;
        }

        int totalSleep = 0;
        while (totalSleep < sleepTime && !m_stopRequested.load()) {
            msleep(10);
            totalSleep += 10;
        }

        if (m_stopRequested.load()) {
            break;
        }
    }

    try {
        cv::destroyAllWindows();
    } catch (...) {}
    qDebug() << "Camera thread finished";
}
