#!/usr/bin/env sh
# Install a standalone (PyInstaller) yt-dlp into ~/.local/bin without root.
# Used on a desktop host for quick testing of the client.

set -e

VERSION="2026.08.19"
DEST="${HOME}/.local/bin"
URL="https://github.com/yt-dlp/yt-dlp/releases/download/${VERSION}"

case "$(uname -m)" in
    x86_64) ASSET="yt-dlp_linux" ;;
    aarch64|arm64) ASSET="yt-dlp_linux_aarch64" ;;
    *)
        echo "Unsupported architecture: $(uname -m)"
        exit 1
        ;;
esac

mkdir -p "${DEST}"

if [ -x "${DEST}/yt-dlp" ]; then
    echo "yt-dlp already installed: ${DEST}/yt-dlp"
    "${DEST}/yt-dlp" --version || true
    exit 0
fi

echo "Downloading yt-dlp ${VERSION} (${ASSET}) ..."
curl -fL -o "${DEST}/yt-dlp" "${URL}/${ASSET}"
chmod +x "${DEST}/yt-dlp"
echo "Installed to ${DEST}/yt-dlp"
"${DEST}/yt-dlp" --version