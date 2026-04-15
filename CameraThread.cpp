// CameraThread.cpp
#include "CameraThread.h"
#include <QDebug>

CameraThread::CameraThread(QObject *parent, CMvCamera *camera) :
    QThread(parent), m_pcMyCamera(camera), m_running(true), zhuizong(std::make_unique<Zhuizong>()), colorc(0)
{
    multiTracker = cv::MultiTracker::create();
    presetDetectionBox = cv::Rect2d(0, 0, 0, 0);
    presetTrackingBox = cv::Rect2d(0, 0, 0, 0);
    usePresetBoxes = false;
}

CameraThread::~CameraThread() { m_running = false; wait(); }

void CameraThread::setPresetBoxes(const cv::Rect2d& detBox, const cv::Rect2d& trackBox) {
    presetDetectionBox = detBox;
    presetTrackingBox = trackBox;
    usePresetBoxes = true;
}

void CameraThread::clearPresetBoxes() { usePresetBoxes = false; }
void CameraThread::received(QString data) { receivedata = data; }
void CameraThread::receiveangle1(int a) { angle2 = a; }
void CameraThread::receivecolorchannel(int c) { colorc = c; }
void CameraThread::stopTracking() { tracking = false; multiTracker = cv::MultiTracker::create(); }

void CameraThread::forceStop() {
    requestStop();
    if (!wait(5000)) qDebug() << "WARNING: Thread did not finish in 5 seconds";
}

void CameraThread::requestStop() {
    m_stopRequested.store(true);
    m_running = false;
    if (m_pcMyCamera) {
        try { m_pcMyCamera->requestStop(); } catch (...) {}
    }
}

bool CameraThread::CheckRisingEdge() {
    bool previousState = false, currentState = false, inputStatus;
    if (m_pcMyCamera->GetBoolValue("LineStatus", &inputStatus) != MV_OK) return false;
    currentState = (inputStatus == true);
    bool risingEdgeDetected = (!previousState && currentState);
    previousState = currentState;
    return risingEdgeDetected;
}

void CameraThread::run() {
    std::unique_ptr<cv::Mat> image = std::make_unique<cv::Mat>();
    m_pcMyCamera->setnonblocking(true);

    cv::Rect2d detectionBox = presetDetectionBox;
    cv::Rect2d initialDetectionBox = presetDetectionBox;
    cv::Rect2d trackingBox = presetTrackingBox;
    cv::Rect2d initialTrackingBox = presetTrackingBox;

    bool needInitTracker = (usePresetBoxes && presetDetectionBox.width > 0);
    tracking = false;

    while (m_running && !m_stopRequested.load()) {
        if (m_pcMyCamera) {
            try {
                *image = m_pcMyCamera->timesGetImage();
                if (image->empty()) { msleep(10); continue; }

                if (angle2 == 1) cv::rotate(*image, *image, cv::ROTATE_90_CLOCKWISE);
                else if (angle2 == 2) cv::rotate(*image, *image, cv::ROTATE_90_COUNTERCLOCKWISE);
                else if (angle2 == 3) cv::rotate(*image, *image, cv::ROTATE_180);

                if (colorc > 0 && image->channels() >= 3) {
                    std::vector<cv::Mat> channels;
                    cv::split(*image, channels);
                    if (colorc == 1) *image = channels[2];
                    else if (colorc == 2) *image = channels[1];
                    else if (colorc == 3) *image = channels[0];
                }

                if (needInitTracker && !tracking) {
                    cv::Rect imageRect(0, 0, image->cols, image->rows);
                    cv::Rect trackBoxInt(trackingBox.x, trackingBox.y, trackingBox.width, trackingBox.height);
                    if ((trackBoxInt & imageRect) == trackBoxInt) {
                        multiTracker = cv::MultiTracker::create();
                        multiTracker->add(cv::TrackerMOSSE::create(), *image, trackingBox);
                        tracking = true;
                        needInitTracker = false;
                        emit signal_boxesSelected(detectionBox, trackingBox);
                        m_pcMyCamera->deferswitchtoblockingafternextframe(); // 通知驱动
                    } else {
                        needInitTracker = false;
                    }
                }

                cv::Mat displayImage = image->clone();

                if (tracking) {
                    multiTracker->update(*image);
                    if (multiTracker->getObjects().size() > 0) {
                        cv::Rect2d updatedTrackingBox = multiTracker->getObjects()[0];
                        cv::Point2d displacement(updatedTrackingBox.x - initialTrackingBox.x, updatedTrackingBox.y - initialTrackingBox.y);
                        detectionBox.x = initialDetectionBox.x + displacement.x;
                        detectionBox.y = initialDetectionBox.y + displacement.y;

                        // 只有当相机缓冲区准备好，才发起真实识别
                        if (m_pcMyCamera->isImageReadyForMain()) {
                            emit signal_cleanlabel();
                            emit signal_sendForDetection(image->clone(), detectionBox);
                        }
                        cv::rectangle(displayImage, updatedTrackingBox, cv::Scalar(0, 255, 0), 4, 1);
                    }
                } else if (usePresetBoxes) {
                    cv::rectangle(displayImage, detectionBox, cv::Scalar(0, 255, 0), 4, 1);
                }

                // 将画好绿框的图像发给 UI (告别弹窗)
                emit signal_messImage(displayImage);

            } catch (...) {
                msleep(10); continue;
            }
        }

        // 睡眠缓冲
        int sleepTime = receivedata.toInt();
        if (sleepTime <= 0) sleepTime = 100;
        int totalSleep = 0;
        while (totalSleep < sleepTime && !m_stopRequested.load()) {
            msleep(10);
            totalSleep += 10;
        }
    }
}
