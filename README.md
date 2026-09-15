# meta-imtube

**ImTube Qt** – a lightweight YouTube client for embedded Linux, written in
**Qt 6 / QML** with **GStreamer hardware acceleration only**.

This repository *is* a Yocto/OE layer. It builds and ships the `imtube-qt`
client, the `yt-dlp` helper and the GStreamer QML-sink plumbing needed to
render video 100 % on the GPU. It is designed to sit in
`watermelon-wine-os/layers/` (works as a standalone layer anywhere else too).

> Inspired by [ImTube](https://github.com/DanielMartensson/ImTube)
> (ImGui + GStreamer + yt-dlp) and FLTube (FLTK + yt-dlp). This is a Qt6 QML
> re-implementation tuned for the Watermelon Wine 1A platform
> (STMicroelectronics STM32MP2x, Weston).

---

## Features

- **Hardware only.** Video is decoded by the platform hardware decoder
  (auto-plugged, e.g. `v4l2slh264dec`/`v4l2h264dec`, or forced in Settings)
  and rendered by the GPU inside the QML scene graph through the
  `qml6glsink` element of `gst-plugins-good`. No software decoding, no
  CPU-side frame copies.
- **Vulkan ready.** The renderer is auto-detected at startup:

  | GStreamer stack                | Scene graph API | Sink used            |
  |--------------------------------|-----------------|----------------------|
  | has `qml6vulkansink` (≥1.28)   | **Vulkan**       | `qml6vulkansink`      |
  | otherwise (1.26.11, this layer)| OpenGL / OpenGL ES | `qml6glsink`        |

  Upgrade GStreamer ≥ 1.28 in some future build and Vulkan turns on
  automatically – no layer changes needed.
- **yt-dlp as streamer.** Instead of fragile URL handshakes, `yt-dlp`
  downloads/remuxes the best fitting stream and writes a single MKV to
  stdout. That byte stream is fed into a GStreamer `appsrc`. Seeking restarts
  the download with `--download-sections "*H:MM:SS-"`.
- **Built-in library.** Favorites are persisted locally
  (`QSettings`, JSON), with local thumbnail caching.
- **Dark/light theme**, resolution cap (144–1080p), decoder selection,
  live search, and a QML-based transport bar (play/pause, seek, volume, mute).

---

## Layer layout

```
meta-imtube/
├── conf/
│   └── layer.conf                        # collection "imtube", depends on qt6-layer
├── recipes-devtools/
│   └── python/yt-dlp/
│       └── yt-dlp_2026.08.19.bb          # standalone aarch64 binary (no Python needed)
├── recipes-multimedia/
│   ├── gstreamer/
│   │   └── gstreamer1.0-plugins-good_%.bbappend   # enables the Qt6 QML sink
│   ├── imtube-qt/
│   │   └── imtube-qt_git.bb              # the Qt6 client (fetches from this repo)
│   └── packagegroups/
│       └── packagegroup-imtube.bb
├── src/                                  # imtube-qt source (CMake + QML)
│   └── src/
│       ├── core/                         # C++ backend (GStreamer player, yt-dlp, ...)
│       └── qml/                          # QtQuick UI
└── tools/
    └── install_yt-dlp.sh                 # host-only, see "Desktop testing"
```

---

## Requirements

- BitBake / OpenEmbedded (Scarthgap)
- [`meta-qt6`](https://github.com/qt-ltq/meta-qt6) – Qt 6.8.x recipes.
  Required at build time for `qt6-cmake.bbclass` and the Qt packages.
- GStreamer 1.26.11 (the version pinned by `watermelon-wine-os`)
- A Weston/Vulkan-capable build (`opengl`, `wayland`, `vulkan` in
  `DISTRO_FEATURES`) – already set in the Watermelon Wine 1A build.

---

## Integration (watermelon-wine-os)

> You can keep this repo as-is, e.g. cloned into
> `watermelon-wine-os/layers/` as `meta-imtube`.

**1. Add the layer to the build**

Edit
`build-openstlinuxweston-stm32mp25-mx-watermelon-wine-1a/conf/bblayers.conf`:

```bitbake
# ImTube Qt6 YouTube client layer
BBLAYERS =+ "/home/mint/YOUR-PATH/meta-imtube"
```

**2. Add the packagegroup to the image**

Edit `conf/local.conf`:

```bitbake
CORE_IMAGE_EXTRA_INSTALL:append = " packagegroup-imtube"
```

**3. Build and flash**

```shell
bitbake st-image-weston
```

The image now contains `imtube-qt`, `yt-dlp` and `ffmpeg`.
On the target, run:

```shell
imtube-qt
```

---

## Build details

### imtube-qt (`recipes-multimedia/imtube-qt`)

- Fetches from `git://github.com/DanielMartensson/meta-imtube.git` (this
  repo), source lives in `src/`.
- Inherits `qt6-cmake` from meta-qt6.
- `RDEPENDS`: `yt-dlp`, `ffmpeg`, the `gstreamer1.0-plugins-good-qml6`
  sink package, `qtdeclarative`, `qtwayland`.
- `SRCREV` is `${AUTOREV}` during development – replace it with the exact
  commit SHA for reproducible image builds.

### gst-plugins-good bbappend

Adds the `qt6` PACKAGECONFIG flag:

```
-Dqt6=enabled -Dqt-egl=enabled -Dqt-wayland=enabled -Dqt-x11=disabled
```

…plus the Qt build/runtime dependencies (`qtbase`, `qtdeclarative`,
`qtbase-native`, `qtshadertools-native`, `qtwayland`).

### yt-dlp (`recipes-devtools/python/yt-dlp`)

The `yt-dlp_linux_aarch64` release asset is a self-contained
PyInstaller binary – **no Python runtime on the target**. The recipe
installs it as `${bindir}/yt-dlp` with `ffmpeg` as the only runtime
dependency. Source checksums are pinned in the recipe.

> This recipe is aarch64-specific. For an x86_64 host build, use the
> `yt-dlp_linux` asset instead (same recipe, different `SRC_URI`).

---

## Configuration (at runtime)

Everything is reachable from the **Settings** page in the app:

| Setting            | Values                                            |
|--------------------|---------------------------------------------------|
| Max video height   | 144 / 240 / 360 / 480 / 720 / 1080                |
| Decoder            | `auto` / `v4l2slh264dec` / `v4l2h264dec` / custom |
| Search results     | 5–50                                              |
| Theme              | Dark / light                                      |

Environment override for testing:

```shell
IMTUBE_VIDEO_SINK=fakesink ./imtube-qt
```

Settings are stored in `~/.config/WatermelonWine/imtube-qt.conf`.

---

## Desktop testing (host)

Requires Qt ≥ 6.5 (Quick, QuickControls2, Network) and the GStreamer
`-dev` packages (`gstreamer-1.0`, `gstreamer-app-1.0`):

```shell
# 1. install a local yt-dlp without root
./tools/install_yt-dlp.sh

# 2. build
cmake -S src -B build
cmake --build build

# 3. run
QT_QPA_PLATFORM=wayland ./build/imtube-qt   # or xcb
```

---

## Troubleshooting

- **`yt-dlp` not found / could not be started**
  Check that `yt-dlp` is on `PATH` on the target (`which yt-dlp`), or set an
  absolute path in Settings → yt-dlp.

- **"QML video sink not available"**
  The `qt6` PACKAGECONFIG was not enabled in `gst-plugins-good`, or the
  `-qml6` runtime package is missing. Verify with:
  `gst-inspect-1.0 qml6glsink`.

- **Forced hardware decoder fails**
  The selected decoder is not present on the target. GStreamer falls back to
  `decodebin` automatically; set the decoder to `auto` if videos refuse to
  play.

- **Audio is missing**
  `autoaudiosink` needs the ALSA/Pulse plugins of `gst-plugins-good/base`;
  make sure the base image pulls them (they are RDEPENDS of this layer).

---

## License

MIT – see [`LICENSE`](LICENSE).