#ifndef IMAGELABEL1_H
#define IMAGELABEL1_H

#include <QLabel>
#include <QMouseEvent>
#include <QPainter>

class ImageLabel1 : public QLabel
{
    Q_OBJECT

public:
    explicit ImageLabel1(QWidget *parent = nullptr);

    QRect getSelectionRect() const;
    void clearSelection();
    void setSelectionRect(const QRect &rect);

    void setStartPoint(const QPoint &point);
    QPoint getStartPoint() const;
    void setDrawing(bool draw);
    bool isDrawing() const;
    bool drawing;
    QRect selectionRect;
    QPoint startPoint;
signals:
    void mousePressed(QMouseEvent *event);
    void mouseMoved(QMouseEvent *event);
    void mouseReleased(QMouseEvent *event);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:

};

#endif // IMAGELABEL1_H
