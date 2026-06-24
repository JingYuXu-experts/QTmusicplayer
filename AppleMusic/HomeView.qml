import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ScrollView {
    contentWidth: availableWidth
    clip: true

    Component.onCompleted: musicMgr.refreshLocalMusic()

    ColumnLayout {
        width: parent.width
        spacing: 30
        anchors.margins: 40

        RowLayout {
            Layout.topMargin: 20
            Rectangle {
                width: 44
                height: 44
                radius: 22
                color: "#5d4037"
                Text { text: "J"; color: "white"; anchors.centerIn: parent; font.bold: true }
            }
            Column {
                Text { text: "Jingyu"; color: "white"; font.pixelSize: 22; font.bold: true }
                Text { text: "Good evening"; color: "#AAAAAA"; font.pixelSize: 14 }
            }
        }

        Row {
            spacing: 12
            Repeater {
                model: ["Chill", "Sleep", "Workout", "Commute", "Focus"]
                Rectangle {
                    width: 80
                    height: 32
                    radius: 16
                    color: "transparent"
                    border.color: "#555"
                    border.width: 1
                    Text { text: modelData; color: "white"; anchors.centerIn: parent; font.pixelSize: 13 }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: parent.color = "#333"
                        onExited: parent.color = "transparent"
                    }
                }
            }
        }

        Text { text: "Listen Again"; color: "white"; font.pixelSize: 24; font.bold: true }

        GridLayout {
            Layout.fillWidth: true
            columns: Math.max(1, Math.floor(width / 230))
            columnSpacing: 24
            rowSpacing: 32

            Repeater {
                model: musicMgr.localMusic

                ColumnLayout {
                    spacing: 12
                    Layout.preferredWidth: 200

                    Item {
                        Layout.preferredWidth: 200
                        Layout.preferredHeight: 200

                        Rectangle {
                            anchors.fill: parent
                            color: "#333"
                        }

                        Image {
                            anchors.fill: parent
                            source: modelData.cover
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                        }

                        Rectangle {
                            id: hoverOverlay
                            anchors.fill: parent
                            color: "#80000000"
                            opacity: 0
                            Behavior on opacity { NumberAnimation { duration: 200 } }
                            Text { text: "Play"; color: "white"; font.pixelSize: 28; anchors.centerIn: parent }
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onEntered: hoverOverlay.opacity = 1
                            onExited: hoverOverlay.opacity = 0
                            onClicked: musicMgr.playTrack(modelData.title, modelData.artist, modelData.cover, modelData.url)
                        }
                    }

                    Text {
                        text: modelData.title
                        color: "white"
                        font.bold: true
                        font.pixelSize: 16
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Text {
                        text: modelData.artist
                        color: "#AAAAAA"
                        font.pixelSize: 14
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                window.selectedArtistName = modelData.artist || ""
                                window.currentViewIndex = 7
                            }
                        }
                    }
                }
            }
        }

        Text {
            visible: musicMgr.localMusic.length === 0
            text: "No local music"
            color: "#AAAAAA"
            font.pixelSize: 16
        }

        Item { Layout.preferredHeight: 100 }
    }
}
