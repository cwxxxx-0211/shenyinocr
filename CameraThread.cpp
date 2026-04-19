// CameraThread.cpp
#include "CameraThread.h"
#include <QDebug>

CameraThread::CameraThread(QObject *parent, CMvCamera *camera) :
    QThread(parent), m_pcMyCamera(camera), m_running(true), zhuizong(std::make_unique<Zhuizong>()), colorc(0)
{

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

void CameraThread::run() {
    std::unique_ptr<cv::Mat> image = std::make_unique<cv::Mat>();
    m_pcMyCamera->setnonblocking(true); //

    cv::Rect2d detectionBox = presetDetectionBox;
    cv::Rect2d initialDetectionBox = presetDetectionBox;
    cv::Rect2d trackingBox = presetTrackingBox;
    cv::Rect2d initialTrackingBox = presetTrackingBox;

    // 🔥 修复点 1：根据是否有预载模板决定初始状态，不再强制覆盖为 false
    if (!m_trackingTemplate.empty()) {
        tracking = true;
    } else {
        tracking = false;
    }

    // 只有在没模板且使用了预设框时才需要首帧初始化
    bool needInitTracker = (usePresetBoxes && presetDetectionBox.width > 0 && m_trackingTemplate.empty());

    while (m_running && !m_stopRequested.load()) {
        if (m_pcMyCamera) {
            try {
                *image = m_pcMyCamera->timesGetImage(); //
                if (image->empty()) { msleep(10); continue; }

                // 图像预处理（旋转与颜色通道）
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

                // 动态初始化静态模板 (仅在启动时无预载模板的情况下触发一次)
                if (needInitTracker && !tracking) {
                    cv::Rect imageRect(0, 0, image->cols, image->rows);
                    cv::Rect trackBoxInt(trackingBox.x, trackingBox.y, trackingBox.width, trackingBox.height);

                    if ((trackBoxInt & imageRect) == trackBoxInt) {
                        m_trackingTemplate = (*image)(trackBoxInt).clone();
                        tracking = true;
                        needInitTracker = false;
                        emit signal_boxesSelected(detectionBox, trackingBox); //
                        m_pcMyCamera->deferswitchtoblockingafternextframe(); //
                    } else {
                        needInitTracker = false;
                    }
                }

                cv::Mat displayImage = image->clone();

                // ================== 静态模板匹配追踪 ==================
                if (tracking && !m_trackingTemplate.empty()) {
                    // 🔥 修复点 2：加大搜索区域至 200 像素，大幅提高捕捉成功率
                    int margin = 200;
                    cv::Rect searchRoi(
                        initialTrackingBox.x - margin,
                        initialTrackingBox.y - margin,
                        initialTrackingBox.width + margin * 2,
                        initialTrackingBox.height + margin * 2
                    );
                    searchRoi &= cv::Rect(0, 0, image->cols, image->rows); // 边界安全检查

                    if (searchRoi.width >= m_trackingTemplate.cols && searchRoi.height >= m_trackingTemplate.rows) {
                        cv::Mat searchArea = (*image)(searchRoi);
                        cv::Mat matchResult;
                        cv::matchTemplate(searchArea, m_trackingTemplate, matchResult, cv::TM_CCOEFF_NORMED); //

                        double minVal, maxVal;
                        cv::Point minLoc, maxLoc;
                        cv::minMaxLoc(matchResult, &minVal, &maxVal, &minLoc, &maxLoc);

                        // 🔥 修复点 3：降低匹配阈值至 0.45，增加环境耐受度
                        if (maxVal > 0.45) {
                            cv::Rect2d updatedTrackingBox(
                                searchRoi.x + maxLoc.x,
                                searchRoi.y + maxLoc.y,
                                initialTrackingBox.width,
                                initialTrackingBox.height
                            );

                            // 计算位移并同步应用到检测框
                            cv::Point2d displacement(
                                updatedTrackingBox.x - initialTrackingBox.x,
                                updatedTrackingBox.y - initialTrackingBox.y
                            );

                            detectionBox.x = initialDetectionBox.x + displacement.x;
                            detectionBox.y = initialDetectionBox.y + displacement.y;

                            if (m_pcMyCamera->isImageReadyForMain()) {
                                emit signal_cleanlabel(); //
                                emit signal_sendForDetection(image->clone(), detectionBox); //
                            }
                            cv::rectangle(displayImage, updatedTrackingBox, cv::Scalar(0, 255, 0), 4, 1);
                        } else {
                            // 匹配失败时绘制红色警示框
                            cv::rectangle(displayImage, initialTrackingBox, cv::Scalar(0, 0, 255), 4, 1);
                        }
                    }
                } else if (usePresetBoxes) {
                    cv::rectangle(displayImage, detectionBox, cv::Scalar(0, 255, 0), 4, 1);
                }

                emit signal_messImage(displayImage); //

            } catch (...) { msleep(10); continue; }
        }

        // 线程睡眠逻辑
        int sleepTime = receivedata.toInt();
        if (sleepTime <= 0) sleepTime = 100;
        int totalSleep = 0;
        while (totalSleep < sleepTime && !m_stopRequested.load()) {
            msleep(10);
            totalSleep += 10;
        }
    }
}

void CameraThread::clearPresetBoxes() { usePresetBoxes = false; }
void CameraThread::received(QString data) { receivedata = data; }
void CameraThread::receiveangle1(int a) { angle2 = a; }
void CameraThread::receivecolorchannel(int c) { colorc = c; }
void CameraThread::stopTracking() {
    tracking = false;
    // 彻底清空内存中的静态模板，防止影响下一次启动
    if (!m_trackingTemplate.empty()) {
        m_trackingTemplate.release();
    }
}

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

