// mythread.cpp
#include "mythread.h"
#include <QDebug>
#include <chrono>

MyThread::MyThread(QObject *parent)
    : QThread{parent}, myImage(new QImage()), zhuizong(new Zhuizong),
      cameraPtr(nullptr), imagePtr(nullptr), angle1(0), colorc1(0),
      m_stopRequested(false), m_tracking(false)
{
    multiTracker = cv::MultiTracker::create();
    lastDetectionTime = std::chrono::steady_clock::now();
    presetDetectionBox = cv::Rect2d(0, 0, 0, 0);
    presetTrackingBox = cv::Rect2d(0, 0, 0, 0);
    usePresetBoxes = false;
}

MyThread::~MyThread() {
    if (isRunning()) {
        requestStop();
        wait(1000);
    }
    delete myImage;
    if (zhuizong) delete zhuizong;
}

void MyThread::stop() { m_stopRequested.store(true); m_tracking.store(false); }
void MyThread::requestStop() { m_stopRequested.store(true); m_tracking.store(false); }

void MyThread::setPresetBoxes(const cv::Rect2d& detBox, const cv::Rect2d& trackBox) {
    presetDetectionBox = detBox;
    presetTrackingBox = trackBox;
    usePresetBoxes = true;
}

void MyThread::clearPresetBoxes() { usePresetBoxes = false; }
void MyThread::receiveangle(int a) { angle1 = a; }
void MyThread::receivecolorchannel1(int c) { colorc1 = c; }
void MyThread::getCameraPtr(CMvCamera *camera) { cameraPtr = camera; }
void MyThread::getImagePtr(cv::Mat *image) { imagePtr = image; }
void MyThread::received(QString data) { receivedata = data; }

void MyThread::run() {
    if (!cameraPtr || !imagePtr) return;

    m_stopRequested.store(false);
    m_tracking.store(false);

    cv::Rect2d detectionBox = presetDetectionBox;
    cv::Rect2d initialDetectionBox = presetDetectionBox;
    cv::Rect2d trackingBox = presetTrackingBox;
    cv::Rect2d initialTrackingBox = presetTrackingBox;

    bool needInitTracker = (usePresetBoxes && presetDetectionBox.width > 0);
    lastDetectionTime = std::chrono::steady_clock::now();

    while (cameraPtr && !m_stopRequested.load()) {
        try {
            cameraPtr->CommandExecute("TriggerSoftware");
            *imagePtr = cameraPtr->timesGetImage();

            if (imagePtr->empty()) {
                msleep(10);
                continue;
            }

            // 图像旋转与通道过滤
            if (angle1 == 1) cv::rotate(*imagePtr, *imagePtr, cv::ROTATE_90_CLOCKWISE);
            else if (angle1 == 2) cv::rotate(*imagePtr, *imagePtr, cv::ROTATE_90_COUNTERCLOCKWISE);
            else if (angle1 == 3) cv::rotate(*imagePtr, *imagePtr, cv::ROTATE_180);

            if (colorc1 > 0 && imagePtr->channels() >= 3) {
                std::vector<cv::Mat> channels;
                cv::split(*imagePtr, channels);
                if (colorc1 == 1) *imagePtr = channels[2];
                else if (colorc1 == 2) *imagePtr = channels[1];
                else if (colorc1 == 3) *imagePtr = channels[0];
            }

            // 初始化追踪器 (首帧)
            if (needInitTracker && !m_tracking.load()) {
                cv::Rect imageRect(0, 0, imagePtr->cols, imagePtr->rows);
                cv::Rect trackBoxInt(trackingBox.x, trackingBox.y, trackingBox.width, trackingBox.height);
                if ((trackBoxInt & imageRect) == trackBoxInt) {
                    multiTracker = cv::MultiTracker::create(); // 清空旧的
                    multiTracker->add(cv::TrackerMOSSE::create(), *imagePtr, trackingBox);
                    m_tracking.store(true);
                    needInitTracker = false;
                    lastDetectionTime = std::chrono::steady_clock::now();
                    emit signal_boxesSelected(detectionBox, trackingBox);
                } else {
                    needInitTracker = false; // 框超出边界，放弃追踪
                }
            }

            // 将原始图像克隆一份用于画框展示，绝不污染原图
            cv::Mat displayImage = imagePtr->clone();

            if (m_tracking.load()) {
                // 更新追踪器
                bool updateSuccess = multiTracker->update(*imagePtr);
                if (updateSuccess && multiTracker->getObjects().size() > 0) {
                    cv::Rect2d updatedTrackingBox = multiTracker->getObjects()[0];
                    // 计算位移并应用到检测框 (因为我们只有1个框，所以它俩始终完全重合)
                    cv::Point2d displacement(updatedTrackingBox.x - initialTrackingBox.x, updatedTrackingBox.y - initialTrackingBox.y);
                    detectionBox.x = initialDetectionBox.x + displacement.x;
                    detectionBox.y = initialDetectionBox.y + displacement.y;

                    // 节拍延时检测触发
                    auto now = std::chrono::steady_clock::now();
                    int detectionInterval = receivedata.toInt();
                    if (detectionInterval <= 0) detectionInterval = 300;
                    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastDetectionTime).count() >= detectionInterval) {
                        emit signal_cleanlabel();
                        emit signal_sendForDetection(imagePtr->clone(), detectionBox);
                        lastDetectionTime = now;
                    }

                    // 画出追踪到的绿框（在原始分辨率上画，UI层会自动缩放）
                    cv::rectangle(displayImage, updatedTrackingBox, cv::Scalar(0, 255, 0), 4, 1);
                }
            } else if (usePresetBoxes) {
                // 如果追踪失败，退化为固定框
                cv::rectangle(displayImage, detectionBox, cv::Scalar(0, 255, 0), 4, 1);
            }

            // 发送给 UI 显示（代替了之前的 imshow 弹窗）
            emit signal_messImage(displayImage);

        } catch (...) {
            qDebug() << "Exception in run loop";
        }
        msleep(100);
    }
}

void MyThread::startTracking() { m_tracking.store(true); lastDetectionTime = std::chrono::steady_clock::now(); }
void MyThread::stopTracking() { m_tracking.store(false); multiTracker = cv::MultiTracker::create(); }
bool MyThread::CheckRisingEdge() { return false; /* 保持原接口 */ }
