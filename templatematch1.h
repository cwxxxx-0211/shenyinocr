#ifndef TEMPLATEMATCH1_H
#define TEMPLATEMATCH1_H

#include <opencv2/opencv.hpp>
#include<opencv2/highgui.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <time.h>
#include <QtCore>
#include <QObject>
#include <opencv2/features2d.hpp>
#include <opencv2/imgcodecs.hpp>

using namespace cv;
using namespace std;

class TemplateMatch1:public QThread
{
    Q_OBJECT
public:
    explicit TemplateMatch1(QObject* parent = nullptr) ;
    cv::Mat img;
    Mat *img1=nullptr;
    Mat *imgmuban=nullptr;
    Mat *imgshibie=nullptr;

    cv::Mat templ,result;
    char window_title_1;
    char window_title_2;
    char window_title_3;
    int match_method;
    int kernelsize;
    int value;
    int width_min,width_max,height_min,height_max,block_size1,block_size2,fixedcols,fixedrows;
    int horizontalKernelsize ;
    int verticalKernelsize ;
    Mat imageWithBoxes;
    vector<Point> rectTopCenterPoints;
    String input="";
//    int max_Trackbar = 5;
    double getMSSIM(cv::Mat& img1, cv::Mat& img2);
    double getSimilarity(const cv::Mat &img1, const cv::Mat &img2);
    int calculateOverlapArea(const cv::Rect& rect1, const cv::Rect& rect2);
    vector<Mat> vertical_projection(Mat input_src);
    double estimateNoiseLevel(const cv::Mat& src, int blockSize2);
    void sauvolaBinarization(cv::InputArray _src, cv::OutputArray _dst,
                            int windowSize, double k, int R);
    void splitWatershedRegions(const cv::Mat& markers,
                             std::vector<cv::Mat>& regions,
                             int minArea);


signals:



public slots:
    double run1(vector<Mat> digitTemplates,vector<Mat> digitRigions);
    void jianceshibiestr(String string1);
    void extractDigits(const Mat &image, vector<Mat> &digitRegions);
    void caijiansize(int a,int b,int c,int d,int e,int f,int g, int h);
    void kernel(int a);
    void ssimvalue(int s);
};

#endif // TEMPLATEMATCH_H1
