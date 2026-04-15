#ifndef IMAGELABEL_H
#define IMAGELABEL_H

#include <QLabel>
#include <QRect>
#include <QVector>

class ImageLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ImageLabel(QWidget *parent = nullptr);
    void setColor(int color);
    void addSelectionRect(const QRect &rect, int color);
    QRect getSelectionRect() const;
    void clearSelection();
    void setSelectionRect(const QRect &rect);
    void setStartPoint(const QPoint &point);
    QPoint getStartPoint() const;
    void setDrawing(bool draw);
    void clearGreenRects();
    void clearredRects();
    void clearblueRects();
    void addSelectionPolygon(const QPolygonF &polygon, int color);
    bool isDrawing() const;
    bool allowBlueDraw;
    bool blueRectRedrawn;
    int rectAdded;

    // 重写setPixmap，自动按比例缩放
    void setPixmap(const QPixmap &pixmap);

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
    struct ColoredRect
    {
        QRect rect;
        int color;
    };

    bool drawing;
    QPoint startPoint;
    QRect selectionRect;
    QRect selectionRect1;
    QList<QRect> redRects;
    QList<QRect> greenRects;
    QList<QRect> blueRects;
    QList<QRect> yellowRects;
    QVector<QPolygonF> redPolygons;
    QVector<QPolygonF> greenPolygons;
    QVector<QPolygonF> bluePolygons;
    QVector<ColoredRect> rectangles;
    int m_color;
};

#endif // IMAGELABEL_H
