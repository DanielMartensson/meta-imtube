import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

Item {
    id: root

    property QtObject theme: Theme {}
    property bool searching: false
    property string searchError: ""

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            spacing: 8

            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Search YouTube…")
                font.pixelSize: 16
                color: root.theme.text
                onAccepted: doSearch()
                background: Rectangle {
                    radius: 8
                    color: root.theme.surface
                    border.color: searchField.activeFocus ? root.theme.accent : root.theme.border
                    border.width: 1
                }
            }

            Button {
                text: qsTr("Search")
                font.pixelSize: 15
                onClicked: doSearch()
                background: Rectangle {
                    radius: 8
                    color: parent.hovered ? Qt.lighter(root.theme.accent, 1.1) : root.theme.accent
                }
                contentItem: Label {
                    text: parent.text
                    color: "white"
                    font.pixelSize: 15
                    font.bold: true
                }
            }
        }

        // status line
        RowLayout {
            visible: root.searching || root.searchError !== ""
            BusyIndicator {
                running: root.searching
                visible: running
                width: 18
                height: 18
            }
            Label {
                text: root.searching ? qsTr("Searching…")
                                     : root.searchError
                color: root.searching ? root.theme.dimText : root.theme.danger
                font.pixelSize: 13
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("No results yet – search above.")
            color: root.theme.dimText
            font.pixelSize: 14
            visible: !root.searching && searchModel.count === 0 && root.searchError === ""
        }

        GridView {
            id: grid
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: searchModel
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

    function doSearch()
    {
        const text = searchField.text.trim()
        if (text.length === 0)
            return
        root.searchError = ""
        root.searching = true
        ytDlp.search(text)
    }

    Connections {
        target: ytDlp
        function onSearchFinished(videos) {
            root.searching = false
            root.searchError = ""
        }
        function onSearchFailed(error) {
            root.searching = false
            root.searchError = error
            if (error && error.length > 120)
                root.searchError = error.substring(0, 120) + "…"
        }
    }
}