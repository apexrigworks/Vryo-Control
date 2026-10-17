/*---------------------------------------------------------*\
| VryoUiFx.cpp                                              |
|                                                           |
|   App-wide interface effects (button click ripple).       |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "VryoUiFx.h"

#include <cmath>

#include <QAbstractButton>
#include <QCheckBox>
#include <QCoreApplication>
#include <QEasingCurve>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QRadioButton>
#include <QVariantAnimation>
#include <QWidget>

#include "../../startup/VryoTheme.h"

namespace
{
    /*-----------------------------------------------------*\
    | A see-through widget laid over a button for a moment. |
    | It draws one expanding, fading circle and then        |
    | deletes itself.                                       |
    \*-----------------------------------------------------*/
    class VryoRipple : public QWidget
    {
    public:
        VryoRipple(QWidget * host, const QPointF& click_point) :
            QWidget(host),
            origin(click_point)
        {
            setAttribute(Qt::WA_TransparentForMouseEvents, true);
            setGeometry(host->rect());
            show();
            raise();

            animation = new QVariantAnimation(this);
            animation->setStartValue(0.0);
            animation->setEndValue(1.0);
            animation->setDuration(550);
            animation->setEasingCurve(QEasingCurve(QEasingCurve::OutCubic));

            connect(animation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
            {
                progress = value.toDouble();
                update();
            });

            connect(animation, &QVariantAnimation::finished, this, [this]()
            {
                deleteLater();
            });

            animation->start();
        }

    protected:
        void paintEvent(QPaintEvent *) override
        {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing, true);

            /*---------------------------------------------*\
            | Keep the ripple inside the button's rounded   |
            | corners (the theme uses a 10 px radius)       |
            \*---------------------------------------------*/
            QPainterPath clip;
            clip.addRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 10.0, 10.0);
            painter.setClipPath(clip);

            const qreal reach = std::sqrt((qreal)((width() * width()) + (height() * height())));

            QColor colour = VryoTheme::HexColor(VryoTheme::VRYO_COLOR_ACCENT_LIGHT);
            colour.setAlphaF(0.38 * (1.0 - progress));

            painter.setPen(Qt::NoPen);
            painter.setBrush(colour);
            painter.drawEllipse(origin, reach * progress, reach * progress);
        }

    private:
        QPointF             origin;
        qreal               progress    = 0.0;
        QVariantAnimation * animation   = nullptr;
    };
}

VryoUiFx::VryoUiFx(QObject * parent) :
    QObject(parent)
{
}

void VryoUiFx::Install()
{
    static bool installed = false;

    if(installed || (QCoreApplication::instance() == nullptr))
    {
        return;
    }

    QCoreApplication::instance()->installEventFilter(new VryoUiFx(QCoreApplication::instance()));
    installed = true;
}

bool VryoUiFx::eventFilter(QObject * watched, QEvent * event)
{
    if(event->type() == QEvent::MouseButtonPress)
    {
        QAbstractButton * button = qobject_cast<QAbstractButton *>(watched);

        if((button != nullptr) && button->isEnabled()
         && (qobject_cast<QCheckBox *>(button) == nullptr)
         && (qobject_cast<QRadioButton *>(button) == nullptr))
        {
            QMouseEvent * mouse = static_cast<QMouseEvent *>(event);

            if(mouse->button() == Qt::LeftButton)
            {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                new VryoRipple(button, mouse->position());
#else
                new VryoRipple(button, mouse->localPos());
#endif
            }
        }
    }

    return(QObject::eventFilter(watched, event));
}
