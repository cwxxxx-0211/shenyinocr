#include "imagelabel.h"
#include <QPainter>
#include <QPen>

// 🔥 仿照 widget.cpp，加入这句声明，彻底解决底层发送的中文乱码问题
#pragma execution_character_set("utf-8")

ImageLabel::ImageLabel(QWidget *parent) : QLabel(parent) {
    m_currentStep = STEP_TRACKING;
}

void ImageLabel::setPixmap(const QPixmap &pixmap) {
    QLabel::setPixmap(pixmap);
}

// =====================================================================
// 恢复你原来丢失的函数实现（保持兼容）
// =====================================================================
void ImageLabel::setColor(int color) { m_color = color; }

void ImageLabel::addSelectionRect(const QRect &rect, int color) {
    rectangles.append({rect, color});
    update();
}

QRect ImageLabel::getSelectionRect() const { return selectionRect; }

void ImageLabel::setSelectionRect(const QRect &rect) {
    selectionRect = rect;
    update();
}

void ImageLabel::setStartPoint(const QPoint &point) { startPoint = point; }

QPoint ImageLabel::getStartPoint() const { return startPoint; }

void ImageLabel::setDrawing(bool draw) { drawing = draw; }

void ImageLabel::clearGreenRects() {
    greenRects.clear();
    update();
}

void ImageLabel::clearredRects() {
    redRects.clear();
    redPolygons.clear();
    update();
}

void ImageLabel::clearblueRects() {
    blueRects.clear();
    bluePolygons.clear();
    update();
}

void ImageLabel::addSelectionPolygon(const QPolygonF &polygon, int color) {
    if (color == 1) greenPolygons.append(polygon);
    else if (color == 2) redPolygons.append(polygon);
    else if (color == 3) bluePolygons.append(polygon);
    update();
}

bool ImageLabel::isDrawing() const { return drawing; }

// =====================================================================
// 全左键顺序画双框交互逻辑（带中文提示，不再乱码）
// =====================================================================

void ImageLabel::resetDrawingStep() {
    m_currentStep = STEP_TRACKING;
    m_trackingRect = QRect();
    m_detectionRect = QRect();
    selectionRect = QRect();
    m_isInteracting = false;
    update();
    emit signal_hintMessage("第一步：请【按住左键】框选固定的特征(锚点)");
}

void ImageLabel::clearSelection() {
    selectionRect = QRect();
    selectionRect1 = QRect();
    rectangles.clear();
    resetDrawingStep();
}

void ImageLabel::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (m_currentStep == STEP_DONE) {
            resetDrawingStep();
        }
        m_isInteracting = true;
        m_startPoint = event->pos();

        if (m_currentStep == STEP_TRACKING) {
            m_trackingRect = QRect(m_startPoint, m_startPoint);
        } else if (m_currentStep == STEP_DETECTION) {
            m_detectionRect = QRect(m_startPoint, m_startPoint);
        }
    }
    emit mousePressed(event);
}

void ImageLabel::mouseMoveEvent(QMouseEvent *event) {
    if (m_isInteracting) {
        if (m_currentStep == STEP_TRACKING) {
            m_trackingRect.setBottomRight(event->pos());
        } else if (m_currentStep == STEP_DETECTION) {
            m_detectionRect.setBottomRight(event->pos());
        }
        update();
    }
    emit mouseMoved(event);
}

void ImageLabel::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_isInteracting) {
        m_isInteracting = false;

        if (m_currentStep == STEP_TRACKING) {
            m_trackingRect = m_trackingRect.normalized();
            if (m_trackingRect.width() > 5) {
                m_currentStep = STEP_DETECTION;
                emit signal_hintMessage("锚点选好了！第二步：请继续【按住左键】框选变动的日期区域");
            } else {
                m_trackingRect = QRect();
                emit signal_hintMessage("框太小！请重新【按住左键】框选锚点");
            }
        } else if (m_currentStep == STEP_DETECTION) {
            m_detectionRect = m_detectionRect.normalized();
            if (m_detectionRect.width() > 5) {
                m_currentStep = STEP_DONE;
                selectionRect = m_detectionRect;
                emit signal_hintMessage("双框已就绪！请点击右侧【保存模板】");
            } else {
                m_detectionRect = QRect();
                emit signal_hintMessage("框太小！请重新【按住左键】框选日期");
            }
        }
        update();
    }
    emit mouseReleased(event);
}

void ImageLabel::paintEvent(QPaintEvent *event) {
    QLabel::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 恢复绘制旧的矩形（保持对原有代码的兼容）
    painter.setPen(QPen(Qt::green, 2));
    for (const QRect& r : greenRects) painter.drawRect(r);
    painter.setPen(QPen(Qt::red, 2));
    for (const QRect& r : redRects) painter.drawRect(r);
    for (const ColoredRect& cr : rectangles) {
        if(cr.color == 1) painter.setPen(QPen(Qt::green, 2));
        else if(cr.color == 2) painter.setPen(QPen(Qt::red, 2));
        else painter.setPen(QPen(Qt::blue, 2));
        painter.drawRect(cr.rect);
    }

    // 画追踪框 (仅显示纯净的蓝色粗框，去除文字避免乱码或遮挡)
    if (!m_trackingRect.isNull()) {
        painter.setPen(QPen(Qt::blue, 3, Qt::SolidLine));
        painter.drawRect(m_trackingRect);
    }

    // 画检测框 (仅显示纯净的绿色粗框，去除文字避免乱码或遮挡)
    if (!m_detectionRect.isNull()) {
        painter.setPen(QPen(Qt::green, 3, Qt::SolidLine));
        painter.drawRect(m_detectionRect);
    }
}
