#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "videoprocessor.h"

#include <QQuickStyle>

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle("Basic");
    QGuiApplication app(argc, argv);

    qmlRegisterType<VideoProcessor>("OCRVideoApp", 1, 0, "VideoProcessor");

    QQmlApplicationEngine engine;

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("OCRVideoApp", "Main");
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load QML module OCRVideoApp/Main";
        return -1;
    }

    return app.exec();
}
