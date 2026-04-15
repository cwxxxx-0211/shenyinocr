#include "imagemanipulator.h"
#include<iostream>
#include<string.h>
#include<QDebug>
#include <memory>
#include "widget.h"
#include "ui_widget.h"
#include<opencv.hpp>

using namespace std;
ImageManipulator::ImageManipulator()
{
}

ImageManipulator::~ImageManipulator()
{
}

void ImageManipulator::processImage(cv::Mat& originalImage, cv::Mat& processedImage, const QMap<QString, bool>& settings)
{
    if (settings.contains("switchleft") && settings["switchleft"]) {
        rotateImage(originalImage, processedImage, "switchleft");
    } else if (settings.contains("switchright") && settings["switchright"]) {
        rotateImage(originalImage, processedImage, "switchright");
    } else if (settings.contains("switchover") && settings["switchover"]) {
        rotateImage(originalImage, processedImage, "switchover");
    } else {
        processedImage = originalImage.clone();
    }
    if (settings.contains("leftright") && settings["leftright"]) {
        flipImage(processedImage, processedImage, "leftright");
    }
    if (settings.contains("updown") && settings["updown"]) {
        flipImage(processedImage, processedImage, "updown");
    }
}

void ImageManipulator::rotateImage(cv::Mat& src, cv::Mat& dst, const QString& rotationKey)
{
    int rotationType = cv::ROTATE_90_CLOCKWISE;
    if (rotationKey == "switchleft") {
        rotationType = cv::ROTATE_90_COUNTERCLOCKWISE;
    } else if (rotationKey == "switchover") {
        rotationType = cv::ROTATE_180;
    }
    cv::rotate(src, dst, rotationType);
}






void ImageManipulator::flipImage(cv::Mat& src, cv::Mat& dst, const QString& flipKey)
{
    int flipCode = 0;
    if (flipKey == "leftright") {
        flipCode = 1;
    } else if (flipKey == "updown") {
        flipCode = 0;
    }
    cv::flip(src, dst, flipCode);
}
