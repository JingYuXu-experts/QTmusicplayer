import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "#000000"
    Component.onCompleted: musicMgr.refreshLocalMusic()

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 200
            color: "#121212"
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 15
                Text { text: "My Music Library"; color: "#666"; font.bold: true }
                Rectangle {
                    Layout.fillWidth: true
                    height: 40
                    color: "#222"
                    radius: 6
                    Row {
                        anchors.centerIn: parent
                        spacing: 10
                        Text { text: "*"; color: "#FF3a3a" }
                        Text { text: "Local Music"; color: "white"; font.bold: true }
                    }
                }
                Item { Layout.fillHeight: true }
            }
        }

        Rectangle {
            Layout.fillHeight: true
            Layout.fillWidth: true
            color: "black"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 40
                spacing: 20
                Text { text: "Local Music"; color: "white"; font.pixelSize: 32; font.bold: true }

                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "#"; color: "#666"; width: 30 }
                    Text { text: "Title"; color: "#666"; Layout.preferredWidth: 250 }
                    Text { text: "Album"; color: "#666"; Layout.preferredWidth: 200 }
                    Text { text: "Duration"; color: "#666"; Layout.preferredWidth: 70 }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: "#222" }

                ListView {
                    id: songList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: musicMgr.localMusic

                    delegate: Rectangle {
                        width: songList.width
                        height: 60
                        color: area.containsMouse ? "#1A1A1A" : "transparent"
                        radius: 6

                        MouseArea {
                            id: area
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: musicMgr.playTrack(modelData.title, modelData.artist, modelData.cover, modelData.url)
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            spacing: 15
                            Text { text: index + 1; color: "#666"; width: 30 }

                            RowLayout {
                                Layout.preferredWidth: 250
                                spacing: 15

                                Image {
                                    source: modelData.cover
                                    Layout.preferredWidth: 40
                                    Layout.preferredHeight: 40
                                    fillMode: Image.PreserveAspectCrop
                                    Rectangle { anchors.fill: parent; color: "#333"; visible: parent.status !== Image.Ready; z: -1 }
                                }

                                Column {
                                    Text { text: modelData.title; color: "white"; font.bold: true; elide: Text.ElideRight; width: 190 }
                                    Text {
                                        text: modelData.artist
                                        color: "#AAA"
                                        font.pixelSize: 12
                                        elide: Text.ElideRight
                                        width: 190
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

                            Text { text: modelData.album; color: "#AAA"; Layout.preferredWidth: 200; elide: Text.ElideRight }
                            Text { text: modelData.duration || "--:--"; color: "#AAA"; Layout.preferredWidth: 70 }
                        }
                    }
                }
            }
        }
    }
}
