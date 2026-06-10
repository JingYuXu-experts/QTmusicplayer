import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: sidebarRoot
    color: "#000000"
    width: 240

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 10

        RowLayout {
            Layout.preferredHeight: 50
            Layout.fillWidth: true
            spacing: 12
            Rectangle {
                width: 30
                height: 30
                radius: 15
                color: "red"
                Text { text: ">"; color: "white"; anchors.centerIn: parent }
            }
            Text { text: "Music"; color: "white"; font.pixelSize: 26; font.bold: true }
        }

        Item { Layout.preferredHeight: 20 }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            radius: 8
            color: window.currentViewIndex === 0 ? "#222" : "transparent"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 15
                spacing: 15
                Text { text: "H"; color: window.currentViewIndex === 0 ? "white" : "#CCC"; font.pixelSize: 20 }
                Text { text: "Home"; color: window.currentViewIndex === 0 ? "white" : "#CCC"; font.pixelSize: 15; font.bold: true }
            }
            MouseArea { anchors.fill: parent; onClicked: window.currentViewIndex = 0 }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            radius: 8
            color: window.currentViewIndex === 1 ? "#222" : "transparent"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 15
                spacing: 15
                Text { text: "S"; color: window.currentViewIndex === 1 ? "white" : "#CCC"; font.pixelSize: 20 }
                Text { text: "Search"; color: window.currentViewIndex === 1 ? "white" : "#CCC"; font.pixelSize: 15; font.bold: true }
            }
            MouseArea { anchors.fill: parent; onClicked: window.currentViewIndex = 1 }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            radius: 8
            color: window.currentViewIndex === 2 ? "#222" : "transparent"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 15
                spacing: 15
                Text { text: "L"; color: window.currentViewIndex === 2 ? "white" : "#CCC"; font.pixelSize: 20 }
                Text { text: "Library"; color: window.currentViewIndex === 2 ? "white" : "#CCC"; font.pixelSize: 15; font.bold: true }
            }
            MouseArea { anchors.fill: parent; onClicked: window.currentViewIndex = 2 }
        }

        Item { Layout.preferredHeight: 10 }
        Rectangle { Layout.fillWidth: true; height: 1; color: "#333" }
        Item { Layout.preferredHeight: 10 }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: "transparent"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 15
                spacing: 15
                Text { text: "+"; color: "#CCC"; font.pixelSize: 20 }
                Text { text: "New Playlist"; color: "#CCC"; font.pixelSize: 15; font.bold: true }
            }
        }

        Item { Layout.fillHeight: true }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            Text { text: "SOCIAL"; color: "#666"; font.pixelSize: 12; font.bold: true }

            Rectangle {
                Layout.fillWidth: true
                height: 48
                color: "#161616"
                radius: 8

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    spacing: 12
                    Rectangle { width: 32; height: 32; radius: 16; color: "#007AFF"; Text { text: "C"; color: "white"; anchors.centerIn: parent } }
                    Column {
                        Text { text: "Music Chat"; color: "white"; font.bold: true; font.pixelSize: 14 }
                        Text { text: "3 friends online"; color: "#4CD964"; font.pixelSize: 11 }
                    }
                }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: window.currentViewIndex = 5 }
            }

            RowLayout {
                spacing: 10
                Rectangle { width: 8; height: 8; radius: 4; color: "#4CD964" }
                Text { text: "Jay Chou"; color: "white"; font.bold: true }
                Text { text: "Online"; color: "#888"; font.pixelSize: 12 }
            }
            RowLayout {
                spacing: 10
                Rectangle { width: 8; height: 8; radius: 4; color: "#4CD964" }
                Text { text: "Taylor"; color: "white"; font.bold: true }
                Text { text: "Midnights"; color: "#888"; font.pixelSize: 12 }
            }
        }

        Item { Layout.preferredHeight: 20 }
    }
}
