#include "mainwindow.hpp"

#include <3rdparty/breakpad.hpp>
#include <3rdparty/qtsingleapplication/qtsingleapplication.h>
#include <utils/logasync.h>
#include <utils/utils.h>

#include <QApplication>
#include <QDir>
#include <QNetworkProxyFactory>
#include <QStyle>

#define AppnName "QFfmpegTranscoder"

void setAppInfo()
{
    qApp->setApplicationVersion("0.0.1");
    qApp->setApplicationDisplayName(AppnName);
    qApp->setApplicationName(AppnName);
    qApp->setDesktopFileName(AppnName);
    qApp->setOrganizationDomain("Youth");
    qApp->setOrganizationName("Youth");
    qApp->setWindowIcon(qApp->style()->standardIcon(QStyle::SP_DriveDVDIcon));
}

auto main(int argc, char *argv[]) -> int
{
#if defined(Q_OS_WIN) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (!qEnvironmentVariableIsSet("QT_OPENGL")) {
        QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
    }
#else
    qputenv("QSG_RHI_BACKEND", "opengl");
#endif
    Utils::setHighDpiEnvironmentVariable();
    SharedTools::QtSingleApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    SharedTools::QtSingleApplication app(AppnName, argc, argv);
    if (app.isRunning()) {
        qWarning() << "This is already running";
        if (app.sendMessage("raise_window_noop", 5000)) {
            return EXIT_SUCCESS;
        }
    }
#ifndef Q_OS_WIN
    Q_INIT_RESOURCE(shaders);
#endif
#ifdef Q_OS_WIN
    if (!qFuzzyCompare(qApp->devicePixelRatio(), 1.0)
        && QApplication::style()->objectName().startsWith(QLatin1String("windows"),
                                                          Qt::CaseInsensitive)) {
        QApplication::setStyle(QLatin1String("fusion"));
    }
#endif
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
    app.setAttribute(Qt::AA_DisableWindowContextHelpButton);
#endif

    Utils::BreakPad::instance();
    QDir::setCurrent(app.applicationDirPath());

    // 异步日志
    Utils::LogAsync *log = Utils::LogAsync::instance();
    log->setOrientation(Utils::LogAsync::Orientation::StdAndFile);
    log->setLogLevel(QtDebugMsg);
    log->startWork();

    Utils::printBuildInfo();
    Utils::setGlobalThreadPoolMaxSize();

    setAppInfo();

    // Make sure we honor the system's proxy settings
    QNetworkProxyFactory::setUseSystemConfiguration(true);

    MainWindow w;
    w.show();

    int result = app.exec();
    log->stop();
    return result;
}
