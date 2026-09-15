import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ImTube 1.0

// Bottom transport bar attached to the PlayerPage. All bindings go through
// the "player" context property.
Rectangle {
    id: root

    property QtObject theme: Theme {}

    height: 84
    color: "#c8000000"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 4

        Label {
            text: player.currentTitle
            color: "white"
            font.pixelSize: 15
            font.bold: true
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        RowLayout {
            spacing: 6
            Layout.fillWidth: true

            // play / pause
            Button {
                text: (player.state === PlaybackControl.Playing
                       || player.state === PlaybackControl.Buffering)
                      ? "\u275A\u275A" : "\u25B6"
                font.pixelSize: 14
                implicitWidth: 44
                implicitHeight: 36
                onClicked: player.togglePause()
                background: Rectangle {
                    radius: 6
                    color: parent.hovered ? Qt.lighter(root.theme.accent, 1.1)
                                          : root.theme.accent
                }
                contentItem: Label {
                    text: parent.text
                    color: "white"
                    font.pixelSize: 14
                }
            }

            // stop
            Button {
                text: "\u25A0"
                implicitWidth: 44
                implicitHeight: 36
                font.pixelSize: 12
                onClicked: player.stop()
                background: Rectangle {
                    radius: 6
                    color: parent.hovered ? Qt.lighter(root.theme.surfaceHi, 1.1)
                                          : root.theme.surfaceHi
                }
                contentItem: Label {
                    text: parent.text
                    color: root.theme.text
                    font.pixelSize: 12
                }
            }

            // seek
            Slider {
                id: seek
                Layout.fillWidth: true
                from: 0
                to: Math.max(player.duration, 1)
                value: pressed ? dragValue : player.position
                enabled: player.hasVideo

                property real dragValue: 0

                onPressedChanged: {
                    if (pressed)
                        dragValue = player.position
                }
                onMoved: {
                    if (pressed)
                        dragValue = value
                }
                onReleased: {
                    player.seekTo(dragValue)
                }
            }

            Label {
                text: fmt(player.position) + " / " + fmt(player.duration)
                color: root.theme.dimText
                font.pixelSize: 12
            }

            // mute
            Button {
                text: player.muted ? qsTr("Unmute") : qsTr("Mute")
                implicitWidth: 64
                implicitHeight: 36
                font.pixelSize: 12
                onClicked: player.setMuted(!player.muted)
                background: Rectangle {
                    radius: 6
                    color: parent.hovered ? root.theme.surfaceHi : "#aa000000"
                }
                contentItem: Label {
                    text: parent.text
                    color: root.theme.text
                    font.pixelSize: 12
                }
            }

            // volume
            Slider {
                from: 0
                to: 100
                value: player.volume
                implicitWidth: 130
                implicitHeight: 36
                onMoved: player.setVolume(value)
            }
        }
    }

    function fmt(ms)
    {
        const totalSec = Math.floor(ms / 1000)
        const h = Math.floor(totalSec / 3600)
        const m = Math.floor((totalSec % 3600) / 60)
        const s = totalSec % 60
        const mm = m < 10 ? "0" + m : "" + m
        const ss = s < 10 ? "0" + s : "" + s
        return h > 0 ? (h + ":" + mm + ":" + ss) : (mm + ":" + ss)
    }
}