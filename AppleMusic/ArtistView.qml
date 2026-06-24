import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: artistRoot
    color: "#000000"

    function openArtist(name) {
        var clean = (name || "").trim()
        if (clean.length === 0) {
            return
        }
        window.selectedArtistName = clean
        artistInput.text = clean
        musicMgr.fetchArtistInfo(clean)
    }

    Component.onCompleted: {
        var artists = musicMgr.localArtists()
        var initial = window.selectedArtistName || ""
        if (initial.length === 0 && artists.length > 0) {
            initial = artists[0].name
        }
        openArtist(initial.length > 0 ? initial : "Taylor Swift")
    }

    onVisibleChanged: {
        if (visible && window.selectedArtistName && window.selectedArtistName !== artistInput.text) {
            openArtist(window.selectedArtistName)
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: artistRoot.width
            spacing: 0

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 320

                Image {
                    anchors.fill: parent
                    source: musicMgr.artistProfile.background || ""
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    opacity: status === Image.Ready ? 0.62 : 0
                }

                Rectangle {
                    anchors.fill: parent
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#33000000" }
                        GradientStop { position: 0.58; color: "#99000000" }
                        GradientStop { position: 1.0; color: "#000000" }
                    }
                }

                RowLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 40
                    spacing: 24

                    Rectangle {
                        Layout.preferredWidth: 150
                        Layout.preferredHeight: 150
                        radius: 8
                        color: "#181818"
                        clip: true

                        Image {
                            anchors.fill: parent
                            source: musicMgr.artistProfile.avatar || musicMgr.artistProfile.background || ""
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Text {
                            text: "ARTIST"
                            color: "#CCCCCC"
                            font.pixelSize: 12
                            font.bold: true
                        }

                        Text {
                            text: musicMgr.artistProfile.name || window.selectedArtistName || "Artist"
                            color: "white"
                            font.pixelSize: 56
                            font.bold: true
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Text {
                            text: musicMgr.artistProfile.subtitle || musicMgr.artistProfile.source || ""
                            color: "#DDDDDD"
                            font.pixelSize: 15
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            spacing: 14
                            Text { text: (musicMgr.artistProfile.trackCount || musicMgr.artistTracks.length || 0) + " songs"; color: "#CCCCCC"; font.pixelSize: 13 }
                            Text { text: (musicMgr.artistProfile.albumCount || 0) + " albums"; color: "#CCCCCC"; font.pixelSize: 13; visible: (musicMgr.artistProfile.albumCount || 0) > 0 }
                            Text { text: (musicMgr.artistProfile.fanCount || 0) + " fans"; color: "#CCCCCC"; font.pixelSize: 13; visible: (musicMgr.artistProfile.fanCount || 0) > 0 }
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 40
                Layout.rightMargin: 40
                Layout.bottomMargin: 120
                spacing: 24

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    TextField {
                        id: artistInput
                        Layout.preferredWidth: 360
                        placeholderText: "Artist name"
                        color: "white"
                        selectedTextColor: "white"
                        selectionColor: "#E53935"
                        background: Rectangle {
                            radius: 8
                            color: "#161616"
                            border.color: artistInput.activeFocus ? "#E53935" : "#303030"
                        }
                        onAccepted: artistRoot.openArtist(text)
                    }

                    Button {
                        text: musicMgr.artistLoading ? "Loading" : "Load"
                        enabled: !musicMgr.artistLoading
                        onClicked: artistRoot.openArtist(artistInput.text)
                    }

                    Text {
                        text: musicMgr.artistError
                        visible: musicMgr.artistError.length > 0
                        color: "#FF7676"
                        font.pixelSize: 13
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 10

                    Repeater {
                        model: musicMgr.localArtists()
                        delegate: Rectangle {
                            width: Math.max(120, artistName.implicitWidth + 34)
                            height: 34
                            radius: 17
                            color: area.containsMouse ? "#2A2A2A" : "#161616"
                            border.color: "#333333"

                            Text {
                                id: artistName
                                anchors.centerIn: parent
                                text: modelData.name
                                color: "white"
                                font.pixelSize: 13
                            }

                            MouseArea {
                                id: area
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: artistRoot.openArtist(modelData.name)
                            }
                        }
                    }
                }

                Text {
                    text: musicMgr.artistProfile.description || ""
                    color: "#D6D6D6"
                    font.pixelSize: 15
                    lineHeight: 1.25
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    visible: text.length > 0
                }

                Text {
                    text: "Songs"
                    color: "white"
                    font.pixelSize: 24
                    font.bold: true
                }

                ListView {
                    id: trackList
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.max(260, Math.min(620, contentHeight))
                    interactive: false
                    clip: true
                    model: musicMgr.artistTracks

                    delegate: Rectangle {
                        width: trackList.width
                        height: 64
                        radius: 6
                        color: rowArea.containsMouse ? "#1A1A1A" : "transparent"

                        MouseArea {
                            id: rowArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onDoubleClicked: musicMgr.playTrack(modelData.title || "", modelData.artist || "", modelData.cover || modelData.image || "", modelData.url || modelData.preview || "")
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 14

                            Text { text: index + 1; color: "#777"; Layout.preferredWidth: 28 }

                            Image {
                                Layout.preferredWidth: 44
                                Layout.preferredHeight: 44
                                source: modelData.cover || modelData.image || ""
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 3
                                Text { text: modelData.title || "Unknown title"; color: "white"; font.bold: true; font.pixelSize: 14; elide: Text.ElideRight; Layout.fillWidth: true }
                                Text { text: (modelData.album || "Single") + " - " + (modelData.source || "Online"); color: "#9A9A9A"; font.pixelSize: 12; elide: Text.ElideRight; Layout.fillWidth: true }
                            }

                            Text { text: modelData.duration || "--:--"; color: "#AAAAAA"; font.pixelSize: 13; Layout.preferredWidth: 58; horizontalAlignment: Text.AlignRight }
                            Button {
                                text: "Play"
                                visible: rowArea.containsMouse
                                onClicked: musicMgr.playTrack(modelData.title || "", modelData.artist || "", modelData.cover || modelData.image || "", modelData.url || modelData.preview || "")
                            }
                        }
                    }
                }

                Text {
                    visible: !musicMgr.artistLoading && musicMgr.artistTracks.length === 0
                    text: "No songs found"
                    color: "#999999"
                    font.pixelSize: 15
                }
            }
        }
    }
}
