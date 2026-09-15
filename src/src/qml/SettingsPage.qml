import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

Item {
    id: root

    property QtObject theme: Theme {}

    Flickable {
        anchors.fill: parent
        anchors.margins: 16
        contentWidth: availableWidth
        contentHeight: layout.implicitHeight
        clip: true
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        ColumnLayout {
            id: layout
            width: parent.width
            spacing: 14

            Label {
                text: qsTr("Settings")
                color: root.theme.text
                font.pixelSize: 20
                font.bold: true
            }

            Rectangle { width: parent.width; height: 1; color: root.theme.border }

            GridLayout {
                columns: 2
                columnSpacing: 16
                rowSpacing: 10
                Layout.fillWidth: true

                Label { text: qsTr("Maximum video height"); color: root.theme.text }
                ComboBox {
                    model: [144, 240, 360, 480, 720, 1080]
                    currentIndex: findSetting(settings.resolution)
                    onActivated: settings.setResolution(parseInt(currentText, 10))
                    function findSetting(v) {
                        for (let i = 0; i < model.length; i++)
                            if (model[i] === v) return i
                        return 2
                    }
                }

                Label { text: qsTr("Video decoder"); color: root.theme.text }
                ComboBox {
                    editable: true
                    model: ["auto"].concat(settings.preferredDecoderNames())
                    currentText: settings.decoder
                    onActivated: settings.setDecoder(currentText.trim())
                    onEditingFinished: settings.setDecoder(currentText.trim())
                }

                Label { text: qsTr("Search results"); color: root.theme.text }
                SpinBox {
                    from: 5
                    to: 50
                    stepSize: 5
                    value: settings.maxResults
                    onValueModified: settings.setMaxResults(value)
                }

                Label { text: qsTr("Dark theme"); color: root.theme.text }
                Switch {
                    checked: settings.darkTheme
                    onToggled: settings.setDarkTheme(checked)
                    contentItem: Label { text: parent.checked ? qsTr("On") : qsTr("Off") }
                }
            }

            Rectangle { width: parent.width; height: 1; color: root.theme.border }

            Label { text: qsTr("Graphics / video path"); color: root.theme.text; font.bold: true }
            Label {
                text: settings.rendererDescription
                color: root.theme.dimText
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Label { text: qsTr("yt-dlp"); color: root.theme.text; font.bold: true }
            Label {
                text: settings.ytDlpPath
                color: root.theme.dimText
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Label {
                text: ytDlp.version() !== "" ? qsTr("Version: %1").arg(ytDlp.version())
                                             : qsTr("No yt-dlp found – see install_yt-dlp.sh")
                color: root.theme.dimText
            }

            Label {
                text: qsTr("Hardware acceleration note: video is decoded by the "
                          + "platform VPU/hardware decoder and rendered by the "
                          + "GPU inside the QML scene graph. No software "
                          + "decoding is used.")
                color: root.theme.dimText
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.topMargin: 8
            }
        }
    }
}