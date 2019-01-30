#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>
#include <QDebug>

#include "applicationcontroller.h"
#include "visualisationrenderer.h"

#include "portaudio.h"
#include "libnyquist/Decoders.h"
#include <QLoggingCategory>

#ifdef NDEBUG
    #define QT_NO_DEBUG
    #define QT_NO_DEBUG_OUTPUT
#endif

int main(int argc, char *argv[]) try
{
    // debug
#ifndef NDEBUG
    QLoggingCategory::setFilterRules("default.debug=true");

    qSetMessagePattern("%{message}");
#else
    QLoggingCategory::setFilterRules("default.debug=false");
#endif

    qmlRegisterType<Visualisation>("VisRenderOpenGL", 1, 0, "Visualisation");

    // init GUI
    QGuiApplication app(argc, argv);

    // app settings
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_DisableShaderDiskCache);
    QCoreApplication::setOrganizationName("Alexander Sh.");
    QCoreApplication::setApplicationName("Tunage");

    // init sound engine
    ApplicationController appController;

    // init QML
    QQmlApplicationEngine engine;
    QQmlContext *context = engine.rootContext();
    context->setContextProperty("appController", &appController);

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    // actions after QML exec

    return app.exec();
}
catch (const std::exception & e)
{
    std::cerr << "Caught: " << e.what() << std::endl;
}

