import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "#030303"
    property string searchText: searchInput.text

    RowLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        Text { text: "Music"; color: "white"; font.bold: true; font.pixelSize: 22 }

        Rectangle {
            Layout.fillWidth: true
            Layout.maximumWidth: 500
            Layout.preferredHeight: 40
            color: "#212121"
            radius: 8

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                Text { text: "Search"; color: "gray"; font.pixelSize: 12 }

                TextInput {
                    id: searchInput
                    Layout.fillWidth: true
                    color: "white"
                    font.pixelSize: 16
                    selectByMouse: true
                    clip: true

                    Text {
                        text: "Search songs, albums, artists..."
                        color: "#AAAAAA"
                        visible: !parent.text
                    }

                    onTextEdited: musicMgr.search(text)
                }

                Text {
                    text: "Clear"
                    color: "gray"
                    visible: searchInput.text.length > 0
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            searchInput.text = ""
                            musicMgr.search("")
                        }
                    }
                }
            }
        }

        Rectangle {
            width: 32
            height: 32
            radius: 16
            color: "brown"
            Text { text: "J"; color: "white"; anchors.centerIn: parent }
        }
    }
}
