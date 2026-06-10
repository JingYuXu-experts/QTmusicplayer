import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "#000000"

    Component.onCompleted: chatClient.connectToServer()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#111"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 15

                Rectangle {
                    width: 36
                    height: 36
                    radius: 18
                    color: "#007AFF"
                    Text { text: "C"; color: "white"; anchors.centerIn: parent; font.pixelSize: 18 }
                }

                Column {
                    Text { text: "Public Chat"; color: "white"; font.bold: true; font.pixelSize: 16 }
                    Text { text: chatClient.statusText; color: "#666"; font.pixelSize: 12 }
                }

                Item { Layout.fillWidth: true }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: "#222"
                anchors.bottom: parent.bottom
            }
        }

        ListView {
            id: chatList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: chatClient.messages
            spacing: 12
            leftMargin: 30
            rightMargin: 30
            topMargin: 25
            bottomMargin: 25

            delegate: Rectangle {
                width: chatList.width - 60
                height: Math.max(56, messageColumn.implicitHeight + 18)
                radius: 8
                color: "#161616"

                Column {
                    id: messageColumn
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 4

                    Text {
                        text: modelData.user || modelData.userName || "Friend"
                        color: "#4CD964"
                        font.bold: true
                        font.pixelSize: 12
                    }
                    Text {
                        text: modelData.content || modelData.text || ""
                        color: "white"
                        font.pixelSize: 14
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 76
            color: "#111"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 12

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    radius: 20
                    color: "#222"

                    TextInput {
                        id: inputField
                        anchors.fill: parent
                        anchors.leftMargin: 18
                        anchors.rightMargin: 18
                        verticalAlignment: TextInput.AlignVCenter
                        color: "white"
                        font.pixelSize: 14
                        selectByMouse: true
                        clip: true

                        Text {
                            text: "Send a message..."
                            color: "#555"
                            visible: !parent.text
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                        }

                        onAccepted: sendBtn.clicked()
                    }
                }

                Rectangle {
                    id: sendBtn
                    width: 40
                    height: 40
                    radius: 20
                    color: inputField.text ? "#0A84FF" : "#333"

                    Text {
                        text: ">"
                        color: "white"
                        font.pixelSize: 24
                        anchors.centerIn: parent
                    }

                    signal clicked()
                    MouseArea {
                        anchors.fill: parent
                        onClicked: parent.clicked()
                    }

                    onClicked: {
                        if (inputField.text.length > 0) {
                            chatClient.sendMessage("Jingyu", inputField.text)
                            inputField.text = ""
                        }
                    }
                }
            }
        }
    }
}
