# meta-imtube

Lightweight YouTube client for embedded Linux made in **Qt6 QML** with
**GStreamer hardware acceleration only** – inspired by
[ImTube](https://github.com/DanielMartensson/ImTube) (ImGui + GStreamer +
yt-dlp) and FLTube (FLTK + yt-dlp).

The client is optimized for the Watermelon Wine 1A platform (STMicroelectronics
STM32MP2x), where it integrates with the meta-qt6 / meta-st stacks used by
`watermelon-wine-os`.

## What makes it different

* **Hardware only.** Video is decoded by the platform hardware decoder
  (v4l2slh264dec / v4l2h264dec, auto-plugged) and rendered by the GPU inside the
  QML scene graph via the `qml6glsink` element of gst-plugins-good. No software
  decoding and no CPU-side frame copies.
* **Vulkan ready.** The app detects the renderer at startup:
  * `qml6vulkansink` present (newer gst-plugins-good / GStreamer >= 1.28)
    → the scene graph runs on **Vulkan** and the sink renders through it.
  * otherwise → OpenGL/OpenGL ES scene graph with `qml6glsink` (still 100% GPU).
* **yt-dlp as streamer.** Instead of messy URL handshakes the helper tool
  downloads/remuxes the best fitting stream and writes a single MKV to stdout,
  which is fed into a GStreamer `appsrc`. Seeking restarts the download with
  `--download-sections "*H:MM:SS-"`.

## Layer layout

```
meta-imtube/
├── conf/layer.conf
├── recipes-devtools/python/yt-dlp/yt-dlp_2026.08.19.bb   # standalone binary
├── recipes-multimedia/
│   ├── gstreamer/gstreamer1.0-plugins-good_%.bbappend   # enables the Qt6 sink
│   ├── imtube-qt/imtube-qt_git.bb                       # the Qt6 client
│   └── packagegroups/packagegroup-imtube.bb
└── src/                                                  # imtube-qt source
```

## Integrating into watermelon-wine-os

1. Make sure the layer is on disk (this repo is used as-is) and add it to
   `build-openstlinuxweston-stm32mp25-mx-watermelon-wine-1a/conf/bblayers.conf`:

   ```
   BBLAYERS =+ "/home/mint/Documents/Github/meta-imtube"
   ```

2. Add the packagegroup to `conf/local.conf`:

   ```
   CORE_IMAGE_EXTRA_INSTALL:append = " packagegroup-imtube"
   ```

3. Build the image. `imtube-qt` depends on:

   * meta-qt6 (`qtbase`, `qtdeclarative`, `qtwayland`, `qtshadertools-native`)
   * GStreamer 1.26.11 (the layer's bbappend enables the `qt6` plugin in
     gst-plugins-good: `-Dqt6=enabled -Dqt-egl=enabled -Dqt-wayland=enabled`)
   * `yt-dlp` (standalone aarch64 binary, no Python needed) + `ffmpeg`

Run the client on the target:

```
imtube-qt
```

## Desktop / host testing

Install a local copy of yt-dlp without root:

```
./tools/install_yt-dlp.sh
```

To build the client for desktop you need Qt 6.5+ (Quick, QuickControls2),
gstreamer-1.0 + gstreamer-app-1.0 dev packages:

```
cmake -S src -B build && cmake --build build
```

The renderer can be overridden for testing:

```
IMTUBE_VIDEO_SINK=fakesink ./imtube-qt
```