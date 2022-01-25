#pragma once

#include <QString>
#include <functional>

typedef std::function<void(const QString&, const QString&)> SendMove;

class BaseBot
{
public:
  BaseBot(const SendMove& sendMove);

  virtual void startGame(bool playWithWhite, double secondsPerTurn);
  virtual void enemyMoved(const QString& from, const QString& to); // G2 G4

protected:
  const SendMove m_sendMove;
};
