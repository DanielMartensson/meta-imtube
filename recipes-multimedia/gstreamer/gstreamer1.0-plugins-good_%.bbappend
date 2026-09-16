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
                      qtbase qtdeclarative qtbase-native qtdeclarative-native qtshadertools-native qttools-native ${QT6WAYLANDDEPENDS}"

# meta-qt6 installs the Qt6 host tools (moc/uic/rcc/lrelease) in the native
# sysroot without a -qt6 suffix; meson's qt6 module looks them up as
# <tool>-qt6 on PATH (pkg-config detection). Provide tagged symlinks so the
# qml6 sink builds. lrelease comes from qttools-native.
do_configure:prepend() {
    for t in moc uic rcc lrelease; do
        for src in ${STAGING_BINDIR_NATIVE}/${t} \
                   ${STAGING_LIBEXECDIR_NATIVE}/${t} \
                   ${STAGING_LIBEXECDIR_NATIVE}/qt6/libexec/${t} \
                   ${STAGING_LIBDIR_NATIVE}/qt6/libexec/${t}; do
            if [ -e "${src}" ]; then
                ln -sf "${src}" "${STAGING_BINDIR_NATIVE}/${t}-qt6"
                break
            fi
        done
    done
}

# The Qt6 plugin builds as libgstqml6.so; split it into its own package so
# imtube-qt can depend on the sink explicitly.
PACKAGES:append = " ${PN}-qml6"
FILES:${PN}-qml6 = "${libdir}/gstreamer-1.0/libgstqml6.so"
RDEPENDS:${PN}-qml6 += "qtwayland qtdeclarative"