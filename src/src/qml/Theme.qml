import QtQuick
import QtQuick.Controls

// Central color palette. Two variants follow settings.darkTheme; every page
// instantiates `Theme {}` so colors always stay in sync.
QtObject {
    id: root

    property color bg
    property color surface
    property color surfaceHi
    property color accent
    property color text
    property color dimText
    property color border
    property color danger

    function refresh()
    {
        if (settings.darkTheme) {
            root.bg = "#101418"
            root.surface = "#1a2128"
            root.surfaceHi = "#232c36"
            root.accent = "#ff5e3a"
            root.text = "#eef2f6"
            root.dimText = "#9aa7b4"
            root.border = "#2d3742"
            root.danger = "#ff5e5e"
        } else {
            root.bg = "#f4f6f8"
            root.surface = "#ffffff"
            root.surfaceHi = "#e9edf1"
            root.accent = "#e04e28"
            root.text = "#182028"
            root.dimText = "#5c6b78"
            root.border = "#d3dbe2"
            root.danger = "#d52b2b"
        }
    }

    Component.onCompleted: refresh()
    Connections {
        target: settings
        function onDarkThemeChanged() { root.refresh() }
    }
}