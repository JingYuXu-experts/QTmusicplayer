import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "#000000"

    Image {
        anchors.fill: parent
        source: musicMgr.currentCover
        fillMode: Image.PreserveAspectCrop
        opacity: 0.3
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#000000" }
            GradientStop { position: 0.5; color: "#80000000" }
            GradientStop { position: 1.0; color: "#000000" }
        }
    }

    Text {
        anchors.centerIn: parent
        text: musicMgr.lyricsModel.length === 0 ? "No lyrics" : ""
        color: "#666"
        font.pixelSize: 24
        visible: musicMgr.lyricsModel.length === 0
    }

    ListView {
        id: lyricsList
        anchors.fill: parent
        anchors.margins: 40
        anchors.topMargin: 80
        anchors.bottomMargin: 120

        model: musicMgr.lyricsModel
        clip: true
        spacing: 30

        highlightRangeMode: ListView.StrictlyEnforceRange
        preferredHighlightBegin: height / 2 - 20
        preferredHighlightEnd: height / 2 + 20
        currentIndex: musicMgr.currentLyricsIndex

        delegate: Text {
            width: ListView.view.width
            text: modelData.text
            color: ListView.isCurrentItem ? "white" : "#888"
            font.pixelSize: ListView.isCurrentItem ? 32 : 24
            font.bold: ListView.isCurrentItem
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap

            Behavior on color { ColorAnimation { duration: 200 } }
            Behavior on font.pixelSize { NumberAnimation { duration: 200 } }

            MouseArea { anchors.fill: parent; onClicked: musicMgr.seek(modelData.time) }
        }
    }
}
