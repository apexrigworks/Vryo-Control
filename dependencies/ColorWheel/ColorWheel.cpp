/*-----------------------------------------------------*\
| ColorWheel.cpp                                        |
|                                                       |
|  Color wheel selector widget for Qt                   |
|                                                       |
|  Original: https://github.com/liuyanghejerry/Qt-Plus  |
|                                                       |
|  Modified by Adam Honse (calcprogrammer1@gmail.com)   |
|                                                       |
|  Vryo Control: thicker hue ring, rounded square with  |
|  brightness running upwards (bright at the top, dark  |
|  at the bottom, like every other picker), large       |
|  handles filled with the colour they pick, and a      |
|  much faster square.                                  |
\*-----------------------------------------------------*/

#include "ColorWheel.h"
#include <QPainter>
#include <QResizeEvent>
#include <QStyleOption>
#include <QtCore/qmath.h>
#include <QDebug>
#include <QPainterPath>

namespace
{
QSize devicePixelSize(const QSize &logicalSize, const qreal devicePixelRatio)
{
    return QSize(qMax(1, qRound(logicalSize.width() * devicePixelRatio)),
                 qMax(1, qRound(logicalSize.height() * devicePixelRatio)));
}

/*-----------------------------------------------------*\
| A round handle: dark soft outline outside, thick      |
| white ring, and the picked colour in the middle       |
\*-----------------------------------------------------*/
void drawHandle(QPainter &painter, const QPointF &centre, qreal radius, const QColor &fill)
{
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(0, 0, 0, 120), 1.5));
    painter.drawEllipse(centre, radius + 1.75, radius + 1.75);

    painter.setPen(QPen(QColor(255, 255, 255), 2.5));
    painter.setBrush(fill);
    painter.drawEllipse(centre, radius, radius);
}
}

ColorWheel::ColorWheel(QWidget *parent) :
    QWidget(parent),
    initSize(150,150),
    mouseDown(false),
    margin(5),
    wheelWidth(14),
    current(Qt::red),
    inWheel(false),
    inSquare(false)
{
    current = current.toHsv();
}

QColor ColorWheel::color()
{
    return current;
}

void ColorWheel::setColor(const QColor &color)
{
    if(color == current) return;
    if(color.hue() != current.hue())
    {
        hueChanged(color.hue());
    }

    if((color.saturation() != current.saturation()) || (color.value() != current.value()))
    {
        svChanged(color);
    }

    update();
    emit colorChanged(color);
}

/*-----------------------------------------------------*\
| Corner radius of the saturation / value square        |
\*-----------------------------------------------------*/
qreal ColorWheel::squareCornerRadius() const
{
    return qBound(6.0, 0.07 * qMin(width(), height()), 12.0);
}

/*-----------------------------------------------------*\
| Where the saturation / value square sits, in widget   |
| coordinates. The corners are rounded, so the square   |
| can be larger than a sharp one would be inside the    |
| same ring.                                            |
\*-----------------------------------------------------*/
void ColorWheel::squareGeometry(qreal &left, qreal &top, qreal &side) const
{
    const qreal w           = qMin(width(), height());
    const qreal outer       = (w / 2.0) - margin;
    const qreal inner       = outer - wheelWidth;
    const qreal reach       = qMax(0.0, inner - squareGap);
    const qreal corner      = squareCornerRadius();
    const qreal half        = (qMax(0.0, reach - corner) / qSqrt(2.0)) + corner;

    side    = 2.0 * half;
    left    = x_offset + ((w / 2.0) - half);
    top     = y_offset + ((w / 2.0) - half);
}

QColor ColorWheel::posColor(const QPoint &point)
{
    /*-----------------------------------------------------*\
    | If within wheel region, update hue from the angle of  |
    | the point around the centre                           |
    \*-----------------------------------------------------*/
    if(inWheel)
    {
        const qreal center  = qMin(width(), height()) / 2.0;
        const qreal dx      = (point.x() - x_offset) - center;
        const qreal dy      = center - (point.y() - y_offset);      // screen y grows downwards

        qreal hue = qRadiansToDegrees(qAtan2(dy, dx));

        if(hue < 0.0)
        {
            hue += 360.0;
        }

        /*-------------------------------------------------*\
        | Restrict hue to range 0-359                       |
        \*-------------------------------------------------*/
        const int hue_int = qBound(0, (int)hue, 359);

        return QColor::fromHsv(hue_int,
                               current.saturation(),
                               current.value());
    }

    /*-----------------------------------------------------*\
    | If within square region, update saturation and value  |
    | from point position. Saturation grows to the right,   |
    | value grows upwards.                                  |
    \*-----------------------------------------------------*/
    if(inSquare)
    {
        qreal left;
        qreal top;
        qreal side;

        squareGeometry(left, top, side);

        const qreal saturation  = qBound(0.0, (point.x() - left) / side, 1.0);
        const qreal value       = 1.0 - qBound(0.0, (point.y() - top) / side, 1.0);
        const qreal current_hue = current.hueF();

        return QColor::fromHsvF( (current_hue < 0.0) ? 0.0 : current_hue,
                                 saturation,
                                 value);
    }
    return QColor();
}

QSize ColorWheel::sizeHint () const
{
    return QSize(height(),height());
}

QSize ColorWheel::minimumSizeHint () const
{
    return initSize;
}

void ColorWheel::mousePressEvent(QMouseEvent *event)
{
    /*-----------------------------------------------------*\
    | Update last position                                  |
    \*-----------------------------------------------------*/
    lastPos = event->pos();

    /*-----------------------------------------------------*\
    | If mouse is within wheel region, process wheel (hue)  |
    \*-----------------------------------------------------*/
    if(wheelRegion.contains(lastPos))
    {
        inWheel = true;
        inSquare = false;
        QColor color = posColor(lastPos);
        hueChanged(color.hue());
    }

    /*-----------------------------------------------------*\
    | If mouse is within square region, process square      |
    | (saturation and value)                                |
    \*-----------------------------------------------------*/
    else if(squareRegion.contains(lastPos))
    {
        inWheel = false;
        inSquare = true;
        QColor color = posColor(lastPos);
        svChanged(color);
    }

    /*-----------------------------------------------------*\
    | Set the mouse down flag if the click started inside a |
    | selectable region                                     |
    \*-----------------------------------------------------*/
    mouseDown = inWheel || inSquare;
}

void ColorWheel::mouseMoveEvent(QMouseEvent *event)
{
    /*-----------------------------------------------------*\
    | Update last position                                  |
    \*-----------------------------------------------------*/
    lastPos = event->pos();

    /*-----------------------------------------------------*\
    | Don't process if mouse button is not down             |
    \*-----------------------------------------------------*/
    if(!mouseDown)
    {
        return;
    }

    /*-----------------------------------------------------*\
    | If dragging started in the wheel, continue processing |
    | hue from the cursor angle even outside the wheel      |
    \*-----------------------------------------------------*/
    if(inWheel)
    {
        QColor color = posColor(lastPos);
        hueChanged(color.hue());
    }

    /*-----------------------------------------------------*\
    | If dragging started in the square, continue processing|
    | saturation and value with clamped coordinates         |
    \*-----------------------------------------------------*/
    else if(inSquare)
    {
        QColor color = posColor(lastPos);
        svChanged(color);
    }
}

void ColorWheel::mouseReleaseEvent(QMouseEvent *)
{
    /*-----------------------------------------------------*\
    | Clear mouse down and in-region flags                  |
    \*-----------------------------------------------------*/
    mouseDown   = false;
    inWheel     = false;
    inSquare    = false;
}

void ColorWheel::resizeEvent(QResizeEvent *event)
{
    rebuildCache(event->size());
    update();
}

void ColorWheel::paintEvent(QPaintEvent *)
{
    if(wheel.isNull() || !qFuzzyCompare(wheel.devicePixelRatio(), devicePixelRatioF()))
    {
        rebuildCache(size());
    }

    QPainter painter(this);
    QStyleOption opt;
    opt.initFrom(this);
    composeWheel();
    painter.drawPixmap(0,0,wheel);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);
}

void ColorWheel::rebuildCache(const QSize &newSize)
{
    const int size = qMin(newSize.width(), newSize.height());
    const qreal devicePixelRatio = devicePixelRatioF();

    wheelWidth = qMax(12, qRound(0.13 * size));

    wheel = QPixmap(devicePixelSize(newSize, devicePixelRatio));
    wheel.setDevicePixelRatio(devicePixelRatio);
    wheel.fill(Qt::transparent);
    drawWheelImage(newSize);
    drawSquareImage(current.hue());
}

void ColorWheel::drawWheelImage(const QSize &newSize)
{
    /*-----------------------------------------------------*\
    | Create image canvas                                   |
    \*-----------------------------------------------------*/
    const qreal devicePixelRatio = devicePixelRatioF();
    wheelImage = QImage(devicePixelSize(newSize, devicePixelRatio), QImage::Format_ARGB32_Premultiplied);
    wheelImage.setDevicePixelRatio(devicePixelRatio);

    /*-----------------------------------------------------*\
    | Paint the background                                  |
    \*-----------------------------------------------------*/
    wheelImage.fill(Qt::transparent);

    /*-----------------------------------------------------*\
    | Create rainbow gradient for wheel                     |
    \*-----------------------------------------------------*/
    QConicalGradient conicalGradient(0, 0, 0);
    conicalGradient.setColorAt(0.0,           Qt::red);
    conicalGradient.setColorAt(60.0  / 360.0, Qt::yellow);
    conicalGradient.setColorAt(120.0 / 360.0, Qt::green);
    conicalGradient.setColorAt(180.0 / 360.0, Qt::cyan);
    conicalGradient.setColorAt(240.0 / 360.0, Qt::blue);
    conicalGradient.setColorAt(300.0 / 360.0, Qt::magenta);
    conicalGradient.setColorAt(1.0,           Qt::red);

    /*-----------------------------------------------------*\
    | Set up painter with antialiasing                      |
    \*-----------------------------------------------------*/
    QPainter painter(&wheelImage);
    painter.setRenderHint(QPainter::Antialiasing);

    /*-----------------------------------------------------*\
    | Paint the wheel                                       |
    \*-----------------------------------------------------*/
    int size = qMin(newSize.width(), newSize.height());
    x_offset = (newSize.width() - size) / 2;
    y_offset = (newSize.height() - size) / 2;
    int r = size;
    const qreal center = size / 2.0;
    const qreal outerRadius = center - margin;
    const qreal innerRadius = outerRadius - wheelWidth;

    QPainterPath painterpath;
    painterpath.addEllipse(QPointF(0,0), outerRadius, outerRadius);
    painterpath.addEllipse(QPointF(0,0), innerRadius, innerRadius);

    painter.translate(x_offset + center, y_offset + center);

    QBrush brush(conicalGradient);
    painter.setPen(Qt::NoPen);
    painter.setBrush(brush);

    painter.drawPath(painterpath);

    /*-----------------------------------------------------*\
    | Calculate wheel region and subtract out the inner     |
    | region                                                |
    \*-----------------------------------------------------*/
    wheelRegion = QRegion(r/2, r/2, r-2*margin, r-2*margin, QRegion::Ellipse);
    wheelRegion.translate(x_offset - (r-2*margin)/2, y_offset - (r-2*margin)/2);

    int tmp = 2*(margin+wheelWidth);
    QRegion subRe( r/2, r/2, r-tmp, r-tmp, QRegion::Ellipse );
    subRe.translate( x_offset - (r-tmp)/2, y_offset - (r-tmp)/2);
    wheelRegion -= subRe;

    CleanWheel = QPixmap::fromImage(wheelImage);
    CleanWheel.setDevicePixelRatio(devicePixelRatio);
}

void ColorWheel::drawSquareImage(const int &hue)
{
    /*-----------------------------------------------------*\
    | Where the square goes                                 |
    \*-----------------------------------------------------*/
    qreal left;
    qreal top;
    qreal SquareWidth;

    squareGeometry(left, top, SquareWidth);

    /*-----------------------------------------------------*\
    | Create image canvas                                   |
    \*-----------------------------------------------------*/
    const qreal devicePixelRatio = devicePixelRatioF();
    const int squarePixelWidth = qMax(2, qRound(SquareWidth * devicePixelRatio));
    const qreal squarePixelCoordinateRange = (qreal)(squarePixelWidth - 1);
    QImage square(squarePixelWidth, squarePixelWidth, QImage::Format_ARGB32_Premultiplied);

    /*-----------------------------------------------------*\
    | The fully saturated, full brightness colour of this   |
    | hue. Every pixel is a blend of white, this colour and |
    | black, so no per-pixel colour conversion is needed.   |
    \*-----------------------------------------------------*/
    const QColor pure = QColor::fromHsv((hue < 0) ? 0 : hue, 255, 255);
    const qreal pure_r = pure.redF();
    const qreal pure_g = pure.greenF();
    const qreal pure_b = pure.blueF();

    /*-----------------------------------------------------*\
    | Paint the square. X axis is saturation (none on the   |
    | left, full on the right). Y axis is value (full at    |
    | the top, none at the bottom).                         |
    \*-----------------------------------------------------*/
    for(int y = 0; y < squarePixelWidth; y++)
    {
        QRgb *line = reinterpret_cast<QRgb *>(square.scanLine(y));
        const qreal value = 1.0 - ((qreal)y / squarePixelCoordinateRange);

        for(int x = 0; x < squarePixelWidth; x++)
        {
            const qreal saturation = (qreal)x / squarePixelCoordinateRange;

            const qreal red     = value * (1.0 - (saturation * (1.0 - pure_r)));
            const qreal green   = value * (1.0 - (saturation * (1.0 - pure_g)));
            const qreal blue    = value * (1.0 - (saturation * (1.0 - pure_b)));

            line[x] = qRgb(qRound(red * 255.0), qRound(green * 255.0), qRound(blue * 255.0));
        }
    }

    /*-----------------------------------------------------*\
    | Round the corners with a smooth (antialiased) mask    |
    \*-----------------------------------------------------*/
    {
        QPainter mask(&square);
        mask.setRenderHint(QPainter::Antialiasing);
        mask.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        mask.setPen(Qt::NoPen);
        mask.setBrush(Qt::black);

        const qreal corner = squareCornerRadius() * devicePixelRatio;
        mask.drawRoundedRect(QRectF(0.0, 0.0, squarePixelWidth, squarePixelWidth), corner, corner);
    }

    square.setDevicePixelRatio(devicePixelRatio);

    /*-----------------------------------------------------*\
    | Store the square image at the target device resolution |
    \*-----------------------------------------------------*/
    squareImage = square;

    /*-----------------------------------------------------*\
    | Calculate square region                               |
    \*-----------------------------------------------------*/
    const int squareX = qRound(left);
    const int squareY = qRound(top);
    const int squareSize = qMax(1, qRound(SquareWidth));
    squareRegion = QRegion(squareX, squareY, squareSize, squareSize);
    CleanSquare = squareImage;
}

void ColorWheel::drawIndicator(const int &hue)
{
    QPainter painter(&wheel);
    painter.setRenderHint(QPainter::Antialiasing);

    const int safe_hue = qBound(0, hue, 359);

    const qreal size = qMin(height(), width());
    const qreal center = size / 2.0;
    const qreal outerRadius = center - margin;
    const qreal innerRadius = outerRadius - wheelWidth;
    const qreal indicatorRadius = (outerRadius + innerRadius) / 2.0;
    const qreal handleRadius = qBound(6.0, (wheelWidth / 2.0) + 0.5, 11.0);

    painter.translate(x_offset + center, y_offset + center);
    painter.rotate( -safe_hue );

    drawHandle(painter, QPointF(indicatorRadius, 0.0), handleRadius, QColor::fromHsv(safe_hue, 255, 255));
}

void ColorWheel::drawPicker(const QColor &color)
{
    QPainter painter(&wheel);
    painter.setRenderHint(QPainter::Antialiasing);

    qreal left;
    qreal top;
    qreal SquareWidth;

    squareGeometry(left, top, SquareWidth);

    /*-----------------------------------------------------*\
    | Saturation grows to the right, value grows upwards    |
    \*-----------------------------------------------------*/
    const qreal S = color.saturationF() * SquareWidth;
    const qreal V = (1.0 - color.valueF()) * SquareWidth;

    drawHandle(painter, QPointF(left + S, top + V), 8.0, QColor(color.rgb()));
}

void ColorWheel::composeWheel()
{
    wheel = CleanWheel;
    squareImage = CleanSquare;

    QPainter composePainter(&wheel);
    composePainter.setRenderHint(QPainter::Antialiasing);

    /*-----------------------------------------------------*\
    | The square, with a faint outline so that its dark     |
    | edge does not melt into the background                |
    \*-----------------------------------------------------*/
    const QRect square_rect = squareRegion.boundingRect();
    composePainter.drawImage(square_rect.topLeft(), squareImage);

    const qreal corner = squareCornerRadius();
    composePainter.setBrush(Qt::NoBrush);
    composePainter.setPen(QPen(QColor(255, 255, 255, 38), 1.0));
    composePainter.drawRoundedRect(QRectF(square_rect).adjusted(0.5, 0.5, -0.5, -0.5), corner, corner);
    composePainter.end();

    drawIndicator(current.hue());
    drawPicker(current);
}

void ColorWheel::hueChanged(const int &hue)
{
    if((hue < 0) || (hue > 359))
    {
        return;
    }

    int s = current.saturation();
    int v = current.value();
    current.setHsv(hue, s, v);

    drawSquareImage(hue);

    if(!isVisible())
    {
        return;
    }
    repaint();

    emit colorChanged(current);
}

void ColorWheel::svChanged(const QColor &newcolor)
{
    int hue = current.hue();

    current.setHsv
            (
            hue,
            newcolor.saturation(),
            newcolor.value()
            );

    if(!isVisible())
    {
        return;
    }

    repaint();

    emit colorChanged(current);
}
