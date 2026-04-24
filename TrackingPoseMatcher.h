#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include "TrackingTypes.h"

class TrackingPoseMatcher
{
public:
    bool init(const cv::Mat& trackingTemplateBgr);
    void clear();
    bool isReady() const;
    DetectionPose match(const cv::Mat& frameBgr,
                        const std::vector<cv::Point2f>& relDatePoly) const;

private:
    cv::Mat templateGray;
    std::vector<cv::Mat> preRotatedTemplates;
    std::vector<int> preRotatedAngles;
    std::vector<cv::Mat> preRotatedTemplatesSmall;
    double pyramidScale = 0.2;
};
