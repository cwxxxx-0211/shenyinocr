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
    m_detectionPoly.clear();
    selectionRect = QRect();
    m_isInteracting = false;
    update();
    emit signal_hintMessage(QStringLiteral("\u7b2c\u4e00\u6b65\uFF1A\u8BF7\u3010\u6309\u4F4F\u5DE6\u952E\u62D6\u52A8\u3011\u6846\u9009\u56FA\u5B9A\u7684\u7279\u5F81(\u951A\u70B9)"));
}

void ImageLabel::clearSelection() {
    selectionRect = QRect();
    selectionRect1 = QRect();
    rectangles.clear();
    resetDrawingStep();
}

void ImageLabel::mousePressEvent(QMouseEvent *event) {
    if (m_currentStep == STEP_DONE && event->button() == Qt::LeftButton) {
        resetDrawingStep();
    }

    if (m_currentStep == STEP_TRACKING) {
        if (event->button() == Qt::LeftButton) {
            m_isInteracting = true;
            m_startPoint = event->pos();
            m_trackingRect = QRect(m_startPoint, m_startPoint);
        }
    } else if (m_currentStep == STEP_DETECTION_POLY) {
        if (event->button() == Qt::LeftButton) {
            m_detectionPoly << event->pos();
            m_tempPolyPoint = event->pos();
            update();
        } else if (event->button() == Qt::RightButton) {
            if (m_detectionPoly.size() >= 3) {
                m_currentStep = STEP_DONE;
                emit signal_hintMessage(QStringLiteral("\u591A\u8FB9\u5F62\u5DF2\u95ED\u5408\uFF01\u8BF7\u70B9\u51FB\u53F3\u4FA7\u3010\u4FDD\u5B58\u6A21\u677F\u3011"));
            } else {
                emit signal_hintMessage(QStringLiteral("\u591A\u8FB9\u5F62\u9876\u70B9\u592A\u5C11\uFF0C\u8BF7\u7EE7\u7EED\u70B9\u51FB\u5DE6\u952E\uFF01"));
            }
            update();
        }
    }
    emit mousePressed(event);
}

void ImageLabel::mouseMoveEvent(QMouseEvent *event) {
    if (m_currentStep == STEP_TRACKING && m_isInteracting) {
        m_trackingRect.setBottomRight(event->pos());
        update();
    } else if (m_currentStep == STEP_DETECTION_POLY) {
        m_tempPolyPoint = event->pos();
        update();
    }
    emit mouseMoved(event);
}

void ImageLabel::mouseReleaseEvent(QMouseEvent *event) {
    if (m_currentStep == STEP_TRACKING && event->button() == Qt::LeftButton && m_isInteracting) {
        m_isInteracting = false;
        m_trackingRect = m_trackingRect.normalized();
        if (m_trackingRect.width() > 5) {
            m_currentStep = STEP_DETECTION_POLY;
            m_detectionPoly.clear();
            emit signal_hintMessage(QStringLiteral("\u951A\u70B9\u9009\u597D\u4E86\uFF01\u7B2C\u4E8C\u6B65\uFF1A\u8BF7\u3010\u8FDE\u7EED\u70B9\u51FB\u5DE6\u952E\u3011\u63CF\u7ED8\u5B8C\u6574\u7684\u65E5\u671F\u8FB9\u7F18\uFF0C\u3010\u53F3\u952E\u3011\u5B8C\u6210\u95ED\u5408"));
        } else {
            m_trackingRect = QRect();
            emit signal_hintMessage(QStringLiteral("\u6846\u592A\u5C0F\uFF01\u8BF7\u91CD\u65B0\u3010\u6309\u4F4F\u5DE6\u952E\u3011\u6846\u9009\u951A\u70B9"));
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

    // 画追踪框 (蓝色粗框)
    if (!m_trackingRect.isNull()) {
        painter.setPen(QPen(Qt::blue, 3, Qt::SolidLine));
        painter.drawRect(m_trackingRect);
    }

    // 画生产日期多边形 (绿色)
    if (!m_detectionPoly.isEmpty()) {
        painter.setPen(QPen(Qt::green, 3, Qt::SolidLine));
        painter.drawPolyline(m_detectionPoly);

        // 如果还没画完，画一根跟随鼠标的虚线
        if (m_currentStep == STEP_DETECTION_POLY) {
            painter.setPen(QPen(Qt::green, 2, Qt::DashLine));
            painter.drawLine(m_detectionPoly.last(), m_tempPolyPoint);
            painter.drawLine(m_tempPolyPoint, m_detectionPoly.first()); // 闭合预览
        } else if (m_currentStep == STEP_DONE) {
            painter.setPen(QPen(Qt::green, 3, Qt::SolidLine));
            painter.drawPolygon(m_detectionPoly); // 闭合
        }
        
        // 画顶点圆圈
        painter.setBrush(Qt::green);
        for (const QPoint& pt : m_detectionPoly) {
            painter.drawEllipse(pt, 4, 4);
        }
    }
}
