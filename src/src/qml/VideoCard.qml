import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Grid card shown in the search/library lists. The `model` context object of
// the surrounding delegate provides the videoId/title/uploader/... roles.
Item {
    id: root

    property QtObject theme: Theme {}
    property bool favorite: false
    property string localThumb: ""

    Rectangle {
        anchors.fill: parent
        anchors.margins: 6
        color: theme.surface
        border.color: theme.border
        border.width: 1
        radius: 8
    }

    // Background placeholder drawn behind the cover image.
    Rectangle {
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            topMargin: 10
            leftMargin: 10
            rightMargin: 10
        }
        height: parent.height * 0.60
        radius: 4
        color: theme.surfaceHi
    }

    Image {
        id: cover
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            topMargin: 10
            leftMargin: 10
            rightMargin: 10
        }
        height: parent.height * 0.60
        fillMode: Image.PreserveAspectCrop
        clip: true
        sourceSize.height: 360
        source: root.localThumb ? "file://" + root.localThumb
                                : (model.thumbRemote ? model.thumbRemote : "")

        // thumbnail placeholder
        Label {
            anchors.centerIn: parent
            text: qsTr("No thumbnail")
            color: theme.dimText
            font.pixelSize: 12
            visible: cover.status !== Image.Ready
        }

        // duration badge
        Rectangle {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 6
            radius: 3
            color: "#c0000000"
            padding: 4
            Label {
                text: model.durationText
                color: "white"
                font.pixelSize: 11
            }
        }
    }

    ColumnLayout {
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            leftMargin: 14
            rightMargin: 48
            bottomMargin: 12
        }
        spacing: 2

        Label {
            text: model.title
            color: theme.text
            font.pixelSize: 14
            font.bold: true
            elide: Text.ElideRight
            maximumLineCount: 1
            Layout.fillWidth: true
        }
        Label {
            text: model.uploader
            color: theme.dimText
            font.pixelSize: 12
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }

    // Open the video (covers the whole card; the favorite button sits above).
    MouseArea {
        anchors.fill: parent
        z: 1
        onClicked: player.playVideo(model.videoId, model.title)
    }

    // Favorite toggle (top-right, above the click area).
    MouseArea {
        anchors {
            top: parent.top
            right: parent.right
            topMargin: 12
            rightMargin: 12
        }
        width: 30
        height: 30
        z: 2
        onClicked: {
            library.toggleFavorite(model.videoId, model.title,
                                   model.uploader, model.durationSec,
                                   model.thumbRemote)
        }
        Rectangle {
            anchors.fill: parent
            radius: 15
            color: root.favorite ? theme.accent : "#c0000000"
            Label {
                anchors.centerIn: parent
                text: root.favorite ? "\u2665" : "\u2661"
                color: "white"
                font.pixelSize: 17
            }
        }
    }

    Connections {
        target: thumbs
        function onReady(videoId, path) {
            if (videoId === model.videoId)
                root.localThumb = path
        }
    }

    Connections {
        target: library
        function onFavoritesChanged() {
            root.favorite = library.isFavorite(model.videoId)
        }
    }

    Component.onCompleted: {
        if (model.thumbRemote)
            thumbs.request(model.videoId, model.thumbRemote)
        root.favorite = library.isFavorite(model.videoId)
    }
}