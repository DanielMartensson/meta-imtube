import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

Item {
    id: root

    property QtObject theme: Theme {}

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Label {
                text: qsTr("My Library")
                color: root.theme.text
                font.pixelSize: 20
                font.bold: true
            }
            Label {
                text: qsTr("%n video(s)", "", library.count())
                color: root.theme.dimText
                font.pixelSize: 13
            }
            Item { Layout.fillWidth: true }
            Button {
                text: qsTr("Clear library")
                visible: library.count() > 0
                onClicked: {
                    const items = library.favorites()
                    for (const v of items)
                        library.removeFavorite(v.videoId)
                }
            }
        }

        Label {
            text: qsTr("No favorites yet – tap the heart on a video.")
            color: root.theme.dimText
            font.pixelSize: 14
            visible: libraryModel.count === 0
        }

        GridView {
            id: grid
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: libraryModel
            cellWidth: Math.max(240, Math.floor((width - 12) / 2))
            cellHeight: cellWidth * 9.0 / 16.0 + 84
            delegate: VideoCard {
                width: grid.cellWidth
                height: grid.cellHeight
            }
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }
        }
    }
}