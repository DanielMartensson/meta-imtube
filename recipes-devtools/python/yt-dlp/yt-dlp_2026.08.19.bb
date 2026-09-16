SUMMARY = "yt-dlp – YouTube downloader (standalone aarch64 binary)"
LICENSE = "Unlicense"
LIC_FILES_CHKSUM = "file://LICENSE;md5=7246f848faa4e9c9fc0ea91122d6e680"

# The binary ships as a self-contained ELF executable (built with
# PyInstaller) – no Python runtime is required on the target.
SRC_URI = " \
    https://github.com/yt-dlp/yt-dlp/releases/download/2026.08.19/yt-dlp_linux_aarch64;name=yt-dlp-bin;unpack=0 \
    https://raw.githubusercontent.com/yt-dlp/yt-dlp/2026.08.19/LICENSE;name=license;unpack=0 \
"

SRC_URI[yt-dlp-bin.sha256sum] = "b16e4dab368a816cd05d477d698a605a6ae87ccee1c8ffd38fa21d7254141fcc"
SRC_URI[license.sha256sum] = "7e12e5df4bae12cb21581ba157ced20e1986a0508dd10d0e8a4ab9a4cf94e85c"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/yt-dlp_linux_aarch64 ${D}${bindir}/yt-dlp
}

FILES:${PN} = "${bindir}/yt-dlp"
RDEPENDS:${PN} = "ffmpeg zlib"
INSANE_SKIP:${PN} = "already-stripped"
