#include "templatematch1.h"
#include <iostream>
#include <QDebug>
#include <QObject>
#include <QMessageBox>
#include <opencv2/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>

using namespace std;
using namespace cv;
using namespace cv::xfeatures2d;

TemplateMatch1::TemplateMatch1(QObject* parent)
    : QThread(parent)
{
//    myThread = new MyThread();
//    cameraThread = new CameraThread();
//    qRegisterMetaType<cv::Mat>("cv::Mat");
//    qRegisterMetaType<cv::Mat*>("cv::Mat*");
}

//------------------------【1】基于轮廓特征的相似度计算--------------------------------//
// 与之前类似，只是多加一个参数 winNamePrefix 用于区分显示窗口
double TemplateMatch1::getSimilarity(const cv::Mat &img1, const cv::Mat &img2)
{
    // 1) 转灰度
    cv::Mat gray1, gray2;
    if (img1.channels() == 3) {
        cv::cvtColor(img1, gray1, cv::COLOR_BGR2GRAY);
    } else {
        gray1 = img1.clone();
    }

    if (img2.channels() == 3) {
        cv::cvtColor(img2, gray2, cv::COLOR_BGR2GRAY);
    } else {
        gray2 = img2.clone();
    }

    // 2) 高斯模糊（可根据实际情况调整核大小）
    cv::GaussianBlur(gray1, gray1, cv::Size(5, 5), 0);
    cv::GaussianBlur(gray2, gray2, cv::Size(5, 5), 0);

    // 3) 二值化（使用Otsu阈值）
    cv::Mat bin1, bin2;
    cv::threshold(gray1, bin1, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
    cv::threshold(gray2, bin2, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    // 4) 保证模板图不大于目标图，如若 bin2（模板）大于 bin1，就缩放 bin2 到与 bin1 同样大小
    if (bin2.cols > bin1.cols || bin2.rows > bin1.rows) {
        cv::resize(bin2, bin2, bin1.size());
    }

    // 5) 使用 matchTemplate 计算相似度
    // 注意 matchTemplate 中第二个参数为“模板”，第一个参数为“待匹配的图像”
    // 如果 bin2 是模板，就应该写成 matchTemplate(bin1, bin2, result, ...)
    cv::Mat result;
    cv::matchTemplate(bin1, bin2, result, cv::TM_CCOEFF_NORMED);

    // 6) matchTemplate 结果在 result 中，若和 bin1、bin2 大小正好相同，那么只会输出单个值
    double similarity = result.at<float>(0, 0);

    return similarity;
}


//------------------------【2】接收/存储外部图像--------------------------------//

// 这里如果你想把 int 型的 value 用作阈值，也可以保留
void TemplateMatch1::ssimvalue(int s)
{
    value = s;
}

int TemplateMatch1::calculateOverlapArea(const cv::Rect& rect1, const cv::Rect& rect2) {
    int overlapX = std::max(0, std::min(rect1.x + rect1.width, rect2.x + rect2.width) - std::max(rect1.x, rect2.x));
    int overlapY = std::max(0, std::min(rect1.y + rect1.height, rect2.y + rect2.height) - std::max(rect1.y, rect2.y));
    return overlapX * overlapY;
}




//垂直投影分割
//vector<Mat> TemplateMatch::vertical_projection(Mat input_src) //输入二值化图片
//{
//    /**************统计原图片中每列白色像素数目******************************/
//    blur(input_src, input_src, Size(3, 3));//模糊，去锯齿
//    int src_width = input_src.cols;
//    int src_height = input_src.rows;
//    int* projectValArry = new int[src_width]();//创建用于储存每列白色像素个数的数组
//    //memset(projectValArry, 0, src_width*4);//初始化数组
//    //取列白色像素个数
//    for (int i = 0; i < src_height; i++){
//        for (int j = 0; j < src_width; j++){
//            if (input_src.at<uchar>(i, j)){
//                projectValArry[j]++;
//            }
//        }
//    }
//    /**************将每列白色像素数目绘制成直方图***************************/
//    //定义画布 绘制垂直投影下每列白色像素的数目
//    Mat verticalProjectionMat(src_height, src_width, CV_8UC1, Scalar(0));
//    for (int i = 0; i< src_width; i++){
//        for (int j = 0; j < projectValArry[i]; j++){
//            verticalProjectionMat.at<uchar>(src_height-j-1,i) = 255;
//        }
//    }
//    imshow("verticalProjectionMat", verticalProjectionMat);

//    /*********根据每列白色像素数目设置截取起始和截止列***********************/
//    //定义Mat vector ，存储图片组
//    vector<Mat> split_src;
//    //定义标志，用来指示在白色像素区还是在全黑区域
//    bool white_block = 0, black_block = 0;
//    //定义列temp_col_forword  temp_col_behind，记录字符截取起始列和截止列
//    int temp_col_forword=0,temp_col_behind = 0;
//    Mat split_temp;
//    //遍历数组projectValArry
//    for (int i = 0; i < src_width; i++){
//        if (projectValArry[i]){//表示区域有白色像素
//            white_block = 1;
//            black_block = 0;
//        }
//        else{				//若无白色像素（进入黑色区域）
//            if (white_block == 1){//若前一列有白色像素
//                temp_col_behind = i;//取当前列为截止列
//                split_temp=input_src(Rect(temp_col_forword, 0, temp_col_behind - temp_col_forword, src_height)).clone();
//                split_src.push_back(split_temp);
//            }
//            temp_col_forword = i;//记录最新黑色区域的列号，记为起始列
//            black_block = 1;//表示进入黑色区域
//            white_block = 0;
//        }
//    }

//    waitKey(0);
//    return split_src;
//}


//void TemplateMatch::extractDigits(const cv::Mat &image, std::vector<cv::Mat> &digitRegions)
//{
//    // 1. 转灰度
//    cv::Mat gray;
//    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

//    // 2. 自适应二值化
//    cv::Mat binary;
//    cv::adaptiveThreshold(
//        gray,
//        binary,
//        255,
//        cv::ADAPTIVE_THRESH_GAUSSIAN_C,
//        cv::THRESH_BINARY_INV,
//        block_size1,
//        block_size2
//    );
//        // 4. 形态学去噪(开运算等)
//        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(kernelsize, kernelsize));
//        cv::morphologyEx(binary, binary, cv::MORPH_OPEN, kernel);


//        cv::imshow("Binarized Image", binary);
//        cv::imwrite("D:\\software\\QT\\project\\mobanpipei1\\muban\\p1.png", binary);


//        // 5. 连通域分析
//        cv::Mat labels, stats, centroids;
//        cv::connectedComponentsWithStats(binary, labels, stats, centroids, 8, CV_32S);
//        cv::imwrite("D:\\software\\QT\\project\\mobanpipei1\\muban\\p2.png", labels);


//        // 6. 过滤小面积连通域
//        cv::Mat cleanedBinary = cv::Mat::zeros(binary.size(), CV_8UC1);

//        for (int i = 1; i < stats.rows; ++i)  // 忽略背景标签0
//        {
//            int area = stats.at<int>(i, cv::CC_STAT_AREA);
//            if (area > 50) { // area阈值可根据实际需要调节
//                cleanedBinary.setTo(255, labels == i);
//            }
//        }

//       digitRegions=vertical_projection(cleanedBinary);

//}





////固定尺寸分割
//void TemplateMatch::extractDigits(const cv::Mat &image, std::vector<cv::Mat> &digitRegions)
//{


//    // 1. 转灰度
//    cv::Mat gray;
//    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);


//    // 2.中值滤波 去除空白部分噪声 防止对空白部分进行分割
//    cv::Mat filtered;
//    cv::GaussianBlur(gray, filtered, cv::Size(5, 5), 0); // 核大小和标准差可根据实际调整
//    cv::medianBlur(filtered, filtered, 5); // 3为滤波核大小，可根据噪声情况调整

//    cv::imshow("lvbo Image", filtered);

//    // 3. 自适应二值化
//    cv::Mat binary;
//    cv::adaptiveThreshold(
//        filtered,
//        binary,
//        255,
//        cv::ADAPTIVE_THRESH_GAUSSIAN_C,
//        cv::THRESH_BINARY_INV,
//        block_size1,
//        block_size2
//    );

//    // 4. 形态学去噪(开运算等)
//    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(kernelsize, kernelsize));
//    cv::morphologyEx(binary, binary, cv::MORPH_OPEN, kernel);




//    cv::imshow("Binarized Image", binary);
//    cv::imwrite("D:\\software\\QT\\project\\mobanpipei1\\muban\\p1.png", binary);


//    // 5. 连通域分析
//    cv::Mat labels, stats, centroids;
//    cv::connectedComponentsWithStats(binary, labels, stats, centroids, 8, CV_32S);


//    // 6. 过滤小面积连通域
//    cv::Mat cleanedBinary = cv::Mat::zeros(binary.size(), CV_8UC1);

//    for (int i = 1; i < stats.rows; ++i)  // 忽略背景标签0
//    {
//        int area = stats.at<int>(i, cv::CC_STAT_AREA);
//        if (area > 50) { // area阈值可根据实际需要调节
//            cleanedBinary.setTo(255, labels == i);
//        }
//    }
//     cv::imshow("liantong", cleanedBinary);
//    cv::imwrite("D:\\software\\QT\\project\\mobanpipei1\\muban\\p3.png", cleanedBinary);



//    // 7. 查找轮廓
//    std::vector<std::vector<cv::Point>> contours;
//    cv::findContours(cleanedBinary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
////    qDebug()<<"contours.size"<<contours.size();



//    // 8. 根据轮廓提取边界框，并基于宽高过滤
//    std::vector<cv::Rect> boundingBoxes;
//    for (const auto &contour : contours)
//        {
//            cv::Rect bbox = cv::boundingRect(contour);

//            // 原外接矩形的左上角坐标
//            int originalTopLeftX = bbox.x;
//            int originalTopLeftY = bbox.y;

//            // 新矩形的左上角坐标等于原外接矩形的左上角坐标
//            int newX = originalTopLeftX;
//            int newY = originalTopLeftY;

//            // 确保新矩形在图像内
//            if (newX < 0) {
//                newX = 0;
//            }
//            if (newY < 0) {
//                newY = 0;
//            }
//            if (newX + fixedcols > cleanedBinary.cols) {
//                newX = cleanedBinary.cols - fixedcols;
//            }
//            if (newY + fixedrows > cleanedBinary.rows) {
//                newY = cleanedBinary.rows - fixedrows;
//            }

//            // 创建固定宽高的矩形
//            cv::Rect fixedBbox(newX, newY, fixedcols, fixedrows);


//            if ((bbox.x!= 0 || bbox.y!= 0) && bbox.width > 0 && bbox.height > 0 && bbox.width  > width_min  &&
//                                bbox.height > height_min &&
//                                bbox.width  < width_max  &&
//                               bbox.height < height_max) {
//                boundingBoxes.push_back(fixedBbox);
//            }
//        }


//    //9.排序（按行或者列）
//    int rowThreshold = 10; // 根据行距可调

//    // 1. 排序：先按行 (y 坐标)，再按列 (x 坐标)
//    std::sort(
//        boundingBoxes.begin(),
//        boundingBoxes.end(),
//        [rowThreshold](const cv::Rect &a, const cv::Rect &b) -> bool
//        {
//            // 若 a.y 与 b.y 距离小，视为同一行，则按 x 排序
//            if (std::abs(a.y - b.y) < rowThreshold)
//            {
//                return a.x < b.x; // 同一行，按 x 排序
//            }
//            else
//            {
//                // 否则，按 y 排序（谁在上谁优先）
//                return a.y < b.y;
//            }
//        }
//    );

//    // 2. 将同一行的矩形纵坐标统一为该行的最大 y 值
//    for (size_t i = 0; i < boundingBoxes.size();)
//    {
//        int currentRowY = boundingBoxes[i].y; // 当前行的初始 y 坐标
//        std::vector<size_t> sameRowIndices;  // 存储同一行的索引

//        // 找到属于同一行的所有矩形
//        for (size_t j = i; j < boundingBoxes.size(); ++j)
//        {
//            if (std::abs(boundingBoxes[j].y - currentRowY) < rowThreshold)
//            {
//                sameRowIndices.push_back(j); // 同一行
//            }
//            else
//            {
//                break; // 超过 rowThreshold，属于下一行
//            }
//        }

//        // 找到同一行中最小的 y 坐标
//        int minY = currentRowY;
//        for (size_t idx : sameRowIndices)
//        {

//           minY = std::min(minY, boundingBoxes[idx].y); // 更新最大 y
//        }



//        // 将同一行的矩形 y 坐标设置为 `minY`
//        for (size_t idx : sameRowIndices)
//        {
//            boundingBoxes[idx].y = minY; // 统一 y 坐标
//        }

//        // 跳过当前行的所有矩形
//        i += sameRowIndices.size();
//    }

//        // 过滤有重叠的矩形框
//      for (size_t i = 1; i < boundingBoxes.size(); ++i) {
//            bool shouldRemove = false;
//            for (size_t j = 0; j < i; ++j) {
//                int overlapArea = calculateOverlapArea(boundingBoxes[i], boundingBoxes[j]);
//                if (overlapArea > 50) {
//                    shouldRemove = true;
//                    break;
//                }
//            }
//            if (shouldRemove) {
//                boundingBoxes.erase(boundingBoxes.begin() + i);
//                --i; // 由于删除了一个元素，需要将索引减1，以确保不遗漏元素
//            }
//        }

//    // 10. 更新 rectTopCenterPoints，画检测框
//    rectTopCenterPoints.clear();   // 避免重复累加
//    imageWithBoxes = image.clone(); // 用于显示
//    for (const auto &bbox : boundingBoxes)
//    {
//        // 记录“正上方中心点”
//        cv::Point topCenter(bbox.x + bbox.width/2, bbox.y);
//        rectTopCenterPoints.push_back(topCenter);

//        // 绘制矩形框
//        cv::rectangle(imageWithBoxes, bbox, cv::Scalar(0,255,0), 1);
//    }
//    // 显示检测结果
//    cv::imshow("Detected Digit Regions", imageWithBoxes);
//    cv::imwrite("D:\\software\\QT\\project\\mobanpipei1\\muban\\imageWithBoxes.png", imageWithBoxes);

//    // 11. 最终裁剪 ROI 输出
//    digitRegions.clear(); // 避免重复累加
//    for (const auto &bbox : boundingBoxes)
//    {
//        cv::Mat roi = image(bbox).clone();
//        digitRegions.push_back(roi);
//    }

//}





//自适应分割
void TemplateMatch::extractDigits(const cv::Mat &image, std::vector<cv::Mat> &digitRegions)
{
    // 1. 转灰度
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    // 2.中值滤波 去除空白部分噪声 防止对空白部分进行分割
    cv::Mat filtered;
    cv::GaussianBlur(gray, filtered, cv::Size(5, 5), 0); // 核大小和标准差可根据实际调整
    cv::medianBlur(filtered, filtered, 5); // 3为滤波核大小，可根据噪声情况调整
    cv::imshow("windowbinary1",filtered);

    // 2. 自适应二值化
    cv::Mat binary;
    cv::adaptiveThreshold(
        filtered,
        binary,
        255,
        cv::ADAPTIVE_THRESH_GAUSSIAN_C,
        cv::THRESH_BINARY_INV,
        block_size1,
        block_size2
    );


    cv::imshow("windowbinary2",binary);
    // 3. 形态学去噪(开运算等)
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(kernelsize, kernelsize));
    cv::morphologyEx(binary, binary, cv::MORPH_OPEN, kernel);


    cv::imshow("windowbinary3",binary);


    // 纵向膨胀
    cv::Mat verticalKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(1, verticalKernelsize)); // 垂直矩形核
    cv::dilate(binary, binary, verticalKernel);

    // 横向膨胀
    cv::Mat horizontalKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(horizontalKernelsize, 1)); // 水平矩形核
    cv::dilate(binary, binary, horizontalKernel);

    cv::imshow("windowbinary4",binary);


    // 4. 连通域分析
    cv::Mat labels, stats, centroids;
    cv::connectedComponentsWithStats(binary, labels, stats, centroids, 8, CV_32S);

    // 5. 过滤小面积连通域
    cv::Mat cleanedBinary = cv::Mat::zeros(binary.size(), CV_8UC1);
    for (int i = 1; i < stats.rows; ++i)  // 忽略背景标签0
    {
        int area = stats.at<int>(i, cv::CC_STAT_AREA);
        if (area > 100) { // area阈值可根据实际需要调节
            cleanedBinary.setTo(255, labels == i);
        }
    }

    // 6. 查找轮廓
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(cleanedBinary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // 7. 根据轮廓提取边界框，并基于宽高过滤
    std::vector<cv::Rect> boundingBoxes;
    for (const auto &contour : contours)
    {
        cv::Rect bbox = cv::boundingRect(contour);
        // 宽高过滤：width_min, width_max, height_min, height_max 为你设定的阈值
        if (bbox.width  > width_min  &&
            bbox.height > height_min &&
            bbox.width  < width_max  &&
            bbox.height < height_max)
        {
            boundingBoxes.push_back(bbox);
        }
    }
    cv::imshow("windowbinary5",cleanedBinary);
    cv::imwrite("E:/product/p3.png", cleanedBinary);


    // =========================================================
    // 关键：对 boundingBoxes 进行「先上到下，再左到右」排序
    // =========================================================
    int rowThreshold = 5; // 根据行距可调

 //    比较器：若 y 距离近 => 看 x；否则看 y
    std::sort(
        boundingBoxes.begin(),
        boundingBoxes.end(),
        [rowThreshold](const cv::Rect &a, const cv::Rect &b) -> bool
        {
            // 若 a.y 与 b.y 距离小，视为同一行，则按 x 排序
            if (std::abs(a.y - b.y) < rowThreshold)
            {
                return a.x < b.x;
            }
            else
            {
                // 否则，谁 y 小谁在前（谁在上）
                return a.y < b.y;
            }
        }
    );

    // 定义列间距阈值，根据实际图像调整
//    int columnThreshold = 10;

    // 比较器：若 x 距离近 => 看 y；否则看 x
//    std::sort(
//        boundingBoxes.begin(),
//        boundingBoxes.end(),
//        [columnThreshold](const cv::Rect &a, const cv::Rect &b) -> bool
//        {
//            // 若 a.x 与 b.x 距离小，视为同一列，则按 y 排序
//            if (std::abs(a.x - b.x) < columnThreshold)
//            {
//                return a.y < b.y;
//            }
//            else
//            {
//                // 否则，谁 x 小谁在前（谁在左）
//                return a.x < b.x;
//            }
//        }
//    );

    // ---------------------------------------------------------
    // 如果你需要更稳健的“分行聚类”，可以改成以下的「先 y 初排 + 行聚类 + 行内 x 排序」方案
    // （在实战中更灵活一些，但这里先给出最简便的一步排序版本）
    // ---------------------------------------------------------

    // 8. 更新 rectTopCenterPoints，画检测框
    rectTopCenterPoints.clear();   // 避免重复累加
    imageWithBoxes = image.clone(); // 用于显示
    for (const auto &bbox : boundingBoxes)
    {
        // 记录“正上方中心点”
        cv::Point topCenter(bbox.x + bbox.width/2, bbox.y);
        rectTopCenterPoints.push_back(topCenter);

        // 绘制矩形框
        cv::rectangle(imageWithBoxes, bbox, cv::Scalar(0,255,0), 2);
    }
    // 显示检测结果
    cv::imshow("Detected Digit Regions", imageWithBoxes);

    // 9. 最终裁剪 ROI 输出
    digitRegions.clear(); // 避免重复累加
    for (const auto &bbox : boundingBoxes)
    {
        cv::Mat roi = image(bbox).clone();
        digitRegions.push_back(roi);
    }
}






//------------------------【4】设置参数--------------------------------//
void TemplateMatch1::caijiansize(int a, int b, int c, int d, int e, int f, int g, int h)
{
    width_min   = a;
    width_max   = b;
    height_min  = c;
    height_max  = d;
    block_size1 = e;
    block_size2 = f;
//    fixedcols   = g;
//    fixedrows   = h;
    verticalKernelsize = g;
    horizontalKernelsize =h;

}

void TemplateMatch1::kernel(int a)
{
    kernelsize = a;
//    qDebug() << "kernelsize" << kernelsize;
}

//------------------------【5】主要匹配函数：1对1匹配ROI--------------------------------//
double TemplateMatch::run1(std::vector<Mat> digitTemplates,std::vector<Mat> digitRigions)
{
    // --------------------【1】提取ROI-------------------- //




    // 如果数字区域数量少于模板数量，则无法逐一匹配
    if (digitRigions.size() < digitTemplates.size()) {
        qDebug() << "[ERROR] digitRegions.size()=" << digitRigions.size()
                 << " digitTemplates.size()=" << digitTemplates.size();
        return 0.0;
    }

    int N = static_cast<int>(digitRigions.size());
    int M = static_cast<int>(digitTemplates.size());

    // SSIM 阈值(0~1)
    double thresholdSSIM = (double)value / 100.0;
    thresholdSSIM = std::min(std::max(thresholdSSIM, 0.0), 1.0); // 限制在 0~1

    // --------------------【A】搜索第1张模板的最佳匹配位置 -------------------- //
    int startIndex = -1;

    // 遍历所有ROI，找与第一张模板的相似度 >= thresholdSSIM 的那个
    for (int i = 0; i < N; i++)
    {
        // 调整大小，使模板大小适应ROI
        cv::Mat regionResized;
        cv::resize(digitRigions[i], regionResized, digitTemplates[0].size());

        // 直接用getSimilarity进行匹配
        double similarity = getSimilarity(regionResized, digitTemplates[0]);

        // 输出每个区域与第1张模板的匹配度 (可注释)
        // qDebug() << "[INFO] Matching score for ROI[" << i << "] with Template[0] = " << similarity;

        // 检查是否满足阈值
        if (similarity >= thresholdSSIM) {
            startIndex = i;
            // qDebug() << "[INFO] Found matching ROI[" << i << "] for Template[0] with similarity =" << similarity;
            break;
        }
    }

    // 如果没有找到满足阈值的匹配，默认从第一个区域开始
    if (startIndex == -1) {
        startIndex = 0;
//        qDebug() << "[WARN] No ROI matched the first template above the threshold. Starting from index 0.";
    }

    // 确保从startIndex开始，后续匹配不越界
    if (startIndex + M > N) {
        startIndex = N - M;
        if (startIndex < 0) startIndex = 0;
//        qDebug() << "[WARN] Adjusted startIndex to" << startIndex << "to prevent out-of-bounds.";
    }

    // --------------------【B】逐一匹配各张模板 -------------------- //
    double minScore = 1.0;           // 记录所有匹配的最小分(可选)
    std::vector<double> shapeVals(M, 0.0);

    for (int i = 0; i < M; i++)
    {
        int regionIndex = startIndex + i;
        // 确保不越界
        if (regionIndex >= N) {
            qDebug() << "[ERROR] regionIndex" << regionIndex << "is out of bounds. Skipping Template[" << i << "].";
            shapeVals[i] = 0.0;
            continue;
        }

        // 获取当前的目标ROI & 模板图
        cv::Mat roi = digitRigions[regionIndex].clone();
        cv::Mat tmpl = digitTemplates[i].clone();

        // 如果图为空，跳过或赋0
        if (roi.empty() || tmpl.empty()) {
            qDebug() << "[ERROR] ROI[" << regionIndex << "] or Template[" << i << "] is empty. Skip this pair.";
            shapeVals[i] = 0.0;
            continue;
        }

        // 调整大小，使两者尺寸一致
        cv::resize(roi, roi, tmpl.size());

        // 这里不再做轮廓提取，只做基本预处理：
        //   1. 转灰度
        //   2. 高斯模糊 (可选)
        //   3. 二值化 (可选)
        // 这些都已经在 getSimilarity() 里做了，所以我们可以直接传 roi, tmpl 进去。

//        //对目标图像进行膨胀（适用于字符较大的情况）
//        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3,3)); // 可调参数
//        cv::dilate(roi, roi, kernel, cv::Point(-1, -1), 1); // 可调参数
//        //对模板图像进行膨胀
//        cv::dilate(tmpl, tmpl, kernel, cv::Point(-1, -1), 1); // 应与ROI相同

        // 计算相似度
        double scoreVal = getSimilarity(roi, tmpl);
        shapeVals[i] = scoreVal;

        // 输出每个区域的匹配度 (可注释)
         qDebug() << "[INFO] Matching score for ROI[" << regionIndex
                  << "] with Template[" << i << "] = " << scoreVal;

        // 若要记录最小分
        if (scoreVal < minScore) {
            minScore = scoreVal;
        }
    }

    // --------------------【C】在 imageWithBoxes 上写分数 (可选)-------------------- //
    // (如果你的 rectTopCenterPoints 跟 digitRegions 数量一致，可参考如下方法做可视化)
    for (int i = 0; i < M && i < static_cast<int>(rectTopCenterPoints.size()); ++i)
    {
        // 获取当前矩形框的中心坐标
        cv::Point topLeft = rectTopCenterPoints[startIndex + i];
        cv::Size boxSize = digitRigions[startIndex + i].size();

        // 获取当前模板与ROI的匹配得分
        double scoreVal = shapeVals[i];

        // 计算文本(百分比形式)
        double scorePercent = scoreVal * 100.0;
        std::string scoreText = std::to_string(static_cast<int>(scorePercent));

        // 计算文本的大小
        int baseline = 0;
        double fontScale = 0.4;  // 设置文本字体大小
        cv::Size textSize = cv::getTextSize(scoreText, cv::FONT_HERSHEY_SIMPLEX, fontScale, 1, &baseline);

        // 计算文本的居中位置（文本的中心需要与框的中心对齐）
        int x = topLeft.x - textSize.width / 2; // 文本X坐标，使文本居中
        int y = topLeft.y - 1;                  // 文本Y坐标，使文本显示在矩形框上方

        // 绘制文本
        cv::putText(imageWithBoxes, scoreText, cv::Point(x, y),
                    cv::FONT_HERSHEY_SIMPLEX, fontScale, cv::Scalar(255, 0, 0), 1);

        // 判断得分是否低于阈值（说明相似度不够）
        if (scoreVal < thresholdSSIM) {
            // 计算矩形框的左上角和右下角坐标
            int rectLeft   = topLeft.x - boxSize.width / 2;
            int rectTop    = topLeft.y;
            int rectRight  = topLeft.x + boxSize.width / 2;
            int rectBottom = topLeft.y + boxSize.height;

            // 绘制红色矩形框
            cv::rectangle(imageWithBoxes, cv::Point(rectLeft, rectTop),
                          cv::Point(rectRight, rectBottom), cv::Scalar(0, 0, 255), 2);
        }
    }

    // --------------------【D】显示结果 (可选)-------------------- //
    double scaleFactor = 2.0;  // 放大倍数，可根据需要调整
    cv::Mat imageWithBoxesResized;
    cv::resize(imageWithBoxes, imageWithBoxesResized,
               cv::Size(), scaleFactor, scaleFactor, cv::INTER_LINEAR);

    cv::namedWindow("Match Result", cv::WINDOW_NORMAL);
    cv::resizeWindow("Match Result", imageWithBoxesResized.cols, imageWithBoxesResized.rows);
    cv::imshow("Match Result", imageWithBoxesResized);
    cv::waitKey(1);

    // --------------------【E】返回最小分 (或其他指标)-------------------- //
    return minScore;
}





//------------------------【6】其他--------------------------------//
void TemplateMatch::jianceshibiestr(String string1)
{
    input = string1;
}
