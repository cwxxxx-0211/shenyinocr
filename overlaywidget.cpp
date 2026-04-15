#include "overlaywidget.h"

OverlayWidget::OverlayWidget(QWidget *parent)
    : QWidget(parent), rect(QRect())
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
}

void OverlayWidget::setRect(const QRect &newRect)
{
    rect = newRect;
    update(); // 触发重绘
}

void OverlayWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    if (!rect.isNull()) {
        QPainter painter(this);
        painter.setPen(QPen(Qt::red, 2));
        painter.drawRect(rect);
    }
}
