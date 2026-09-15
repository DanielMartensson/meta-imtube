# Enable the Qt6 QML video sink plugin (qml6glsink) used by imtube-qt.
# Decoded frames are rendered on the GPU inside the QML scene graph; no
# CPU-side frame copies.
#
# This applies to the ST/OE-matched gst-plugins-good recipe from OE-core
# (1.22.x): the qt6 meson feature already exists there since GStreamer 1.22,
# it just needs the Qt6 stack from meta-qt6 to be wired in.
#
# The plugin needs host tools (qmake6/qsb/qmltyperegistrar) provided by the
# -native Qt6 packages listed in the depend field below.

QT6WAYLANDDEPENDS = "${@bb.utils.contains('DISTRO_FEATURES', 'wayland', 'qtwayland', '', d)}"
QT6WAYLAND = "${@bb.utils.contains('DISTRO_FEATURES', 'wayland', 'enabled', 'disabled', d)}"

PACKAGECONFIG:append = " qt6"

#                 enable                                              disable
PACKAGECONFIG[qt6] = "-Dqt6=enabled \
                      -Dqt-egl=enabled \
                      -Dqt-wayland=${QT6WAYLAND} \
                      -Dqt-x11=disabled, \
                      -Dqt6=disabled, \
                      qtbase qtdeclarative qtbase-native qtdeclarative-native qtshadertools-native ${QT6WAYLANDDEPENDS}"

# The Qt6 plugin builds as libgstqml6.so; split it into its own package so
# imtube-qt can depend on the sink explicitly.
PACKAGES:append = " ${PN}-qml6"
FILES:${PN}-qml6 = "${libdir}/gstreamer-1.0/libgstqml6.so"
RDEPENDS:${PN}-qml6 += "qtwayland qtdeclarative"