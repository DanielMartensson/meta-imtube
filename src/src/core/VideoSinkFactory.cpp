#include "VideoSinkFactory.h"

#include <QByteArray>
#include <QDebug>
#include <QProcessEnvironment>

VideoSinkFactory::VideoSinkFactory()
{
    probe();
}

void VideoSinkFactory::probe()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString forced = env.value(QStringLiteral("IMTUBE_VIDEO_SINK"));

    GstElementFactory *vulkanSink = gst_element_factory_find("qml6vulkansink");
    GstElementFactory *glSink = gst_element_factory_find("qml6glsink");

    m_vulkanCapable = vulkanSink != nullptr;
    m_sinkAvailable = vulkanSink || glSink;

    if (!forced.isEmpty()) {
        // Explicit override for testing / non-QML sinks (fakesink, ...).
        m_sinkElement = forced;
        if (m_sinkElement.contains(QStringLiteral("vulkan")))
            m_vulkanCapable = true;
        qInfo() << "ImTube: video sink forced to" << m_sinkElement;
    } else if (m_vulkanCapable) {
        m_sinkElement = QStringLiteral("qml6vulkansink");
        qInfo() << "ImTube: Vulkan capable QML sink detected, using" << m_sinkElement;
    } else if (glSink) {
        m_sinkElement = QStringLiteral("qml6glsink");
        qInfo() << "ImTube: using hardware-rendered qml6glsink (Vulkan QML sink not"
                   " present in this GStreamer build)";
    } else {
        m_sinkElement.clear();
        qWarning() << "ImTube: no qml6glsink / qml6vulkansink plugin found - check"
                      "gst-plugins-good (qt6)";
    }

    if (vulkanSink)
        gst_object_unref(vulkanSink);
    if (glSink)
        gst_object_unref(glSink);
}

void VideoSinkFactory::preload() const
{
    if (m_sinkElement.isEmpty())
        return;
    GstElement *sink = gst_element_factory_make(qPrintable(m_sinkElement), "preload-check");
    if (sink) {
        qInfo() << "ImTube: preloaded QML sink" << m_sinkElement
                << "(QML types registered)";
        gst_object_unref(sink);
    } else {
        qWarning() << "ImTube: could not instantiate sink" << m_sinkElement;
    }
}

QSGRendererInterface::GraphicsApi VideoSinkFactory::graphicsApi() const
{
    // The qml GL sink shares an OpenGL context with the Qt scene graph and
    // therefore requires the scene graph to be running on OpenGL/GLES.
    // A Vulkan scene graph is only usable together with a Vulkan QML sink.
    return m_vulkanCapable ? QSGRendererInterface::Vulkan
                           : QSGRendererInterface::OpenGL;
}

bool VideoSinkFactory::vulkanCapable() const
{
    return m_vulkanCapable;
}

QString VideoSinkFactory::sinkElement() const
{
    return m_sinkElement;
}

bool VideoSinkFactory::sinkAvailable() const
{
    return m_sinkAvailable;
}

QString VideoSinkFactory::describe() const
{
    if (!m_sinkAvailable)
        return QStringLiteral("QML video sink missing (install the qt6 plugin of "
                              "gst-plugins-good)");

    if (m_vulkanCapable)
        return QStringLiteral("Vulkan (qml6vulkansink, GPU rendered)");

    return QStringLiteral("OpenGL / OpenGL ES (qml6glsink, GPU rendered)");
}