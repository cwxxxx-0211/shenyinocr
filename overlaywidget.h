#ifndef OVERLAYWIDGET_H
#define OVERLAYWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QRect>

class OverlayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OverlayWidget(QWidget *parent = nullptr);
    void setRect(const QRect &rect);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QRect rect;
};

#endif // OVERLAYWIDGET_H
