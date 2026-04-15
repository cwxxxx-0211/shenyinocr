#ifndef IMAGEMANIPULATOR_H
#define IMAGEMANIPULATOR_H

#include <opencv2/opencv.hpp>
#include <QMap>
#include<QImage>
#include <opencv2/opencv.hpp>
using namespace std;
class ImageManipulator
{
public:
    ImageManipulator();
    ~ImageManipulator();

    void processImage(cv::Mat& originalImage, cv::Mat& processedImage, const QMap<QString, bool>& settings);

private:
    void rotateImage(cv::Mat& src, cv::Mat& dst, const QString& rotationKey);
    void flipImage(cv::Mat& src, cv::Mat& dst, const QString& flipKey);

};

#endif // IMAGEMANIPULATOR_H
