SUMMARY = "ImTube Qt – lightweight YouTube client"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=19b5c44dd600b05d43b5bb78ad0bd491"

inherit qt6-cmake pkgconfig

BRANCH = "main"
SRC_URI = "git://github.com/DanielMartensson/meta-imtube.git;branch=${BRANCH};subdir=src"
# Pin after the first push; ${AUTOREV} resolves against the HEAD commit.
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git/src"

DEPENDS = " \
    qtbase \
    qtdeclarative \
    gstreamer1.0 \
    gstreamer1.0-plugins-base \
"

RDEPENDS:${PN} = " \
    yt-dlp \
    ffmpeg \
    gstreamer1.0-plugins-good-qml6 \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    qtdeclarative \
    qtwayland \
"

PACKAGECONFIG ??= ""
