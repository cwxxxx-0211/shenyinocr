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

void CameraThread::run() {
    std::unique_ptr<cv::Mat> image = std::make_unique<cv::Mat>();
    m_pcMyCamera->setnonblocking(true);

    cv::Rect2d detectionBox = presetDetectionBox;
    cv::Rect2d initialDetectionBox = presetDetectionBox;
    cv::Rect2d trackingBox = presetTrackingBox;
    cv::Rect2d initialTrackingBox = presetTrackingBox;

    // 状态标记
    bool needInitTracker = (usePresetBoxes && presetDetectionBox.width > 0);
    tracking = false;

    while (m_running && !m_stopRequested.load()) {
        if (m_pcMyCamera) {
            try {
                *image = m_pcMyCamera->timesGetImage();
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

                // ================== 核心修改：初始化静态模板 ==================
                if (needInitTracker && !tracking) {
                    cv::Rect imageRect(0, 0, image->cols, image->rows);
                    cv::Rect trackBoxInt(trackingBox.x, trackingBox.y, trackingBox.width, trackingBox.height);

                    if ((trackBoxInt & imageRect) == trackBoxInt) {
                        // 截取当前区域作为永久基准模板，不再更新
                        m_trackingTemplate = (*image)(trackBoxInt).clone();
                        tracking = true;
                        needInitTracker = false;
                        emit signal_boxesSelected(detectionBox, trackingBox);
                        m_pcMyCamera->deferswitchtoblockingafternextframe();
                    } else {
                        needInitTracker = false;
                    }
                }

                cv::Mat displayImage = image->clone();

                // ================== 核心修改：静态模板匹配追踪 ==================
                if (tracking && !m_trackingTemplate.empty()) {
                    // 设定搜索区域：在初始位置基础上向四周外扩 100 像素
                    int margin = 100;
                    cv::Rect searchRoi(
                        initialTrackingBox.x - margin,
                        initialTrackingBox.y - margin,
                        initialTrackingBox.width + margin * 2,
                        initialTrackingBox.height + margin * 2
                    );
                    searchRoi &= cv::Rect(0, 0, image->cols, image->rows); // 边界检查

                    if (searchRoi.width >= m_trackingTemplate.cols && searchRoi.height >= m_trackingTemplate.rows) {
                        cv::Mat searchArea = (*image)(searchRoi);
                        cv::Mat matchResult;
                        cv::matchTemplate(searchArea, m_trackingTemplate, matchResult, cv::TM_CCOEFF_NORMED);

                        double minVal, maxVal;
                        cv::Point minLoc, maxLoc;
                        cv::minMaxLoc(matchResult, &minVal, &maxVal, &minLoc, &maxLoc);

                        // 设定相似度阈值（例如 0.6），低于此值认为没找到目标
                        if (maxVal > 0.6) {
                            // 计算当前帧目标的物理位置
                            cv::Rect2d updatedTrackingBox(
                                searchRoi.x + maxLoc.x,
                                searchRoi.y + maxLoc.y,
                                initialTrackingBox.width,
                                initialTrackingBox.height
                            );

                            // 计算相对于第一帧的位移
                            cv::Point2d displacement(
                                updatedTrackingBox.x - initialTrackingBox.x,
                                updatedTrackingBox.y - initialTrackingBox.y
                            );

                            // 同步修正检测框坐标
                            detectionBox.x = initialDetectionBox.x + displacement.x;
                            detectionBox.y = initialDetectionBox.y + displacement.y;

                            if (m_pcMyCamera->isImageReadyForMain()) {
                                emit signal_cleanlabel();
                                emit signal_sendForDetection(image->clone(), detectionBox);
                            }
                            cv::rectangle(displayImage, updatedTrackingBox, cv::Scalar(0, 255, 0), 4, 1);
                        } else {
                            // 匹配失败，显示初始框（红色警告）
                            cv::rectangle(displayImage, initialTrackingBox, cv::Scalar(0, 0, 255), 4, 1);
                        }
                    }
                } else if (usePresetBoxes) {
                    cv::rectangle(displayImage, detectionBox, cv::Scalar(0, 255, 0), 4, 1);
                }

                emit signal_messImage(displayImage);

            } catch (...) { msleep(10); continue; }
        }

        // 睡眠逻辑...
        int sleepTime = receivedata.toInt();
        if (sleepTime <= 0) sleepTime = 100;
        int totalSleep = 0;
        while (totalSleep < sleepTime && !m_stopRequested.load()) {
            msleep(10);
            totalSleep += 10;
        }
    }
}
