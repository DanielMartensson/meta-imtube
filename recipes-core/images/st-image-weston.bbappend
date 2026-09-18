# Bake the UTF-8 locale profile snippet into every image that runs the Qt
# stack (imtube-qt, wpeqt, opennow).
IMAGE_INSTALL:append = " qtenv"