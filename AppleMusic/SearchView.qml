import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: searchRoot
    color: "#000000"

    property string searchText: ""
    property int indexColumnWidth: 34
    property int artistColumnWidth: 170
    property int albumColumnWidth: 220
    property int durationColumnWidth: 76
    property int actionColumnWidth: 62

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

            Item { Layout.preferredWidth: 10 }
            Text { text: "#"; color: "#666"; Layout.preferredWidth: searchRoot.indexColumnWidth; font.pixelSize: 13 }
            Text { text: "Title"; color: "#666"; Layout.fillWidth: true; font.pixelSize: 13 }
            Text { text: "Artist"; color: "#666"; Layout.preferredWidth: searchRoot.artistColumnWidth; font.pixelSize: 13 }
            Text { text: "Album"; color: "#666"; Layout.preferredWidth: searchRoot.albumColumnWidth; font.pixelSize: 13 }
            Text { text: "Duration"; color: "#666"; Layout.preferredWidth: searchRoot.durationColumnWidth; font.pixelSize: 13; horizontalAlignment: Text.AlignRight }
            Item { Layout.preferredWidth: searchRoot.actionColumnWidth }
            Item { Layout.preferredWidth: 10 }
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

                    Text {
                        text: index + 1
                        color: "#666"
                        Layout.preferredWidth: searchRoot.indexColumnWidth
                        horizontalAlignment: Text.AlignLeft
                    }

                    RowLayout {
                        Layout.fillWidth: true
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
                        Layout.preferredWidth: searchRoot.artistColumnWidth
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                window.selectedArtistName = modelData.artist || ""
                                window.currentViewIndex = 7
                            }
                        }
                    }

                    Text {
                        text: modelData.album || "Single"
                        color: "#AAA"
                        font.pixelSize: 13
                        elide: Text.ElideRight
                        Layout.preferredWidth: searchRoot.albumColumnWidth
                    }

                    Text {
                        text: modelData.duration || "--:--"
                        color: "#AAA"
                        font.pixelSize: 13
                        Layout.preferredWidth: searchRoot.durationColumnWidth
                        horizontalAlignment: Text.AlignRight
                    }

                    Item {
                        Layout.preferredWidth: searchRoot.actionColumnWidth
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
