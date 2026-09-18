# Qt 6 requires a UTF-8 aware locale; the raw "C" locale (ANSI_X3.4-1968)
# makes QML/QPA fail its locale check. en_US.UTF-8 is generated in the image
# (glibc-binary-localedata-en-us / locale-base-en-us).
export LANG=en_US.UTF-8
export LC_ALL=en_US.UTF-8