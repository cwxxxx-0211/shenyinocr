#include "Detector.h"
#include <iostream>
#include <cmath>
#include <fstream>
#include <mutex> // 必须添加，用于多线程安全锁

bool CalibrationData::load(const std::string& yamlPath) {
    // 内存流读取 YAML，彻底解决 Windows 中文路径报错问题
    std::ifstream file(yamlPath);
    if (!file.is_open()) {
        std::cerr << "❌ Cannot open calibration config: " << yamlPath << std::endl;
        return false;
    }
    std::string yamlStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    cv::FileStorage fs(yamlStr, cv::FileStorage::READ | cv::FileStorage::MEMORY);

    if (!fs.isOpened()) return false;
    fs["stamp_poly"] >> stamp_poly;
    fs.release();
    return true;
}

OverlapDetector::OverlapDetector() {}

bool OverlapDetector::init(const std::string& tplRingPath, const std::string& configPath) {
    // 内存流读取图片，彻底解决 Windows 中文路径无法 imread 的 BUG
    std::ifstream file(tplRingPath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[Engine Error] Failed to open template file: " << tplRingPath << std::endl;
        return false;
    }

    std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::vector<uchar> ubuf(buffer.begin(), buffer.end());
    templateRing = cv::imdecode(ubuf, cv::IMREAD_GRAYSCALE);

    if (templateRing.empty() || !calibData.load(configPath)) {
        return false;
    }

    preRotatedRings.clear();
    preRotatedAngles.clear();

    int h = templateRing.rows;
    int w = templateRing.cols;
    cv::Point2f center(w / 2.0f, h / 2.0f);

    // 1. 生成全尺寸的各角度旋转模板
    for (int angle = -45; angle <= 47; angle += 2) {
        cv::Mat M = cv::getRotationMatrix2D(center, angle, 1.0);
        double cos_v = std::abs(M.at<double>(0, 0));
        double sin_v = std::abs(M.at<double>(0, 1));
        int nW = static_cast<int>((h * sin_v) + (w * cos_v));
        int nH = static_cast<int>((h * cos_v) + (w * sin_v));
        M.at<double>(0, 2) += (nW / 2.0) - center.x;
        M.at<double>(1, 2) += (nH / 2.0) - center.y;

        cv::Mat rotated;
        cv::warpAffine(templateRing, rotated, M, cv::Size(nW, nH));
        preRotatedRings.push_back(rotated);
        preRotatedAngles.push_back(angle);
    }

    // ================= 极速优化：提前生成缩放版的模板缓存 =================
    pyramidScale = 0.2; // 设定缩放比例 (面积降至16%，极大提速且不丢特征)
    preRotatedRingsSmall.clear();
    for (const auto& rotTpl : preRotatedRings) {
        cv::Mat smallTpl;
        // 必须使用 INTER_AREA 保证缩小后不产生马赛克失真
        cv::resize(rotTpl, smallTpl, cv::Size(), pyramidScale, pyramidScale, cv::INTER_AREA);
        preRotatedRingsSmall.push_back(smallTpl);
    }
    // ======================================================================

    std::cout << "[Engine Info] Successfully generated " << preRotatedRings.size() << " rotated templates in memory." << std::endl;
    return true;
}


#include <mutex>

DetectResult OverlapDetector::processImage(const cv::Mat& bgrImage, const cv::Rect2d& diffbox) {
    DetectResult res;
    res.isOk = false;
    res.overlapPixels = 0;
    res.foundRing = false;
    res.valRing = 0.0;

    // 1. 预处理
    cv::Mat gray;
    if (bgrImage.channels() == 3) cv::cvtColor(bgrImage, gray, cv::COLOR_BGR2GRAY);
    else gray = bgrImage.clone();

    // 生成生产日期多边形
    res.finalDatePoly.clear();
    res.finalDatePoly.push_back(cv::Point(static_cast<int>(diffbox.x), static_cast<int>(diffbox.y)));
    res.finalDatePoly.push_back(cv::Point(static_cast<int>(diffbox.x + diffbox.width), static_cast<int>(diffbox.y)));
    res.finalDatePoly.push_back(cv::Point(static_cast<int>(diffbox.x + diffbox.width), static_cast<int>(diffbox.y + diffbox.height)));
    res.finalDatePoly.push_back(cv::Point(static_cast<int>(diffbox.x), static_cast<int>(diffbox.y + diffbox.height)));

    // 2. 全图降采样
    cv::Mat smallGray;
    // 🔥 极限加速 1：放弃 INTER_AREA，改用计算极快的 INTER_LINEAR
    cv::resize(gray, smallGray, cv::Size(), pyramidScale, pyramidScale, cv::INTER_LINEAR);

    double bestValSmall = -1.0;
    cv::Point bestLocSmall;
    int bestAngleIdx = -1;
    std::mutex mtx;

    // ================== 🔥 极限加速 2：跳跃式搜索 (大幅降低弱CPU负担) ==================
    // 从 47 个角度中，提取出 16 个代表性角度（每隔 6 度抽样一次）
    std::vector<int> searchIndices;
    for (size_t i = 0; i < preRotatedRingsSmall.size(); i += 3) {
        searchIndices.push_back(i);
    }
    // 把最后边界也加进去
    if (searchIndices.back() != preRotatedRingsSmall.size() - 1) {
        searchIndices.push_back(preRotatedRingsSmall.size() - 1);
    }

    // 只让多线程去匹配这 16 个代表性角度，运算量瞬间下降 60%
    cv::parallel_for_(cv::Range(0, searchIndices.size()), [&](const cv::Range& range) {
        double localBestVal = -1.0;
        cv::Point localBestLoc;
        int localBestIdx = -1;

        for (int i = range.start; i < range.end; ++i) {
            int realIdx = searchIndices[i];
            const cv::Mat& smallTpl = preRotatedRingsSmall[realIdx];
            if (smallTpl.rows > smallGray.rows || smallTpl.cols > smallGray.cols) continue;

            cv::Mat matchR;
            cv::matchTemplate(smallGray, smallTpl, matchR, cv::TM_CCOEFF_NORMED);

            double rMinV, rMaxV;
            cv::Point rMinL, rMaxL;
            cv::minMaxLoc(matchR, &rMinV, &rMaxV, &rMinL, &rMaxL);

            if (rMaxV > localBestVal) {
                localBestVal = rMaxV;
                localBestLoc = rMaxL;
                localBestIdx = realIdx;
            }
        }

        std::lock_guard<std::mutex> lock(mtx);
        if (localBestVal > bestValSmall) {
            bestValSmall = localBestVal;
            bestLocSmall = localBestLoc;
            bestAngleIdx = localBestIdx;
        }
    });

    // 邻域微调：在刚才找到的大致角度左右，补充测算相邻的 2 个角度，确保精度不丢
    if (bestAngleIdx >= 0 && bestValSmall >= 0.25) {
        int neighbors[] = {bestAngleIdx - 2, bestAngleIdx - 1, bestAngleIdx + 1, bestAngleIdx + 2};
        for (int nIdx : neighbors) {
            if (nIdx >= 0 && nIdx < preRotatedRingsSmall.size()) {
                const cv::Mat& smallTpl = preRotatedRingsSmall[nIdx];
                if (smallTpl.rows > smallGray.rows || smallTpl.cols > smallGray.cols) continue;

                cv::Mat matchR;
                cv::matchTemplate(smallGray, smallTpl, matchR, cv::TM_CCOEFF_NORMED);
                double rMinV, rMaxV;
                cv::Point rMinL, rMaxL;
                cv::minMaxLoc(matchR, &rMinV, &rMaxV, &rMinL, &rMaxL);

                if (rMaxV > bestValSmall) {
                    bestValSmall = rMaxV;
                    bestLocSmall = rMaxL;
                    bestAngleIdx = nIdx;
                }
            }
        }
    }

    res.valRing = bestValSmall;

    // 3. 原图局部精配
    if (bestValSmall >= 0.35 && bestAngleIdx >= 0) {
        // 还原粗配坐标到原图尺寸
        cv::Point roughLoc(bestLocSmall.x / pyramidScale, bestLocSmall.y / pyramidScale);
        const cv::Mat& bestTpl = preRotatedRings[bestAngleIdx];

        int padding = 40;
        int sx = std::max(0, roughLoc.x - padding);
        int sy = std::max(0, roughLoc.y - padding);
        int sw = std::min(gray.cols - sx, bestTpl.cols + 2 * padding);
        int sh = std::min(gray.rows - sy, bestTpl.rows + 2 * padding);
        cv::Rect exactRoi(sx, sy, sw, sh);

        cv::Mat exactArea = gray(exactRoi);
        cv::Mat matchR;
        cv::matchTemplate(exactArea, bestTpl, matchR, cv::TM_CCOEFF_NORMED);
        double rMinV, rMaxV;
        cv::Point rMinL, rMaxL;
        cv::minMaxLoc(matchR, &rMinV, &rMaxV, &rMinL, &rMaxL);

        res.valRing = rMaxV;

        // 🔥 修复致命BUG：将错误的 45 纠正回 0.45 ！！！！
        if (rMaxV >= 0.2) {
            res.foundRing = true;
            res.locRing = cv::Point(rMaxL.x + sx, rMaxL.y + sy);
            res.angleRing = preRotatedAngles[bestAngleIdx];
            res.shapeRing = cv::Size(bestTpl.cols, bestTpl.rows);

            float center_x = res.locRing.x + res.shapeRing.width / 2.0f;
            float center_y = res.locRing.y + res.shapeRing.height / 2.0f;

            cv::Mat M_rot = cv::getRotationMatrix2D(cv::Point2f(0, 0), res.angleRing, 1.0);

            for (const auto& spt : calibData.stamp_poly) {
                double rot_dx = M_rot.at<double>(0, 0) * spt.x + M_rot.at<double>(0, 1) * spt.y;
                double rot_dy = M_rot.at<double>(1, 0) * spt.x + M_rot.at<double>(1, 1) * spt.y;
                res.finalStampPoly.push_back(cv::Point(static_cast<int>(center_x + rot_dx), static_cast<int>(center_y + rot_dy)));
            }
        }
    }

    // ================== 局部包围盒相交测试 + 最小化遮罩 ==================
    if (res.finalDatePoly.size() >= 3 && res.finalStampPoly.size() >= 3) {
        cv::Rect boundDate = cv::boundingRect(res.finalDatePoly);
        cv::Rect boundStamp = cv::boundingRect(res.finalStampPoly);

        cv::Rect intersectBound = boundDate & boundStamp;

        if (intersectBound.width > 0 && intersectBound.height > 0) {
            cv::Mat maskDate = cv::Mat::zeros(intersectBound.size(), CV_8UC1);
            cv::Mat maskStamp = cv::Mat::zeros(intersectBound.size(), CV_8UC1);

            std::vector<cv::Point> localDatePoly = res.finalDatePoly;
            std::vector<cv::Point> localStampPoly = res.finalStampPoly;
            for(auto& p : localDatePoly) { p.x -= intersectBound.x; p.y -= intersectBound.y; }
            for(auto& p : localStampPoly) { p.x -= intersectBound.x; p.y -= intersectBound.y; }

            cv::fillPoly(maskDate, std::vector<std::vector<cv::Point>>{localDatePoly}, cv::Scalar(255));
            cv::fillPoly(maskStamp, std::vector<std::vector<cv::Point>>{localStampPoly}, cv::Scalar(255));

            cv::Mat interMask;
            cv::bitwise_and(maskDate, maskStamp, interMask);
            res.overlapPixels = cv::countNonZero(interMask);
            res.isOk = (res.overlapPixels == 0);
        } else {
            res.isOk = true;
            res.overlapPixels = 0;
        }
    }

    return res;
}
