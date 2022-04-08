#include "BaseBot.h"
#include <QDebug>

BaseBot::BaseBot(const SendMove& sendMove): m_sendMove(sendMove)
{
}

void BaseBot::startGame(bool playWithWhite, double secondsPerTurn)
{
  qInfo() << "BaseBot::startGame, playWithWhite/secondsPerTurn: " << playWithWhite << "/" << secondsPerTurn;
  if(playWithWhite && m_sendMove)
    m_sendMove(QString("G2"), QString("G4"), "");
}

void BaseBot::enemyMoved(const QString& from, const QString& to, const QString& transform)
{
  qInfo() << "BaseBot::enemyMoved, from/to/transform: " << from << "/" << to << transform;
  if(m_sendMove)
    m_sendMove(QString("G7"), QString("G5"), "");
}
