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

    Popup {
        id: audioEffectPopup
        x: Math.min(root.width - width - 20, Math.max(20, audioEffectButton.mapToItem(root, 0, 0).x))
        y: -height - 10
        width: 310
        height: 286
        padding: 0
        modal: false
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: "#181818"
            radius: 12
            border.color: "#343434"
            border.width: 1
        }

        contentItem: Column {
            padding: 12
            spacing: 4

            Text {
                text: "选择音效"
                color: "white"
                font.pixelSize: 16
                font.bold: true
                leftPadding: 8
                bottomPadding: 6
            }

            Repeater {
                model: [
                    { mode: 0, name: "标准", detail: "保留歌曲原始声音" },
                    { mode: 1, name: "演唱会", detail: "增强空间感、声场与混响" },
                    { mode: 2, name: "纯净人声", detail: "突出立体声中置人声" },
                    { mode: 3, name: "纯伴奏", detail: "消除中置声部，弱化主唱" }
                ]

                delegate: Rectangle {
                    width: parent.width - 24
                    height: 52
                    radius: 8
                    color: musicMgr.audioEffect === modelData.mode ? "#3A2024" : effectMouse.containsMouse ? "#282828" : "transparent"

                    Column {
                        anchors.left: parent.left
                        anchors.leftMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 3

                        Text {
                            text: modelData.name
                            color: musicMgr.audioEffect === modelData.mode ? "#FF6B73" : "white"
                            font.pixelSize: 14
                            font.bold: musicMgr.audioEffect === modelData.mode
                        }
                        Text {
                            text: modelData.detail
                            color: "#8E8E8E"
                            font.pixelSize: 11
                        }
                    }

                    Text {
                        anchors.right: parent.right
                        anchors.rightMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        text: "✓"
                        color: "#FF5964"
                        font.pixelSize: 16
                        visible: musicMgr.audioEffect === modelData.mode
                    }

                    MouseArea {
                        id: effectMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            musicMgr.audioEffect = modelData.mode
                            audioEffectPopup.close()
                        }
                    }
                }
            }
        }
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

        Rectangle {
            id: audioEffectButton
            width: 106
            height: 32
            radius: 16
            color: musicMgr.audioEffect === 0 ? "#303030" : "#51252A"
            border.color: musicMgr.audioEffect === 0 ? "#454545" : "#A8424B"
            anchors.verticalCenter: parent.verticalCenter

            Text {
                anchors.centerIn: parent
                text: "音效: " + musicMgr.audioEffectName
                color: musicMgr.audioEffect === 0 ? "#D0D0D0" : "#FF8990"
                font.pixelSize: 12
                font.bold: musicMgr.audioEffect !== 0
            }

            MouseArea {
                anchors.fill: parent
                onClicked: audioEffectPopup.opened ? audioEffectPopup.close() : audioEffectPopup.open()
            }
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
            id: volumeSlider
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
                Rectangle { width: volumeSlider.visualPosition * parent.width; height: 4; radius: 2; color: "white" }
            }
            handle: Item {}
        }
    }
}
