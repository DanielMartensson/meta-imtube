SUMMARY = "Set a UTF-8 locale for Qt applications"
DESCRIPTION = "Installs an /etc/profile.d snippet that exports LANG/LC_ALL so \
Qt 6 applications (imtube-qt, wpeqt, opennow) never run under the raw \"C\" \
ASCII locale, which Qt refuses to use as its UTF-8 locale check fails."
HOMEPAGE = "https://github.com/DanielMartensson/meta-imtube"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

SRC_URI = "file://qtenv-locale.sh"

do_install() {
    install -d ${D}${sysconfdir}/profile.d
    install -m 0644 ${WORKDIR}/qtenv-locale.sh ${D}${sysconfdir}/profile.d/qtenv-locale.sh
}

FILES:${PN} = "${sysconfdir}/profile.d/qtenv-locale.sh"