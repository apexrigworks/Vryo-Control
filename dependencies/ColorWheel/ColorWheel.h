#ifndef COLORWHEEL_H
#define COLORWHEEL_H

#include <QWidget>

class ColorWheel : public QWidget
{
    Q_OBJECT
public:
    explicit ColorWheel(QWidget *parent = 0);

    virtual QSize sizeHint () const;
    virtual QSize minimumSizeHint () const;
    QColor color();

signals:
    void colorChanged(const QColor color);
    
public slots:
    void setColor(const QColor &color);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *);
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *);
private:
    QSize initSize;
    QImage wheelImage;
    QImage squareImage;
    QPixmap wheel;
    bool mouseDown;
    QPoint lastPos;
    int margin;
    int wheelWidth;
    const qreal squareGap = 5.0;
    QRegion wheelRegion;
    QRegion squareRegion;
    QColor current;
    bool inWheel;
    bool inSquare;
    int x_offset = 0;
    int y_offset = 0;

    QPixmap CleanWheel;
    QImage CleanSquare;

    qreal squareCornerRadius() const;
    void squareGeometry(qreal &left, qreal &top, qreal &side) const;
    QColor posColor(const QPoint &point);
    void rebuildCache(const QSize &newSize);
    void drawWheelImage(const QSize &newSize);
    void drawIndicator(const int &hue);
    void drawPicker(const QColor &color);
    void drawSquareImage(const int &hue);
    void composeWheel();
private slots:
    void hueChanged(const int &hue);
    void svChanged(const QColor &newcolor);
};

#endif // COLORWHEEL_H
