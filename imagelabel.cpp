#include "imagelabel.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>
#include <QString>
#include <QMessageBox>
ImageLabel::ImageLabel(QWidget *parent)
    : QLabel(parent), drawing(false), m_color(1), allowBlueDraw(false), blueRectRedrawn(false), rectAdded(0)
{
    // 设置尺寸策略：响应布局但不响应内容
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    // 图像显示设置：居中显示，保持图片比例
    setAlignment(Qt::AlignCenter);
    setScaledContents(false); // 关闭自动缩放，手动控制保持比例

    // 设置最小和最大尺寸限制
    setMinimumSize(200, 150);
    setMaximumSize(800, 600);
}

void ImageLabel::setColor(int color)
{
    m_color = color;
}

void ImageLabel::addSelectionRect(const QRect &rect, int color)
{
    switch (color)
    {
    case 1:
        redRects.clear();
        redRects.append(rect);
        break;
    case 2:
        greenRects.append(rect);
        break;
    case 3:
        blueRects.clear();
        blueRects.append(rect);
        rectAdded = rectAdded + 1;
        break;
    default:
        yellowRects.append(rect);
        break;
    }
    update();
}

QRect ImageLabel::getSelectionRect() const
{
    qDebug() << "mcolor" << m_color;
    return selectionRect;
}

void ImageLabel::clearSelection()
{
    redRects.clear();
    greenRects.clear();
    blueRects.clear();
    yellowRects.clear();
    redPolygons.clear();
    bluePolygons.clear();
    greenPolygons.clear();
    selectionRect = QRect();
    update();
}

void ImageLabel::clearGreenRects()
{
    greenRects.clear();
    greenPolygons.clear();
    update();
}

void ImageLabel::clearredRects()
{
    redRects.clear();
    redPolygons.clear();
    update();
}

void ImageLabel::clearblueRects()
{
    blueRects.clear();
    bluePolygons.clear();
    update();
}
void ImageLabel::addSelectionPolygon(const QPolygonF &polygon, int color)
{
    switch (color)
    {
    case 1:
        redRects.clear();
        redPolygons.append(polygon);
        break;
    case 2:
        greenPolygons.append(polygon);
        break;
    case 3:
        blueRects.clear();
        bluePolygons.append(polygon);
        break;
    default:
        break;
    }
    update();
}

void ImageLabel::setSelectionRect(const QRect &rect)
{
    selectionRect = rect;
    update();
}

void ImageLabel::setStartPoint(const QPoint &point)
{
    startPoint = point;
}

QPoint ImageLabel::getStartPoint() const
{
    return startPoint;
}

void ImageLabel::setDrawing(bool draw)
{
    drawing = draw;
}

bool ImageLabel::isDrawing() const
{
    return drawing;
}

void ImageLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (m_color == 3)
        {
            clearblueRects();
        }
        drawing = true;
        startPoint = event->pos();
        selectionRect = QRect(startPoint, QSize());
        update();
    }
    emit mousePressed(event);
}

void ImageLabel::mouseMoveEvent(QMouseEvent *event)
{
    if (drawing)
    {
        selectionRect = QRect(startPoint, event->pos()).normalized();
        update();
    }
    emit mouseMoved(event);
}

void ImageLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        drawing = false;
        selectionRect = QRect(startPoint, event->pos()).normalized();
        addSelectionRect(selectionRect, m_color);
        if (m_color == 3)
        {
            blueRectRedrawn = true;
        }
        update();
    }
    emit mouseReleased(event);
}

void ImageLabel::paintEvent(QPaintEvent *event)
{
    QLabel::paintEvent(event);
    QPainter painter(this);

    // 绘制红色矩形框
    painter.setPen(QPen(Qt::red, 2));
    for (const QRect &rect : redRects)
    {
        painter.drawRect(rect);
    }
    for (const QPolygonF &polygon : redPolygons)
    {
        painter.drawPolygon(polygon);
    }

    // 绘制绿色矩形框
    painter.setPen(QPen(Qt::green, 2));
    for (const QRect &rect : greenRects)
    {
        painter.drawRect(rect);
    }
    for (const QPolygonF &polygon : greenPolygons)
    {
        painter.drawPolygon(polygon);
    }

    // 绘制蓝色矩形框
    painter.setPen(QPen(Qt::blue, 2));
    for (const QRect &rect : blueRects)
    {
        painter.drawRect(rect);
    }
    for (const QPolygonF &polygon : bluePolygons)
    {
        painter.drawPolygon(polygon);
    }

    // 绘制黄色矩形框
    painter.setPen(QPen(Qt::yellow, 2));
    for (const QRect &rect : yellowRects)
    {
        painter.drawRect(rect);
    }

    // 绘制正在拖动的实时矩形框
    if (drawing)
    {
        switch (m_color)
        {
        case 1:
            painter.setPen(QPen(Qt::red, 2, Qt::DashLine));
            break;
        case 2:
            painter.setPen(QPen(Qt::green, 2, Qt::DashLine));
            break;
        case 3:
            painter.setPen(QPen(Qt::blue, 2, Qt::DashLine));
            break;
        case 4:
            painter.setPen(QPen(Qt::yellow, 2, Qt::DashLine));
            break;
        default:
            painter.setPen(QPen(Qt::black, 2, Qt::DashLine));
            break;
        }
        painter.drawRect(selectionRect);
    }
}

void ImageLabel::setPixmap(const QPixmap &pixmap)
{
    if (pixmap.isNull())
    {
        QLabel::setPixmap(pixmap);
        return;
    }

    // 按比例缩放图片以适应控件大小，保持宽高比
    QPixmap scaledPixmap = pixmap.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QLabel::setPixmap(scaledPixmap);
}
