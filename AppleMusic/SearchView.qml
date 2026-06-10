import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: searchRoot
    color: "#000000"

    property string searchText: ""

    onSearchTextChanged: musicMgr.search(searchText === "" ? "all" : searchText)

    Component.onCompleted: {
        if (searchText === "") {
            musicMgr.search("all")
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 40
        spacing: 20

        Text {
            text: searchText ? "Search results: \"" + searchText + "\"" : "Explore Music"
            color: "white"
            font.pixelSize: 32
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Text { text: "#"; color: "#666"; width: 30; font.pixelSize: 13 }
            Text { text: "Title"; color: "#666"; Layout.preferredWidth: 250; font.pixelSize: 13 }
            Text { text: "Artist"; color: "#666"; Layout.preferredWidth: 150; font.pixelSize: 13 }
            Text { text: "Album"; color: "#666"; Layout.preferredWidth: 150; font.pixelSize: 13 }
            Text { text: "Duration"; color: "#666"; Layout.preferredWidth: 70; font.pixelSize: 13 }
            Item { Layout.fillWidth: true }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#222" }

        ListView {
            id: searchList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: musicMgr.searchResults

            delegate: Rectangle {
                width: searchList.width
                height: 60
                color: area.containsMouse ? "#1A1A1A" : "transparent"
                radius: 6

                MouseArea {
                    id: area
                    anchors.fill: parent
                    hoverEnabled: true
                    onDoubleClicked: musicMgr.playTrack(modelData.title || "", modelData.artist || "", modelData.cover || modelData.image || "", modelData.url || "")
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 10

                    Text { text: index + 1; color: "#666"; width: 30 }

                    RowLayout {
                        Layout.preferredWidth: 250
                        spacing: 15

                        Image {
                            Layout.preferredWidth: 40
                            Layout.preferredHeight: 40
                            source: modelData.cover || modelData.image || ""
                            fillMode: Image.PreserveAspectCrop

                            Rectangle { anchors.fill: parent; color: "#333"; visible: parent.status !== Image.Ready; z: -1 }
                            Rectangle { anchors.fill: parent; color: "transparent"; border.color: "#333"; border.width: 1 }
                        }

                        Text {
                            text: modelData.title || "Unknown title"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 14
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    Text {
                        text: modelData.artist || "Unknown artist"
                        color: "#AAA"
                        font.pixelSize: 13
                        elide: Text.ElideRight
                        Layout.preferredWidth: 150
                    }

                    Text {
                        text: modelData.album || "Single"
                        color: "#AAA"
                        font.pixelSize: 13
                        elide: Text.ElideRight
                        Layout.preferredWidth: 150
                    }

                    Text {
                        text: modelData.duration || "--:--"
                        color: "#AAA"
                        font.pixelSize: 13
                        Layout.preferredWidth: 70
                        horizontalAlignment: Text.AlignRight
                    }

                    Item {
                        Layout.fillWidth: true
                        Text {
                            text: "Play"
                            color: "white"
                            font.pixelSize: 14
                            visible: area.containsMouse
                            MouseArea {
                                anchors.fill: parent
                                onClicked: musicMgr.playTrack(modelData.title || "", modelData.artist || "", modelData.cover || modelData.image || "", modelData.url || "")
                            }
                        }
                    }
                }
            }
        }
    }
}
