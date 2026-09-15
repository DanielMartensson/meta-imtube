#pragma once

#include <QSGRendererInterface>
#include <QString>

#include <gst/gst.h>

// ---------------------------------------------------------------------------
// Chooses the scene-graph graphics API (Vulkan when possible, otherwise
// OpenGL/OpenGL ES) and the matching GStreamer video sink element used to
// embed decoded frames into the QML item hierarchy.
//
//   Vulkan  -> qml6vulkansink   (GStreamer >= 1.28 / newer gst-plugins-good)
//   OpenGL  -> qml6glsink       (GStreamer >= 1.22, both GL and GLES)
//
// Both sinks render the video on the GPU with no host-side copies.
// ---------------------------------------------------------------------------
class VideoSinkFactory
{
public:
    VideoSinkFactory();

    // Probe the GStreamer plugin registry and remember the best sink.
    void probe();

    // Instantiate (and immediately drop) the sink element. This forces the
    // plugin to dlopen and run its qt_register_types() / qmlRegisterType code
    // so the org.freedesktop.gstreamer.* QML module exists before the QML
    // engine loads Main.qml. Call before creating the QQmlApplicationEngine.
    void preload() const;

    QSGRendererInterface::GraphicsApi graphicsApi() const;
    bool vulkanCapable() const;

    // Element factory name to use in the pipeline (e.g. "qml6glsink").
    QString sinkElement() const;
    bool sinkAvailable() const;

    // Human readable description used on the Settings page.
    QString describe() const;

private:
    bool m_vulkanCapable = false;
    QString m_sinkElement;
    bool m_sinkAvailable = false;
};