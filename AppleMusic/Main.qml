import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1280; height: 800; visible: true
    title: "CDUESTC MUSIC"
    color: "#030303"

    // 0:首页, 1:搜索, 2:媒体库, 3:歌词, 4:MV, 5:聊天室
    property int currentViewIndex: 0

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 顶部栏 (MV模式隐藏)
        TopBar {
            id: topBar
            visible: window.currentViewIndex !== 4
            Layout.fillWidth: true; Layout.preferredHeight: 64; z: 10
            onSearchTextChanged: window.currentViewIndex = searchText.length > 0 ? 1 : 0
        }

        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0

            // 侧边栏 (MV模式隐藏)
            Sidebar {
                visible: window.currentViewIndex !== 4
                Layout.preferredWidth: 240; Layout.fillHeight: true
            }

            // 页面切换栈
            StackLayout {
                id: contentStack
                Layout.fillWidth: true; Layout.fillHeight: true
                currentIndex: window.currentViewIndex

                HomeView {}      // 0: 首页 (已包含8首歌)
                SearchView { searchText: topBar.searchText } // 1: 搜索
                LibraryView {}   // 2: 媒体库
                LyricsView {}    // 3: 歌词
                MvView {}        // 4: MV

                // 【核心新增】聊天室页面 (Index 5)
                ChatView {}
            }
        }

        // 底部播放栏 (MV模式隐藏)
        PlayerBar {
            visible: window.currentViewIndex !== 4
            Layout.fillWidth: true; Layout.preferredHeight: 80; z: 20
        }
    }
}
