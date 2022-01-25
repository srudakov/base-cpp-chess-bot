#include "BaseBot.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QWebSocket>
#include <QRegExp>
#include <QDebug>

namespace
{
  const QString MESSAGE_TYPE_KEY = "message";
  const QString MOVE_TYPE = "move";
  const QString FROM_KEY = "from";
  const QString TO_KEY = "to";
  
  QJsonObject parse(const QString& message)
  {
    return QJsonDocument::fromJson(message.toUtf8()).object();
  }
  
  void receiveMessage(const QString& message, BaseBot& bot)
  {
    qInfo() << "message: " << message;
    const QJsonObject& values = parse(message);
    if(values.isEmpty() || !values.contains(MESSAGE_TYPE_KEY))
      return;
    
    const QString& type = values.value(MESSAGE_TYPE_KEY).toString();
    if(type == "start_game")
    {
      const bool white = values.value("color").toString().toLower() == QString("white");
      const double secsPerTurn = values.value("seconds_per_turn").toDouble(2.0);
      bot.startGame(white, secsPerTurn);
    }
    if(type == "end_game")
    {
      const QString& winner = values.value("winner").toString();
      const QString& reason = values.value("reason").toString();
      qInfo() << "winner/reason: " << winner << "/" << reason;
    }
    if(type == MOVE_TYPE)
    {
      const QString& from = values.value(FROM_KEY).toString();
      const QString& to = values.value(TO_KEY).toString();
      qInfo() << "enemy move: " << from << " -> " << to;
      bot.enemyMoved(from, to);
    }
  }
  
  void register_on_server(QWebSocket& webSocket)
  {
    qInfo() << "connected";
    const QJsonObject registration{ {"name", "qclient"}, {MESSAGE_TYPE_KEY, "registration"} };
    webSocket.sendBinaryMessage(QJsonDocument(registration).toJson());
  }
  
  void sendMove(QWebSocket& webSocket, const QString& from, const QString& to)
  {
    qInfo() << "move to send: " << from << " -> " << to;
    const QJsonObject move{ {FROM_KEY, from}, {TO_KEY, to}, {MESSAGE_TYPE_KEY, MOVE_TYPE} };
    webSocket.sendBinaryMessage(QJsonDocument(move).toJson());
  }
} // namespace

int main(int argc, char* argv[])
{
  QCoreApplication app(argc, argv);
  QWebSocket webSocket;
  BaseBot bot([&webSocket](const QString& from, const QString& to) { sendMove(webSocket, from, to); });
  QObject::connect(&webSocket, &QWebSocket::connected, &webSocket, [&webSocket](){register_on_server(webSocket);});
  QObject::connect(&webSocket, &QWebSocket::textMessageReceived, &webSocket, [&bot](const QString& message) {receiveMessage(message, bot);});
  webSocket.open(QUrl(QString("ws://localhost:6969")));

  return app.exec();
}
