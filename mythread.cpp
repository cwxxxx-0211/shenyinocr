// mythread.cpp
#include "mythread.h"
#include <QDebug>
#include <chrono>

MyThread::MyThread(QObject *parent)
    : QThread{parent}, myImage(new QImage()), zhuizong(new Zhuizong),
      cameraPtr(nullptr), imagePtr(nullptr), angle1(0), colorc1(0),
      m_stopRequested(false), m_tracking(false)
{

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

    // 🔥 修复点 1：优先检查是否有加载好的模板，不再盲目设为 false
    if (!m_trackingTemplate.empty()) {
        m_tracking.store(true);
    } else {
        m_tracking.store(false);
    }

    cv::Rect2d detectionBox = presetDetectionBox;
    cv::Rect2d initialDetectionBox = presetDetectionBox;
    cv::Rect2d trackingBox = presetTrackingBox;
    cv::Rect2d initialTrackingBox = presetTrackingBox;

    bool needInitTracker = (usePresetBoxes && presetDetectionBox.width > 0 && m_trackingTemplate.empty());
    lastDetectionTime = std::chrono::steady_clock::now(); //

    while (cameraPtr && !m_stopRequested.load()) {
        try {
            cameraPtr->CommandExecute("TriggerSoftware"); //
            *imagePtr = cameraPtr->timesGetImage(); //
            if (imagePtr->empty()) { msleep(10); continue; }

            // 图像旋转与通道处理
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

            // 初始化基准模板
            if (needInitTracker && !m_tracking.load()) {
                cv::Rect imageRect(0, 0, imagePtr->cols, imagePtr->rows);
                cv::Rect trackBoxInt(trackingBox.x, trackingBox.y, trackingBox.width, trackingBox.height);
                if ((trackBoxInt & imageRect) == trackBoxInt) {
                    m_trackingTemplate = (*imagePtr)(trackBoxInt).clone();
                    m_tracking.store(true);
                    needInitTracker = false;
                    emit signal_boxesSelected(detectionBox, trackingBox); //
                } else { needInitTracker = false; }
            }

            cv::Mat displayImage = imagePtr->clone();

            // ================== 静态模板匹配追踪 ==================
            if (m_tracking.load() && !m_trackingTemplate.empty()) {
                // 🔥 修复点 2：加大搜索区域至 200 像素，防止位移过快丢失
                int margin = 200;
                cv::Rect searchRoi(
                    initialTrackingBox.x - margin,
                    initialTrackingBox.y - margin,
                    initialTrackingBox.width + margin * 2,
                    initialTrackingBox.height + margin * 2
                );
                searchRoi &= cv::Rect(0, 0, imagePtr->cols, imagePtr->rows);

                if (searchRoi.width >= m_trackingTemplate.cols && searchRoi.height >= m_trackingTemplate.rows) {
                    cv::Mat matchResult;
                    cv::matchTemplate((*imagePtr)(searchRoi), m_trackingTemplate, matchResult, cv::TM_CCOEFF_NORMED); //

                    double maxVal; cv::Point maxLoc;
                    cv::minMaxLoc(matchResult, nullptr, &maxVal, nullptr, &maxLoc);

                    // 🔥 修复点 3：降低匹配阈值至 0.45
                    if (maxVal > 0.45) {
                        cv::Rect2d curTrackingBox(searchRoi.x + maxLoc.x, searchRoi.y + maxLoc.y,
                                                 initialTrackingBox.width, initialTrackingBox.height);

                        cv::Point2d diff(curTrackingBox.x - initialTrackingBox.x, curTrackingBox.y - initialTrackingBox.y);
                        detectionBox.x = initialDetectionBox.x + diff.x;
                        detectionBox.y = initialDetectionBox.y + diff.y;

                        // 节拍检测逻辑
                        auto now = std::chrono::steady_clock::now();
                        int interval = receivedata.toInt();
                        if (interval <= 0) interval = 300;
                        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastDetectionTime).count() >= interval) {
                            emit signal_cleanlabel(); //
                            emit signal_sendForDetection(imagePtr->clone(), detectionBox); //
                            lastDetectionTime = now;
                        }
                        cv::rectangle(displayImage, curTrackingBox, cv::Scalar(0, 255, 0), 4, 1);
                    } else {
                        // 匹配失败警告
                        cv::rectangle(displayImage, initialTrackingBox, cv::Scalar(0, 0, 255), 4, 1);
                    }
                }
            } else if (usePresetBoxes) {
                cv::rectangle(displayImage, detectionBox, cv::Scalar(0, 255, 0), 4, 1);
            }

            emit signal_messImage(displayImage); //

        } catch (...) { qDebug() << "Exception in run loop"; }
        msleep(100);
    }
}

void MyThread::startTracking() { m_tracking.store(true); lastDetectionTime = std::chrono::steady_clock::now(); }
void MyThread::stopTracking() {
    m_tracking.store(false);
    // 彻底清空内存中的静态模板，防止影响下一次启动
    if (!m_trackingTemplate.empty()) {
        m_trackingTemplate.release();
    }
}
bool MyThread::CheckRisingEdge() { return false; /* 保持原接口 */ }
