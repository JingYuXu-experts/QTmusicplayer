import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#212121"
    height: 80

    function formatTime(ms) {
        if (ms <= 0) return "0:00"
        var totalSec = Math.floor(ms / 1000)
        var m = Math.floor(totalSec / 60)
        var s = totalSec % 60
        return m + ":" + (s < 10 ? "0" + s : s)
    }

    Drawer {
        id: playlistDrawer
        width: 300
        height: window.height - 80
        edge: Qt.RightEdge
        y: -window.height + 80
        modal: true
        background: Rectangle { color: "#161616" }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            Text { text: "Queue"; color: "white"; font.bold: true; font.pixelSize: 18 }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: musicMgr.playlist
                spacing: 10

                delegate: RowLayout {
                    width: parent.width
                    spacing: 10

                    Text { text: ">"; color: "#FF3a3a"; visible: modelData.title === musicMgr.currentTitle }

                    Column {
                        Layout.fillWidth: true
                        Text { text: modelData.title; color: modelData.title === musicMgr.currentTitle ? "#FF3a3a" : "white"; font.bold: true }
                        Text { text: modelData.artist; color: "#666"; font.pixelSize: 12 }
                    }

                    Text {
                        text: "Play"
                        color: "white"
                        font.pixelSize: 14
                        MouseArea { anchors.fill: parent; onClicked: musicMgr.playFromQueue(index) }
                    }
                    Text {
                        text: "X"
                        color: "#666"
                        font.pixelSize: 14
                        MouseArea { anchors.fill: parent; onClicked: musicMgr.removeFromQueue(index) }
                    }
                }
            }
        }
    }

    Slider {
        id: progressBar
        width: parent.width
        height: 12
        anchors.top: parent.top
        anchors.topMargin: -6
        z: 100
        from: 0
        to: musicMgr.duration > 0 ? musicMgr.duration : 1
        value: musicMgr.position
        onMoved: musicMgr.seek(value)
        background: Rectangle {
            y: 6
            height: 2
            width: parent.width
            color: "#4d4d4d"
            Rectangle { width: progressBar.visualPosition * parent.width; height: 2; color: "#FF0000" }
        }
        handle: Item {}
    }

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.verticalCenter: parent.verticalCenter
        spacing: 20

        Text {
            text: musicMgr.playbackMode === 0 ? "Loop" : (musicMgr.playbackMode === 1 ? "One" : "Mix")
            color: "#CCC"
            font.pixelSize: 14
            MouseArea { anchors.fill: parent; onClicked: musicMgr.switchPlaybackMode() }
        }

        Text {
            text: "<"
            color: "#E0E0E0"
            font.pixelSize: 24
            anchors.verticalCenter: parent.verticalCenter
            MouseArea { anchors.fill: parent; onClicked: musicMgr.previous() }
        }

        Rectangle {
            width: 40
            height: 40
            radius: 20
            color: "white"
            anchors.verticalCenter: parent.verticalCenter
            Text {
                text: musicMgr.isPlaying ? "||" : ">"
                color: "black"
                font.pixelSize: 22
                anchors.centerIn: parent
                anchors.horizontalCenterOffset: musicMgr.isPlaying ? 0 : 2
            }
            MouseArea { anchors.fill: parent; onClicked: musicMgr.togglePlayPause() }
        }

        Text {
            text: ">"
            color: "#E0E0E0"
            font.pixelSize: 24
            anchors.verticalCenter: parent.verticalCenter
            MouseArea { anchors.fill: parent; onClicked: musicMgr.next() }
        }

        Text {
            text: formatTime(musicMgr.position) + " / " + formatTime(musicMgr.duration)
            color: "#AAAAAA"
            font.pixelSize: 12
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    Row {
        anchors.centerIn: parent
        spacing: 15
        width: 400
        Rectangle {
            width: 56
            height: 56
            color: "#333"
            anchors.verticalCenter: parent.verticalCenter
            Image {
                anchors.fill: parent
                source: musicMgr.currentCover
                fillMode: Image.PreserveAspectCrop
                sourceSize: Qt.size(56, 56)
                visible: source != ""
            }
        }
        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 4
            width: 300
            Text { text: musicMgr.currentTitle; color: "white"; font.bold: true; font.pixelSize: 16; elide: Text.ElideRight; width: parent.width }
            Text { text: musicMgr.currentArtist; color: "#AAAAAA"; font.pixelSize: 14; elide: Text.ElideRight; width: parent.width }
        }
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: 20
        anchors.verticalCenter: parent.verticalCenter
        spacing: 20

        Text {
            text: "MV"
            color: "#AAA"
            font.bold: true
            font.pixelSize: 12
            anchors.verticalCenter: parent.verticalCenter
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (musicMgr.getMvPath(musicMgr.currentTitle) !== "") {
                        musicMgr.stopPlayback()
                        window.currentViewIndex = 4
                    }
                }
            }
        }

        Text {
            text: "Lyric"
            color: window.currentViewIndex === 3 ? "white" : "#AAA"
            font.pixelSize: 14
            font.bold: true
            anchors.verticalCenter: parent.verticalCenter
            MouseArea { anchors.fill: parent; onClicked: window.currentViewIndex === 3 ? window.currentViewIndex = 0 : window.currentViewIndex = 3 }
        }

        Text {
            text: "List"
            color: playlistDrawer.opened ? "white" : "#AAA"
            font.pixelSize: 14
            anchors.verticalCenter: parent.verticalCenter
            MouseArea { anchors.fill: parent; onClicked: playlistDrawer.opened ? playlistDrawer.close() : playlistDrawer.open() }
        }

        Text { text: "Vol"; color: "#AAA"; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
        Slider {
            from: 0
            to: 100
            value: pressed ? value : musicMgr.volume * 100
            onMoved: musicMgr.volume = value / 100
            width: 80
            height: 10
            anchors.verticalCenter: parent.verticalCenter
            background: Rectangle {
                height: 4
                radius: 2
                color: "#555"
                anchors.verticalCenter: parent.verticalCenter
                Rectangle { width: parent.parent.visualPosition * parent.width; height: 4; radius: 2; color: "white" }
            }
            handle: Item {}
        }
    }
}
