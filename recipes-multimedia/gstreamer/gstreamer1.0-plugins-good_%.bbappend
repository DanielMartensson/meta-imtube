# Enable the Qt6 QML video sink plugin (qml6glsink / qml6vulkansink) used
# by imtube-qt. Decoded frames are rendered on the GPU inside the QML scene
# graph; no CPU-side frame copies.
#
# Requires EGL + gles2 + the Qt6 stack from meta-qt6.
#
# qt-method is left at the meson default (auto -> qmake): since GStreamer
# 1.28 the legacy 'ninja' method no longer exists, and a host qmake6/qsb is
# provided by the qtbase-native/qtshadertools-native deps below.

QT6WAYLANDDEPENDS = "${@bb.utils.contains('DISTRO_FEATURES', 'wayland', 'qtwayland', '', d)}"

PACKAGECONFIG:append = " qt6"

#                 enable                                                    disable
PACKAGECONFIG[qt6] = "-Dqt6=enabled \
                      -Dqt-egl=enabled \
                      -Dqt-wayland=${@bb.utils.contains('DISTRO_FEATURES','wayland','enabled','disabled',d)} \
                      -Dqt-x11=disabled, \
                      -Dqt6=disabled, \
                      qtbase qtdeclarative qtbase-native qtshadertools-native ${QT6WAYLANDDEPENDS}"

RDEPENDS:${PN}-qml6 += "qtwayland"
