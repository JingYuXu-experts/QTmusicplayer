#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include "MusicManager.h"
#include "ChatManager.h"
#include "chatserver.h"
#include "DifyClient.h"

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle("Basic");

    ChatServer server;
    server.startServer();

    MusicManager musicManager;
    ChatManager chatManager;
    DifyClient difyClient;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("musicMgr", &musicManager);
    engine.rootContext()->setContextProperty("chatClient", &chatManager);
    engine.rootContext()->setContextProperty("difyClient", &difyClient);

    const QUrl url(u"qrc:/Main.qml"_s);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
                         if (!obj && url == objUrl)
                             QCoreApplication::exit(-1);
                     }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
