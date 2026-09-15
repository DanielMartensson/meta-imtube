# meta-imtube

**ImTube Qt** – a lightweight YouTube client for embedded Linux, written in
**Qt 6 / QML** with **GStreamer hardware acceleration only**.

This repository *is* a standard, self-contained OpenEmbedded/Yocto layer. It
provides the `imtube-qt` client, the `yt-dlp` helper and the GStreamer
QML-sink plumbing needed to render video 100 % on the GPU.

> Inspired by [ImTube](https://github.com/DanielMartensson/ImTube)
> (ImGui + GStreamer + yt-dlp) and FLTube (FLTK + yt-dlp). This is a Qt6 QML
> re-implementation tuned for embedded platforms with a hardware video
> decoder and a GPU, e.g. the STM32MP2x series running Weston.

---

## How it works

Video is **never decoded or copied on the CPU**:

1. `yt-dlp` downloads/remuxes the best fitting stream (`<= maxHeight`,
   h264/AVC1 video) and writes a single MKV to **stdout**.
2. That byte stream is fed into a GStreamer **`appsrc`** element.
3. A demuxer (`decodebin`, or an explicit `matroskademux` when a hardware
   decoder is forced) splits video/audio.
4. Video reaches the decoder **and** is rendered by the GPU inside the QML
   scene graph via `qml6glsink` – no host-side pixel copies.

Seeking restarts the download with `--download-sections "*H:MM:SS-"`.

---

## Features

- **Hardware acceleration only.** The platform hardware decoder is
  auto-plugged (`v4l2slh264dec`/`v4l2h264dec`, ...) or forced in Settings.
  `decodebin` falls back gracefully if a forced decoder is unavailable.
- **Vulkan ready.** The renderer is auto-detected at startup:

  | GStreamer stack                    | Scene graph API  | Sink used          |
  |------------------------------------|------------------|--------------------|
  | has `qml6vulkansink` (≥ 1.28)      | **Vulkan**        | `qml6vulkansink`    |
  | otherwise (1.26.x, this layer)     | OpenGL / OpenGL ES | `qml6glsink`      |

  Switch to a GStreamer build ≥ 1.28 and Vulkan turns on automatically – no
  layer changes needed.
- **YouTube search** (`ytsearchN:`), play, pause, seek, volume/mute.
- **Library** – favorites persisted via `QSettings` (JSON), with local
  thumbnail caching.
- **Dark/light theme**, configurable resolution cap (144–1080p) and decoder
  selection.
- **Standalone `yt-dlp`** – a self-contained aarch64 binary; no Python on
  the target.

---

## Layer layout

```
meta-imtube/
├── conf/
│   └── layer.conf                        # collection "imtube", depends on qt6-layer
├── recipes-devtools/
│   └── python/yt-dlp/
│       └── yt-dlp_2026.08.19.bb          # standalone aarch64 binary
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

- BitBake / OpenEmbedded, Yocto **Scarthgap** (or later)
- **meta-qt6** – Qt 6.8.x recipes (`qt6-layer`). Provides
  `qt6-cmake.bbclass` and the Qt packages used below.
- GStreamer 1.26.x with `gst-plugins-good` and `gst-plugins-base`
- Recommended `DISTRO_FEATURES`: `opengl`, `wayland`, `vulkan`

---

## Integration

**1. Add the layer**

Clone (or symlink) this repository into your layers directory, e.g.:

```shell
git clone https://github.com/DanielMartensson/meta-imtube.git \
    <your-build>/layers/meta-imtube
```

Then add it to `conf/bblayers.conf`:

```bitbake
BBLAYERS =+ "/path/to/layers/meta-imtube"
```

**2. Add the packagegroup to your image**

Edit `conf/local.conf`:

```bitbake
CORE_IMAGE_EXTRA_INSTALL:append = " packagegroup-imtube"
```

**3. Build**

```shell
bitbake <your-image>      # e.g. bitbake core-image-weston
```

**4. Run on the target**

```shell
imtube-qt
```

---

## What the layer builds

### `imtube-qt`

The Qt6/QML client.

- Inherits `qt6-cmake` (from meta-qt6) and `pkgconfig`.
- `SRC_URI`: `git://github.com/DanielMartensson/meta-imtube.git`
  (`branches=main`, source in `src/`); depends on `qtbase`,
  `qtdeclarative`, `gstreamer1.0`, `gstreamer1.0-plugins-base`.
- `RDEPENDS`: `yt-dlp`, `ffmpeg`, `gstreamer1.0-plugins-good-qml6`,
  `qtdeclarative`, `qtwayland`.
- `SRCREV = "${AUTOREV}"` during development – pin the exact commit SHA for
  reproducible image builds.

### `gst-plugins-good` bbappend

Enables the Qt6 QML video sink:

```
-Dqt6=enabled -Dqt-egl=enabled -Dqt-wayland=enabled -Dqt-x11=disabled
```

plus the Qt build/runtime dependencies (`qtbase`, `qtdeclarative`,
`qtbase-native`, `qtshadertools-native`, `qtwayland`).

### `yt-dlp`

The `yt-dlp_linux_aarch64` GitHub release asset is a self-contained
PyInstaller binary – **no Python runtime on the target**. The recipe
installs it as `${bindir}/yt-dlp`; `ffmpeg` is the only runtime dependency.
Source checksums are pinned.

> This recipe is aarch64-specific. For an x86_64 build use the `yt-dlp_linux`
> asset instead (same recipe, different `SRC_URI`).

---

## Configuration (at runtime)

Everything is reachable from the **Settings** page in the app:

| Setting           | Values                                            |
|-------------------|---------------------------------------------------|
| Max video height  | 144 / 240 / 360 / 480 / 720 / 1080                |
| Decoder           | `auto` / `v4l2slh264dec` / `v4l2h264dec` / custom |
| Search results    | 5–50                                              |
| Theme             | Dark / light                                      |

Environment override for testing:

```shell
IMTUBE_VIDEO_SINK=fakesink ./imtube-qt
```

Settings are stored in `~/.config/immerSoftware/imtube-qt.conf`.

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
./build/imtube-qt                    # use QT_QPA_PLATFORM=wayland|xcb if needed
```

---

## Troubleshooting

- **`yt-dlp` not found / could not be started**
  Check `which yt-dlp` on the target, or set an absolute path in
  Settings → yt-dlp.

- **"QML video sink not available"**
  `gst-plugins-good` was built without the `qt6` PACKAGECONFIG, or the
  `-qml6` runtime package is missing. Verify with:
  `gst-inspect-1.0 qml6glsink`.

- **Forced hardware decoder fails**
  The decoder is not present on the target. `decodebin` falls back
  automatically; set the decoder to `auto` if videos refuse to play.

- **Audio is missing**
  `autoaudiosink` needs the ALSA/Pulse plugins of `gst-plugins-good/base`;
  they are RDEPENDS of this layer.

---

## License

MIT – see [`LICENSE`](LICENSE).