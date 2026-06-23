import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#080808"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 68
            color: "#111111"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 22
                anchors.rightMargin: 22
                spacing: 14

                Rectangle {
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                    radius: 20
                    color: "#8B5CF6"
                    Text {
                        anchors.centerIn: parent
                        text: "AI"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 13
                    }
                }

                Column {
                    spacing: 3
                    Text {
                        text: "Dify AI 音乐助手"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 17
                    }
                    Text {
                        text: difyClient.statusText
                        color: difyClient.configured ? "#999999" : "#FF6B6B"
                        font.pixelSize: 12
                        elide: Text.ElideRight
                        width: 520
                    }
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "清空对话"
                    enabled: !difyClient.busy && difyClient.messages.length > 0
                    onClicked: difyClient.clearHistory()
                }
            }

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: "#242424"
            }
        }

        ListView {
            id: messageList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: difyClient.messages
            spacing: 10
            topMargin: 20
            bottomMargin: 20

            onCountChanged: Qt.callLater(function() {
                if (count > 0)
                    positionViewAtEnd()
            })

            header: Item {
                width: messageList.width
                height: difyClient.messages.length === 0 ? welcomeCard.height + 20 : 0
                visible: difyClient.messages.length === 0

                Rectangle {
                    id: welcomeCard
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: Math.min(parent.width - 80, 620)
                    height: welcomeColumn.implicitHeight + 38
                    radius: 14
                    color: "#151515"
                    border.color: "#292929"

                    Column {
                        id: welcomeColumn
                        anchors.fill: parent
                        anchors.margins: 19
                        spacing: 10

                        Text {
                            text: "你好，我是你的 AI 音乐助手"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 18
                        }
                        Text {
                            width: parent.width
                            text: "你可以询问音乐推荐、歌手和歌曲信息，也可以咨询播放器的使用方法。"
                            color: "#B8B8B8"
                            font.pixelSize: 14
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }

            delegate: Item {
                width: messageList.width
                height: bubble.height + 8

                property bool fromUser: modelData.role === "user"

                Rectangle {
                    id: bubble
                    anchors.right: fromUser ? parent.right : undefined
                    anchors.left: fromUser ? undefined : parent.left
                    anchors.rightMargin: 26
                    anchors.leftMargin: 26
                    width: Math.min(messageList.width * 0.72,
                                    Math.max(180, messageText.implicitWidth + 32))
                    height: messageColumn.implicitHeight + 24
                    radius: 14
                    color: modelData.isError ? "#3A1717"
                                             : (fromUser ? "#6D28D9" : "#1B1B1B")
                    border.color: modelData.isError ? "#7F1D1D"
                                                   : (fromUser ? "#7C3AED" : "#2C2C2C")

                    Column {
                        id: messageColumn
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 5

                        Text {
                            text: fromUser ? "我" : "AI 助手"
                            color: modelData.isError ? "#FF9B9B"
                                                     : (fromUser ? "#E9D5FF" : "#A78BFA")
                            font.bold: true
                            font.pixelSize: 11
                        }
                        Text {
                            id: messageText
                            width: bubble.width - 24
                            text: modelData.content
                            color: "white"
                            font.pixelSize: 14
                            wrapMode: Text.Wrap
                            textFormat: Text.PlainText
                        }
                    }
                }
            }

            footer: Item {
                width: messageList.width
                height: difyClient.busy ? 54 : 0
                visible: difyClient.busy

                Row {
                    anchors.left: parent.left
                    anchors.leftMargin: 30
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 10

                    BusyIndicator {
                        width: 28
                        height: 28
                        running: difyClient.busy
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "AI 正在生成回答…"
                        color: "#AAAAAA"
                        font.pixelSize: 13
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 92
            color: "#111111"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                anchors.topMargin: 14
                anchors.bottomMargin: 14
                spacing: 12

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 12
                    color: "#202020"
                    border.color: inputArea.activeFocus ? "#8B5CF6" : "#333333"

                    TextArea {
                        id: inputArea
                        anchors.fill: parent
                        anchors.margins: 8
                        color: "white"
                        placeholderText: difyClient.configured ? "输入你的问题…" : "Dify 配置不可用"
                        placeholderTextColor: "#777777"
                        font.pixelSize: 14
                        wrapMode: TextEdit.Wrap
                        enabled: difyClient.configured && !difyClient.busy
                        selectByMouse: true

                        Keys.onPressed: function(event) {
                            if ((event.modifiers & Qt.ControlModifier)
                                    && (event.key === Qt.Key_Return || event.key === Qt.Key_Enter)) {
                                root.sendCurrentPrompt()
                                event.accepted = true
                            }
                        }
                    }
                }

                Button {
                    Layout.preferredWidth: 88
                    Layout.fillHeight: true
                    text: difyClient.busy ? "生成中" : "发送"
                    enabled: difyClient.configured
                             && !difyClient.busy
                             && inputArea.text.trim().length > 0
                    onClicked: root.sendCurrentPrompt()
                }
            }
        }
    }

    function sendCurrentPrompt() {
        var prompt = inputArea.text.trim()
        if (prompt.length === 0 || difyClient.busy || !difyClient.configured)
            return
        difyClient.sendPrompt(prompt)
        inputArea.clear()
    }
}
