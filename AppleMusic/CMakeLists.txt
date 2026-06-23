cmake_minimum_required(VERSION 3.16)

project(AppleMusic VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Quick QuickControls2 Multimedia Svg Network Sql)

qt_standard_project_setup()

qt_add_executable(AppleMusic
    main.cpp
    MusicManager.h
    MusicManager.cpp
    qml.qrc
    chatclient.h
    chatclient.cpp
    ChatManager.h
    chatmanager.cpp
    chatserver.h
    chatserver.cpp
    DifyClient.h
    DifyClient.cpp
)

target_link_libraries(AppleMusic
    PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Multimedia Qt6::Svg Qt6::Network Qt6::Sql
)

set_property(TARGET AppleMusic APPEND PROPERTY
    LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/config/dify.ini"
)

add_custom_command(TARGET AppleMusic POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:AppleMusic>/config"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/config/dify.ini"
            "$<TARGET_FILE_DIR:AppleMusic>/config/dify.ini"
    VERBATIM
)

install(TARGETS AppleMusic RUNTIME DESTINATION .)
install(FILES config/dify.ini DESTINATION config)
