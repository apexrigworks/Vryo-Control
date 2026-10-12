/*---------------------------------------------------------*\
| startup.cpp                                               |
|                                                           |
|   Startup for the OpenRGB application                     |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "cli.h"
#include "ResourceManager.h"
#include "NetworkServer.h"
#include "startup.h"

#include <algorithm>
#include <cmath>

#include <QApplication>
#include <QElapsedTimer>
#include <QPixmap>
#include <QSplashScreen>
#include <QTimer>

#include "OpenRGBDialog.h"
#include "DetectionManager.h"
#include "VryoTheme.h"

#ifdef __APPLE__
#include "macutils.h"
#endif

#ifdef __linux__
#include <csignal>
#endif

/******************************************************************************************\
*                                                                                          *
*   Linux signal handler                                                                   *
*                                                                                          *
\******************************************************************************************/
#ifdef __linux__
void sigHandler(int s)
{
    std::signal(s, SIG_DFL);
    qApp->quit();
}
#endif

/******************************************************************************************\
*                                                                                          *
*   startup                                                                                *
*                                                                                          *
*       Opens the main windows or starts the server                                        *
*                                                                                          *
\******************************************************************************************/
int startup(int argc, char* argv[], unsigned int ret_flags)
{
    /*-----------------------------------------------------*\
    | Initialize exit value, which will be returned on exit |
    | in main()                                             |
    \*-----------------------------------------------------*/
    int exitval = EXIT_SUCCESS;

    /*-----------------------------------------------------*\
    | Before opening GUI, wait for automatic connection so  |
    | that settings and profiles can be updated from server |
    \*-----------------------------------------------------*/
    ResourceManager::get()->WaitForInitialization();

    /*-----------------------------------------------------*\
    | If the command line parser indicates that the GUI     |
    | should run, or if there were no command line          |
    | arguments, start the GUI.                             |
    \*-----------------------------------------------------*/
    if(ret_flags & RET_FLAG_START_GUI)
    {
        LOG_TRACE("[main] initializing GUI");

        /*-------------------------------------------------*\
        | Enable high DPI scaling support (Qt5 only)        |
        \*-------------------------------------------------*/
        #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps,    true);
            QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
        #endif

        /*-------------------------------------------------*\
        | Enable high DPI fractional scaling support on     |
        | Windows                                           |
        \*-------------------------------------------------*/
        #if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0) && defined(Q_OS_WIN)
            QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
        #endif

        /*-------------------------------------------------*\
        | Create Qt application                             |
        \*-------------------------------------------------*/
        QApplication a(argc, argv);
        QGuiApplication::setDesktopFileName("org.openrgb.OpenRGB");
        QGuiApplication::setApplicationDisplayName(QStringLiteral("Vryo Control"));
        LOG_TRACE("[startup] QApplication created");

        /*-------------------------------------------------*\
        | Vryo dark and blue theme                          |
        \*-------------------------------------------------*/
        VryoTheme::Apply(a);

        /*-------------------------------------------------*\
        | Animated loading screen, shown until the first    |
        | device scan finishes (or a few seconds pass)      |
        \*-------------------------------------------------*/
        QSplashScreen * splash          = nullptr;
        QTimer *        splash_timer    = nullptr;
        QElapsedTimer   splash_clock;
        QPixmap         splash_logo(QStringLiteral(":vryo_splash_logo.png"));
        bool            splash_seen_scan = false;
        double          splash_progress  = 0.0;

        if(!(ret_flags & RET_FLAG_START_MINIMIZED))
        {
            splash = new QSplashScreen(VryoTheme::PaintSplash(splash_logo, 0.0, 0.0, 0.0, QStringLiteral("Starting up...")));
            splash->setWindowFlag(Qt::WindowStaysOnTopHint, true);
            splash->show();
            splash_clock.start();
            a.processEvents();
        }

        /*-------------------------------------------------*\
        | Main UI widget                                    |
        \*-------------------------------------------------*/
        OpenRGBDialog dlg;
        LOG_TRACE("[startup] Dialog created");

        if(ret_flags & RET_FLAG_I2C_TOOLS)
        {
            dlg.AddI2CToolsPage();
        }

        dlg.AddClientTab();

        if(ret_flags & RET_FLAG_START_MINIMIZED)
        {
#ifdef _WIN32
            /*---------------------------------------------*\
            | Show the window always, even if it will       |
            | immediately be hidden.  On Windows, events    |
            | are not delivered to nativeEventFilter (for   |
            | SuspendResume) until the window has been      |
            | shown once.                                   |
            |                                               |
            | TODO Try using                                |
            | RegisterSuspendResumeNotification instead,    |
            | that should work in headless mode too         |
            \*---------------------------------------------*/
            dlg.showMinimized();
#endif
#ifdef __APPLE__
            MacUtils::ToggleApplicationDocklessState(false);
#endif
            dlg.hide();
        }
        else if(splash == nullptr)
        {
            dlg.show();
        }

        if(splash != nullptr)
        {
            /*---------------------------------------------*\
            | The main window stays hidden (it is not shown |
            | above) while the splash animates, then shown  |
            \*---------------------------------------------*/
            splash_timer = new QTimer(&a);
            splash_timer->setInterval(16);

            QObject::connect(splash_timer, &QTimer::timeout, [&]()
            {
                const qint64        elapsed_ms  = splash_clock.elapsed();
                const unsigned int  percent     = DetectionManager::get()->GetDetectionPercent();

                if(percent < 100)
                {
                    splash_seen_scan = true;
                }

                /*-----------------------------------------*\
                | Finished when the scan is done, but never |
                | before 1.5 s (so it is readable) and never|
                | after 12 s (so it cannot get stuck)       |
                \*-----------------------------------------*/
                const bool scan_done    = splash_seen_scan && (percent >= 100);
                const bool no_scan_seen = !splash_seen_scan && (elapsed_ms >= 2500);
                const bool finished     = (elapsed_ms >= 1500) && (scan_done || no_scan_seen || (elapsed_ms >= 12000));

                /*-----------------------------------------*\
                | Smooth bar: eases toward 90 % over time,  |
                | follows the real scan if it is further on |
                \*-----------------------------------------*/
                double target = 0.9 * (1.0 - std::exp(-(double)elapsed_ms / 2200.0));

                if(splash_seen_scan)
                {
                    target = (std::max)(target, 0.9 * ((double)percent / 100.0));
                }

                if(finished)
                {
                    target = 1.0;
                }

                splash_progress += (target - splash_progress) * 0.12;

                if(finished && (splash_progress > 0.985))
                {
                    splash_timer->stop();
                    dlg.show();
                    splash->finish(&dlg);
                    delete splash;
                    splash = nullptr;
                    return;
                }

                const double angle  = std::fmod((double)elapsed_ms * 0.36, 360.0);
                const double phase  = std::fmod((double)elapsed_ms / 1100.0, 1.0);

                QString message = QStringLiteral("Starting up...");

                if(splash_seen_scan && (percent < 100))
                {
                    message = QStringLiteral("Detecting devices...");
                }
                else if(finished || (splash_progress > 0.8))
                {
                    message = QStringLiteral("Almost ready...");
                }

                splash->setPixmap(VryoTheme::PaintSplash(splash_logo, angle, splash_progress, phase, message));
            });

            splash_timer->start();
        }

        LOG_TRACE("[main] Ready to exec() the dialog");

#ifdef __linux__
        std::signal(SIGINT,  sigHandler);
        std::signal(SIGTERM, sigHandler);
#endif

        exitval = a.exec();
    }
    else
    {
        /*-------------------------------------------------*\
        | If no GUI is needed, we let the background        |
        | threads run as long as they need, but we need to  |
        | AT LEAST wait for initialization to finish        |
        \*-------------------------------------------------*/
        ResourceManager::get()->WaitForInitialization();

        if(ret_flags & RET_FLAG_START_SERVER)
        {
            NetworkServer* server = ResourceManager::get()->GetServer();
            if(server)
            {
                exitval = !server->GetOnline();
            }
            else
            {
                exitval = EXIT_FAILURE;
            }
        }
    }

    return(exitval);
}
