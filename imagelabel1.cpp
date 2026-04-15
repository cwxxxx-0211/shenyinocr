#include "imagelabel1.h"

/**
 * @brief ImageLabel的构造函数
 * @param parent 父窗口指针，默认为nullptr
 * @details 初始化ImageLabel对象，设置初始状态为非绘制状态。
 */
ImageLabel1::ImageLabel1(QWidget *parent) : QLabel(parent), drawing(false)
{
}

/**
 * @brief 处理鼠标按下事件
 * @param event 鼠标事件对象
 * @details 当左键按下时，进入绘制状态，记录起点，初始化选择区域，并刷新界面。
 */
void ImageLabel1::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        drawing = true;
        startPoint = event->pos();
        selectionRect = QRect(startPoint, QSize());
        update();
    }
    emit mousePressed(event);
}

/**
 * @brief 处理鼠标移动事件
 * @param event 鼠标事件对象
 * @details 当处于绘制状态时，根据起点和当前鼠标位置更新选择区域，并刷新界面。
 */
void ImageLabel1::mouseMoveEvent(QMouseEvent *event)
{
    if (drawing) {
        selectionRect = QRect(startPoint, event->pos()).normalized();
        update();
    }
    emit mouseMoved(event);
}

/**
 * @brief 处理鼠标释放事件
 * @param event 鼠标事件对象
 * @details 当左键释放时，退出绘制状态，更新选择区域，并刷新界面。
 */
void ImageLabel1::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        drawing = false;
        selectionRect = QRect(startPoint, event->pos()).normalized();
        update();
    }
    emit mouseReleased(event);
}

/**
 * @brief 处理绘制事件
 * @param event 绘制事件对象
 * @details 在标签上绘制选择区域。
 */
void ImageLabel1::paintEvent(QPaintEvent *event)
{
    QLabel::paintEvent(event);
    if (!selectionRect.isNull()) {
        QPainter painter(this);
        painter.setPen(QPen(Qt::green, 1));
        painter.drawRect(selectionRect);
    }
}

/**
 * @brief 获取选择区域的矩形
 * @return 选择区域的矩形
 */
QRect ImageLabel1::getSelectionRect() const
{
    return selectionRect;
}

/**
 * @brief 清除选择区域
 * @details 将选择区域设置为空矩形，并刷新界面。
 */
void ImageLabel1::clearSelection()
{
    selectionRect = QRect();
    update();
}

/**
 * @brief 设置选择区域的矩形
 * @param rect 新的选择区域矩形
 * @details 更新选择区域为指定矩形，并刷新界面。
 */
void ImageLabel1::setSelectionRect(const QRect &rect)
{
    selectionRect = rect;
    update();
}

/**
 * @brief 设置选择区域的起点
 * @param point 新的起点坐标
 * @details 更新选择区域的起点为指定坐标。
 */
void ImageLabel1::setStartPoint(const QPoint &point)
{
    startPoint = point;
}

/**
 * @brief 获取选择区域的起点
 * @return 选择区域的起点坐标
 */
QPoint ImageLabel1::getStartPoint() const
{
    return startPoint;
}

/**
 * @brief 设置是否处于绘制状态
 * @param draw 绘制状态，true为绘制，false为非绘制
 * @details 更新对象的绘制状态。
 */
void ImageLabel1::setDrawing(bool draw)
{
    drawing = draw;
}

/**
 * @brief 获取是否处于绘制状态
 * @return 当前的绘制状态，true为绘制，false为非绘制
 */
bool ImageLabel1::isDrawing() const
{
    return drawing;
}
