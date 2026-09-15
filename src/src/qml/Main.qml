import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ImTube 1.0

ApplicationWindow {
    id: root

    visible: true
    width: 1280
    height: 720
    minimumWidth: 960
    minimumHeight: 540
    title: "ImTube – lightweight YouTube client"
    color: root.theme.bg

    readonly property QtObject theme: Theme {}

    header: ToolBar {
        height: 56
        background: Rectangle {
            color: root.theme.surface
            Rectangle {
                width: parent.width
                height: 1
                anchors.bottom: parent.bottom
                color: root.theme.border
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 8

            Label {
                text: "ImTube"
                color: root.theme.accent
                font.pixelSize: 22
                font.bold: true
            }

            Rectangle {
                width: 1
                height: 28
                color: root.theme.border
                Layout.leftMargin: 8
                Layout.rightMargin: 8
            }

            ToolButton {
                text: qsTr("Search")
                checked: nav.currentIndex === 0
                onClicked: nav.currentIndex = 0
                font.pixelSize: 15
                padding: 12
                contentItem: Label {
                    text: parent.text
                    color: parent.checked ? root.theme.accent : root.theme.text
                    font.pixelSize: 15
                    font.bold: parent.checked
                }
                background: Rectangle {
                    color: parent.hovered ? root.theme.surfaceHi : "transparent"
                    radius: 6
                }
            }
            ToolButton {
                text: qsTr("Library")
                checked: nav.currentIndex === 1
                onClicked: nav.currentIndex = 1
                contentItem: Label {
                    text: parent.text
                    color: parent.checked ? root.theme.accent : root.theme.text
                    font.pixelSize: 15
                    font.bold: parent.checked
                }
                background: Rectangle {
                    color: parent.hovered ? root.theme.surfaceHi : "transparent"
                    radius: 6
                }
            }
            ToolButton {
                text: qsTr("Settings")
                checked: nav.currentIndex === 2
                onClicked: nav.currentIndex = 2
                contentItem: Label {
                    text: parent.text
                    color: parent.checked ? root.theme.accent : root.theme.text
                    font.pixelSize: 15
                    font.bold: parent.checked
                }
                background: Rectangle {
                    color: parent.hovered ? root.theme.surfaceHi : "transparent"
                    radius: 6
                }
            }

            Item { Layout.fillWidth: true }

            Label {
                text: settings.rendererDescription
                color: root.theme.dimText
                font.pixelSize: 12
            }

            ToolButton {
                text: player.hasVideo ? qsTr("▶ Player") : qsTr("Player")
                checked: nav.currentIndex === 3
                visible: player.hasVideo
                onClicked: nav.currentIndex = 3
                contentItem: Label {
                    text: parent.text
                    color: parent.checked ? root.theme.accent : root.theme.text
                    font.pixelSize: 15
                    font.bold: parent.checked
                }
                background: Rectangle {
                    color: parent.hovered ? root.theme.surfaceHi : root.theme.accent
                    radius: 6
                    visible: player.hasVideo
                }
            }
        }
    }

    // ------------------------------------------------------------------
    StackLayout {
        id: nav
        anchors.fill: parent
        currentIndex: 0

        SearchPage {
            theme: root.theme
            width: nav.width
            height: nav.height
        }
        LibraryPage {
            theme: root.theme
            width: nav.width
            height: nav.height
        }
        SettingsPage {
            theme: root.theme
            width: nav.width
            height: nav.height
        }
        PlayerPage {
            theme: root.theme
            width: nav.width
            height: nav.height
        }
    }

    // Pass data-agnostic helper for pages: short alias of player.playVideo.
    function play(videoId, title)
    {
        player.playVideo(videoId, title)
    }
}