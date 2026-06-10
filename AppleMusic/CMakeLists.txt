cmake_minimum_required(VERSION 3.16)

project(AppleMusic VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

# 【1. 加 Network】
find_package(Qt6 6.4 REQUIRED COMPONENTS Quick QuickControls2 Multimedia Svg Network Sql)

qt_standard_project_setup()

# 【2. 加 ChatClient 文件】
qt_add_executable(AppleMusic
    main.cpp
    MusicManager.h
    MusicManager.cpp
# 新增
    qml.qrc
    ChatClient.h ChatClient.cpp

    ChatManager.h
    chatmanager.cpp
    chatserver.h chatserver.cpp
)

# 【3. 链接 Network】
target_link_libraries(AppleMusic
    PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Multimedia Qt6::Svg Qt6::Network Qt6::Sql
)
