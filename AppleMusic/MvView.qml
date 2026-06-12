import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

Rectangle {
    id: mvRoot
    color: "#000000"

    property string mvSource: musicMgr.getMvPath(musicMgr.currentTitle)
    property bool isFullScreen: window.visibility === Window.FullScreen
    property string currentQuality: "1080P"
    property bool showDescription: true
    property bool controlsVisible: true

    Timer {
        id: hideTimer
        interval: 3000
        onTriggered: controlsVisible = false
    }

    onVisibleChanged: {
        if (visible) {
            mvSource = musicMgr.getMvPath(musicMgr.currentTitle)
            if (mvSource !== "") {
                musicMgr.stopPlayback()
                videoPlayer.source = mvSource
                videoPlayer.play()
            }
        } else {
            videoPlayer.stop()
        }
    }

    MediaPlayer {
        id: videoPlayer
        audioOutput: AudioOutput { id: mvAudio; volume: 1.0 }

        onPlaybackStateChanged: {
            if (playbackState === MediaPlayer.StoppedState) controlsVisible = true
        }
    }

    function formatTime(ms) {
        if (ms <= 0) return "0:00"
        var totalSec = Math.floor(ms / 1000)
        var m = Math.floor(totalSec / 60)
        var s = totalSec % 60
        return m + ":" + (s < 10 ? "0" + s : s)
    }

    function getRealDescription(title) {
        if (title.indexOf("我是如此相信") !== -1)
            return "Jay Chou - I Truly Believe\nTheme song MV."
        if (title.indexOf("Yellow") !== -1)
            return "Coldplay - Yellow (Official Video)"
        if (title.indexOf("Sick Boy") !== -1)
            return "The Chainsmokers - Sick Boy"
        if (title.indexOf("Closer") !== -1)
            return "The Chainsmokers - Closer ft. Halsey"

        return musicMgr.currentTitle + " - Official Music Video\n\nLocal MV"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "black"

            VideoOutput {
                id: videoOutput
                anchors.fill: parent
                fillMode: VideoOutput.PreserveAspectFit
            }
            Component.onCompleted: videoPlayer.videoOutput = videoOutput

            MouseArea {
                id: hoverArea
                anchors.fill: parent
                hoverEnabled: true
                onPositionChanged: { controlsVisible = true; hideTimer.restart() }
                onClicked: {
                    if (videoPlayer.playbackState === MediaPlayer.PlayingState) videoPlayer.pause()
                    else videoPlayer.play()
                    controlsVisible = true
                }
            }

            Rectangle {
                anchors.centerIn: parent
                width: 70
                height: 70
                radius: 35
                color: "#80000000"
                visible: videoPlayer.playbackState !== MediaPlayer.PlayingState
                Text { text: "Play"; color: "white"; font.pixelSize: 24; anchors.centerIn: parent }
            }

            Rectangle {
                width: parent.width
                height: 60
                anchors.top: parent.top
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#CC000000" }
                    GradientStop { position: 1.0; color: "transparent" }
                }
                visible: controlsVisible

                Text {
                    text: "< Back to music"
                    color: "white"
                    font.pixelSize: 18
                    font.bold: true
                    anchors.left: parent.left
                    anchors.leftMargin: 20
                    anchors.verticalCenter: parent.verticalCenter
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            videoPlayer.stop()
                            if (window.visibility === Window.FullScreen) window.visibility = Window.Windowed
                            window.currentViewIndex = 0
                        }
                    }
                }
            }

            Rectangle {
                id: bottomControl
                width: parent.width - 40
                height: 50
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20
                anchors.horizontalCenter: parent.horizontalCenter
                radius: 8
                color: "#CC000000"
                visible: controlsVisible

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 15
                    anchors.rightMargin: 15
                    spacing: 15

                    Text {
                        text: videoPlayer.playbackState === MediaPlayer.PlayingState ? "Pause" : "Play"
                        color: "white"
                        font.pixelSize: 14
                        MouseArea { anchors.fill: parent; onClicked: videoPlayer.playbackState === MediaPlayer.PlayingState ? videoPlayer.pause() : videoPlayer.play() }
                    }

                    Text { text: formatTime(videoPlayer.position); color: "white"; font.pixelSize: 12 }
                    Slider {
                        Layout.fillWidth: true
                        height: 20
                        from: 0
                        to: videoPlayer.duration
                        value: videoPlayer.position
                        onMoved: videoPlayer.setPosition(value)

                        background: Rectangle {
                            y: 9
                            width: parent.width
                            height: 3
                            color: "#555"
                            Rectangle { width: parent.parent.visualPosition * parent.width; height: 3; color: "#FF0000" }
                        }
                        handle: Item {}
                    }
                    Text { text: formatTime(videoPlayer.duration); color: "white"; font.pixelSize: 12 }

                    Rectangle {
                        width: 50
                        height: 24
                        radius: 4
                        color: "transparent"
                        border.color: "white"
                        border.width: 1
                        Text { text: currentQuality; color: "white"; font.pixelSize: 12; anchors.centerIn: parent }
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: qualityMenu.open() }
                        Menu {
                            id: qualityMenu
                            y: -110
                            x: -10
                            background: Rectangle { color: "#222"; border.color: "#444"; radius: 4 }
                            MenuItem { text: "1080P HD"; onTriggered: changeQuality("1080P") }
                            MenuItem { text: "720P"; onTriggered: changeQuality("720P") }
                            MenuItem { text: "480P"; onTriggered: changeQuality("480P") }
                        }
                    }

                    Text {
                        text: "Info"
                        color: showDescription ? "white" : "#AAA"
                        font.pixelSize: 14
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: showDescription = !showDescription
                        }
                    }

                    RowLayout {
                        spacing: 5
                        Text { text: "Vol"; color: "white"; font.pixelSize: 14 }
                        Slider {
                            Layout.preferredWidth: 80
                            from: 0
                            to: 1.0
                            value: mvAudio.volume
                            onMoved: mvAudio.volume = value

                            background: Rectangle {
                                y: 9
                                width: parent.width
                                height: 3
                                color: "#555"
                                Rectangle { width: parent.parent.visualPosition * parent.width; height: 3; color: "white" }
                            }
                            handle: Item {}
                        }
                    }

                    Text {
                        text: isFullScreen ? "Exit" : "Full"
                        color: "white"
                        font.pixelSize: 14
                        MouseArea {
                            anchors.fill: parent
                            onClicked: window.visibility = isFullScreen ? Window.Windowed : Window.FullScreen
                        }
                    }
                }
            }
        }

        Rectangle {
            id: infoPanel
            Layout.preferredHeight: showDescription ? 140 : 0
            Layout.fillWidth: true
            color: "#161616"
            clip: true

            Behavior on Layout.preferredHeight { NumberAnimation { duration: 300; easing.type: Easing.OutQuad } }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 25
                visible: showDescription

                Rectangle {
                    width: 50
                    height: 50
                    radius: 25
                    color: "#333"
                    Image {
                        anchors.fill: parent
                        source: musicMgr.currentCover
                        fillMode: Image.PreserveAspectCrop
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 8

                    Text {
                        text: musicMgr.currentTitle + " (Official MV)"
                        color: "white"
                        font.pixelSize: 22
                        font.bold: true
                    }

                    Text {
                        text: getRealDescription(musicMgr.currentTitle)
                        color: "#AAA"
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }

                Column {
                    spacing: 10
                    Rectangle {
                        width: 90
                        height: 30
                        radius: 15
                        color: "#CC0000"
                        Text { text: "Subscribe"; color: "white"; font.bold: true; anchors.centerIn: parent }
                    }
                    Text { text: "12.5K likes"; color: "#999"; font.pixelSize: 12; anchors.horizontalCenter: parent.horizontalCenter }
                }
            }
        }
    }

    function changeQuality(q) {
        currentQuality = q
        var pos = videoPlayer.position
        videoPlayer.pause()
        qualityTimer.position = pos
        qualityTimer.restart()
    }

    Timer {
        id: qualityTimer
        interval: 500
        property int position: 0
        onTriggered: {
            videoPlayer.play()
            videoPlayer.setPosition(position)
        }
    }
}
