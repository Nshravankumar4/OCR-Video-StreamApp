#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "videoprocessor.h"
#include <QQuickStyle>

// main() - Entry point. Runs once when app starts.
// Order of operations:
//   1. Set Qt Quick style to "Basic" (plain look, no platform theming)
//   2. Create QGuiApplication (required by Qt for all GUI apps)
//   3. Register VideoProcessor C++ class so QML can use it as "VideoProcessor"
//   4. Create QQmlApplicationEngine and load Main.qml
//   5. If QML fails to load, exit with error code -1
//   6. Enter Qt event loop (app.exec) - stays here until window is closed
int main(int argc, char *argv[])
{
    QQuickStyle::setStyle("Basic");        // Use plain style (no OS theming)
    QGuiApplication app(argc, argv);       // Create Qt application object

    // Make VideoProcessor available in QML as: import OCRVideoApp; VideoProcessor {}
    qmlRegisterType<VideoProcessor>("OCRVideoApp", 1, 0, "VideoProcessor");

    QQmlApplicationEngine engine;          // Engine that loads and runs QML

    // Exit with code -1 if any QML object fails to create
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("OCRVideoApp", "Main");  // Load qml/Main.qml

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load QML module OCRVideoApp/Main";
        return -1;
    }

    return app.exec();  // Start event loop - app runs here until closed
}
