#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include "MusicManager.h"
#include "ChatManager.h"
#include "ChatServer.h" // 【新增】

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle("Basic");

    // 1. 启动服务器 (在后台运行)
    ChatServer server;
    server.startServer();

    // 2. 启动客户端业务逻辑
    MusicManager musicManager;
    ChatManager chatManager;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("musicMgr", &musicManager);
    engine.rootContext()->setContextProperty("chatClient", &chatManager);

    const QUrl url(u"qrc:/Main.qml"_s);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
                         if (!obj && url == objUrl)
                             QCoreApplication::exit(-1);
                     }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
