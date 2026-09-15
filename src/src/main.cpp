#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSGRendererInterface>

#include <gst/gst.h>

#include "core/GStreamerPlayer.h"
#include "core/LibraryStore.h"
#include "core/Settings.h"
#include "core/ThumbnailService.h"
#include "core/VideoItem.h"
#include "core/VideoListModel.h"
#include "core/VideoSinkFactory.h"
#include "core/YtDlpHelper.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("WatermelonWine"));
    app.setOrganizationDomain(QStringLiteral("watermelon-wine"));
    app.setApplicationName(QStringLiteral("imtube-qt"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));

    // ------------------------------------------------------------------
    // GStreamer must be initialised and probed BEFORE the QML engine is
    // created: the "qml6" plugin registers the QML type
    // (org.freedesktop.gstreamer.Qt6GLVideoItem) the moment it loads.
    // ------------------------------------------------------------------
    gst_init(&argc, &argv);

    // Pick the scene-graph graphics API and the matching GStreamer video
    // sink. Vulkan is used when the running GStreamer stack ships a Vulkan
    // capable QML sink (qml6vulkansink, gst-plugins-good >= 1.28); otherwise
    // the scene graph runs fully hardware accelerated on OpenGL/OpenGL ES
    // with qml6glsink. Either way the video never touches the CPU.
    VideoSinkFactory sinkFactory;
    if (!qEnvironmentVariableIsSet("QSG_RHI_BACKEND"))
        QQuickWindow::setGraphicsApi(sinkFactory.graphicsApi());

    // Force the chosen sink plugin to dlopen now (registers the QML types).
    sinkFactory.preload();

    Settings settings;
    settings.setRendererDescription(sinkFactory.describe());

    // ------------------------------------------------------------------
    // C++ services, exposed to QML as context properties + models.
    // ------------------------------------------------------------------
    YtDlpHelper ytDlp(&settings);
    ThumbnailService thumbs(&settings);
    LibraryStore library(&settings);
    GStreamerPlayer player(&ytDlp, &sinkFactory, &settings);

    VideoListModel searchModel;
    VideoListModel libraryModel;
    QObject::connect(&ytDlp, &YtDlpHelper::searchFinished, &searchModel,
                     &VideoListModel::setVideos);
    QObject::connect(&library, &LibraryStore::favoritesChanged, &libraryModel,
                     [&libraryModel, &library]() {
                         libraryModel.setVideos(library.favorites());
                     });
    libraryModel.setVideos(library.favorites());

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("settings"), &settings);
    engine.rootContext()->setContextProperty(QStringLiteral("ytDlp"), &ytDlp);
    engine.rootContext()->setContextProperty(QStringLiteral("thumbs"), &thumbs);
    engine.rootContext()->setContextProperty(QStringLiteral("library"), &library);
    engine.rootContext()->setContextProperty(QStringLiteral("player"), &player);
    engine.rootContext()->setContextProperty(QStringLiteral("searchModel"), &searchModel);
    engine.rootContext()->setContextProperty(QStringLiteral("libraryModel"), &libraryModel);

    qmlRegisterType<VideoListModel>("ImTube", 1, 0, "VideoListModel");
    qmlRegisterUncreatableType<VideoItem>("ImTube", 1, 0, "VideoItem",
                                          QStringLiteral("Created by C++ only"));
    qmlRegisterUncreatableType<GStreamerPlayer>("ImTube", 1, 0, "PlaybackControl",
                                                QStringLiteral("Enums only"));
    qmlRegisterUncreatableType<LibraryStore>("ImTube", 1, 0, "Library",
                                             QStringLiteral("Enums only"));

    const QUrl url(QStringLiteral("qrc:/qml/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app,
                     [url](QObject *obj, const QUrl &objUrl) {
                         if (!obj && url == objUrl)
                             qFatal("ImTube: failed to load %s", qPrintable(objUrl.toString()));
                     },
                     Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}