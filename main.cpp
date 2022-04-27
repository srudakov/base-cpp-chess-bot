#include "BaseBot.h"

#include <QCoreApplication>
#include <QCommandLineParser>
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
  const QString TRANS_KEY = "transform";
  
  QJsonObject parse(const QString& message)
  {
    return QJsonDocument::fromJson(message.toUtf8()).object();
  }

  QString validateCell(const QString& cell)
  {
    const QString& upperCell = cell.toUpper();
    const int columnIndex = upperCell.indexOf(QRegExp("[A-H]"));
    const int rowIndex = upperCell.indexOf(QRegExp("[1-8]"));
    if((columnIndex < 0) || (rowIndex < 0))
      return "";
    return upperCell.mid(columnIndex, 1) + upperCell.mid(rowIndex, 1);
  }

  void receiveMessage(const QString& message, BaseBot& bot)
  {
    const QJsonObject& values = parse(message);
    if(values.isEmpty() || !values.contains(MESSAGE_TYPE_KEY))
    {
      qWarning() << "invalid message: " << message;
      return;
    }
    
    const QString& type = values.value(MESSAGE_TYPE_KEY).toString();
    if(type == "start_game")
    {
      const bool white = values.value("color").toString().toLower() == QString("white");
      const double secsPerTurn = values.value("seconds_per_turn").toDouble(2.0);
      bot.startGame(white, secsPerTurn);
      return;
    }
    if(type == "end_game")
    {
      const QString& winner = values.value("winner").toString();
      const QString& reason = values.value("reason").toString();
      qInfo() << "winner/reason: " << winner << "/" << reason;
      return;
    }
    if(type == MOVE_TYPE)
    {
      const QString& from = validateCell(values.value(FROM_KEY).toString());
      const QString& to = validateCell(values.value(TO_KEY).toString());
      const QString& transform = values.value(TRANS_KEY).toString();
      if(from.isEmpty() || to.isEmpty())
        qWarning() << "invalid move: " << message;
      else
      {
        qInfo() << "enemy move: " << from << " -> " << to << "(" << transform << ")";
        bot.enemyMoved(from, to, transform);
      }
      return;
    }
    qWarning() << "unknown message: " << message;
  }
  
  void register_on_server(QWebSocket& webSocket, const QString& name)
  {
    qInfo() << "connected";
    const QJsonObject registration{ {"name", name}, {MESSAGE_TYPE_KEY, "registration"} };
    webSocket.sendBinaryMessage(QJsonDocument(registration).toJson());
  }
  
  void sendMove(QWebSocket& webSocket, const QString& from, const QString& to, const QString& transform)
  {
    qInfo() << "move to send: " << from << " -> " << to;
    const QString& checkedFrom = validateCell(from);
    const QString& checkedTo = validateCell(to);
    if(checkedFrom.isEmpty() || checkedTo.isEmpty())
      return;
    const QJsonObject move{ {FROM_KEY, checkedFrom}, {TO_KEY, checkedTo},
                            {TRANS_KEY, transform}, {MESSAGE_TYPE_KEY, MOVE_TYPE} };
    webSocket.sendBinaryMessage(QJsonDocument(move).toJson());
  }
} // namespace

int main(int argc, char* argv[])
{
  QCoreApplication app(argc, argv);
  QCommandLineParser options;
  const auto& help = options.addHelpOption();
  options.addOptions({ {{"a", "address"}, "web socket server address to connect", "address"},
                       {{"p", "port"}, "web socket server port to connect", "port"},
                       {{"n", "name"}, "this client name to register on server", "name"} });
  options.process(app);
  const QString name = options.isSet("name") ? options.value("name") : QString("base-bot");
  const QString address = options.isSet("address") ? options.value("address") : QString("localhost");
  const QString port = options.isSet("port") ? options.value("port") : QString("6969");

  QWebSocket webSocket;
  BaseBot bot([&webSocket](const QString& from, const QString& to, const QString& transform)
              { sendMove(webSocket, from, to, transform); });

  QObject::connect(&webSocket, &QWebSocket::connected,
                   &webSocket, [&webSocket, &name](){register_on_server(webSocket, name);});
  QObject::connect(&webSocket, &QWebSocket::textMessageReceived,
                   &webSocket, [&bot](const QString& message) {receiveMessage(message, bot);});
  webSocket.open(QUrl(QString("ws://") + address + ":" + port));

  return app.exec();
}
