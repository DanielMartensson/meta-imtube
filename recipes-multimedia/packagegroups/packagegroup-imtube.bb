SUMMARY = "Packagegroup for ImTube Qt6 YouTube client"
LICENSE = "MIT"
inherit packagegroup

RDEPENDS:${PN} = " \
    imtube-qt \
    yt-dlp \
    ffmpeg \
"
