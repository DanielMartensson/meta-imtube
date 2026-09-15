import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import org.freedesktop.gstreamer.Qt6GLVideoItem 1.0
import ImTube 1.0

// Full-screen video surface. The GStreamer qml6 sink renders directly into
// the GstGLQt6VideoItem via the GPU (QSG), no CPU frame copying happens.
Item {
    id: root

    property QtObject theme: Theme {}

    Rectangle {
        anchors.fill: parent
        color: "black"
    }

    GstGLQt6VideoItem {
        id: videoSurface
        anchors.fill: parent

        // The sink receives the QQuickItem* and renders into it.
        Component.onCompleted: player.setVideoItem(videoSurface)
    }

    // top gradient for readability of the title
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 110
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#c8000000" }
            GradientStop { position: 1.0; color: "transparent" }
        }
        ColumnLayout {
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
                margins: 14
            }
            Label {
                text: player.currentTitle
                color: "white"
                font.pixelSize: 18
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Label {
                text: player.videoId !== "" ? qsTr("Now playing · HW accelerated") : ""
                color: "#ddffffff"
                font.pixelSize: 12
            }
        }
    }

    // buffering spinner
    BusyIndicator {
        anchors.centerIn: parent
        width: 64
        height: 64
        running: player.state === PlaybackControl.Buffering
        visible: running
    }

    // paused overlay
    Rectangle {
        anchors.centerIn: parent
        width: 96
        height: 96
        radius: 48
        color: "#b0000000"
        visible: player.state === PlaybackControl.Paused
        Label {
            anchors.centerIn: parent
            text: "\u25B6"
            color: "white"
            font.pixelSize: 40
        }
        MouseArea {
            anchors.fill: parent
            onClicked: player.togglePause()
        }
    }

    // error banner
    Rectangle {
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            topMargin: 120
        }
        height: 48
        color: "#e0ff5e5e"
        visible: player.state === PlaybackControl.Error
        Label {
            anchors.fill: parent
            anchors.margins: 8
            verticalAlignment: Text.AlignVCenter
            text: player.errorMessage
            color: "white"
            font.pixelSize: 13
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
        }
    }

    TransportBar {
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }

    // idle hint
    Label {
        anchors.centerIn: parent
        visible: player.state === PlaybackControl.Idle
        text: qsTr("Pick a video from Search or Library")
        color: "#aa9aa7b4"
        font.pixelSize: 16
    }
}