#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCoreApplication>
#include <QLibraryInfo>
#include <QDir>
#include <QUrl>
#include <QDebug>
#include <QFile>
#include "NetworkClient.h"
#include "FileDialogHelper.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Set Qt Quick Controls style via environment variable
    // This allows customization of Button background and contentItem
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");

    // Set application properties
    app.setApplicationName("Ai là triệu phú");
    app.setOrganizationName("LTM_Group04");

    // Create NetworkClient instance
    NetworkClient *networkClient = new NetworkClient(&app);
    
    // Create FileDialogHelper instance
    FileDialogHelper *fileDialogHelper = new FileDialogHelper(&app);
    
    // Create QML engine
    QQmlApplicationEngine engine;
    
    // Expose NetworkClient to QML as global property
    engine.rootContext()->setContextProperty("networkClient", networkClient);
    engine.rootContext()->setContextProperty("fileDialogHelper", fileDialogHelper);
    
    // Add QML import paths (Qt6 installation path)
    QStringList importPathList = engine.importPathList();
    
    // Method 1: Use QLibraryInfo (standard way)
    QString qt6QmlPath = QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
    if (!qt6QmlPath.isEmpty()) {
        engine.addImportPath(qt6QmlPath);
        qDebug() << "Added QML import path (QLibraryInfo):" << qt6QmlPath;
    }
    
    // Method 2: Try common Qt6 installation paths on Windows
    QStringList commonPaths = {
        "C:/Qt/6.10.1/mingw_64/qml",
        "C:/Qt/6.10.1/mingw_64/qml",
        "C:/Qt/6.10.0/mingw_64/qml",
        "C:/Qt/6.9.2/mingw_64/qml",
        "C:/Qt/6.10.1/msvc2022_64/qml",
    };
    
    for (const QString &path : commonPaths) {
        QDir dir(path);
        if (dir.exists()) {
            engine.addImportPath(path);
            qDebug() << "Added QML import path (common):" << path;
        }
    }
    
    // Debug: Print all import paths
    qDebug() << "QML Import Paths:";
    for (const QString &path : engine.importPathList()) {
        qDebug() << "  -" << path;
    }
    
    // Try to load from resource first, fallback to file system
    QUrl url(QStringLiteral("qrc:/MillionaireClient/qml/main.qml"));
    
    // If resource doesn't work, try file system
    if (!QFile::exists(":/MillionaireClient/qml/main.qml")) {
        QDir appDir(QCoreApplication::applicationDirPath());
        appDir.cdUp();
        QString qmlPath = appDir.absoluteFilePath("qml/main.qml");
        if (QFile::exists(qmlPath)) {
            url = QUrl::fromLocalFile(qmlPath);
            qDebug() << "Loading QML from file system:" << url;
        } else {
            qDebug() << "QML file not found in resource or file system";
        }
    }
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    
    engine.load(url);

    return app.exec();
}

