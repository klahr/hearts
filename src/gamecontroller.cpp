#include "gamecontroller.h"

#include <QVariantMap>

using namespace hearts;

namespace {

// How long the bots think, and how long a finished trick stays on the
// table, from slow to very fast.
const int BotDelayMs[] = {1100, 750, 450, 200};
const int TrickHoldMs[] = {1800, 1200, 800, 450};
const int Targets[] = {50, 100};

QVariantList ids(const Cards &cards)
{
    QVariantList out;
    for (const Card &c : cards)
        out << c.id();
    return out;
}

Cards fromIds(const QVariantList &list)
{
    Cards out;
    for (const QVariant &v : list)
        out.push_back(Card::fromId(v.toInt()));
    return out;
}

} // namespace

GameController::GameController(QObject *parent)
    : QObject(parent)
{
    m_difficulty = qBound(0, m_settings.value(QStringLiteral("difficulty"), 1).toInt(), 2);
    m_target = m_settings.value(QStringLiteral("target"), 100).toInt() == 50 ? 50 : 100;
    m_speed = qBound(0, m_settings.value(QStringLiteral("speed"), 1).toInt(), 3);
    m_botTimer.setSingleShot(true);
    connect(&m_botTimer, &QTimer::timeout, this, &GameController::botStep);
    m_receivedTimer.setSingleShot(true);
    m_receivedTimer.setInterval(2000);
    connect(&m_receivedTimer, &QTimer::timeout, this, [this]() {
        m_received.clear();
        afterAction();
    });
}

int GameController::phase() const
{
    switch (m_game.phase()) {
    case hearts::Phase::Pass:
        return Pass;
    case hearts::Phase::Play:
        return Play;
    case hearts::Phase::Score:
        return Score;
    case hearts::Phase::GameOver:
        break;
    }
    return GameOver;
}

QVariantList GameController::passed() const
{
    QVariantList out;
    for (int p = 0; p < Game::Players; ++p)
        out << QVariant(m_game.phase() == hearts::Phase::Pass ? ids(m_game.passed(p)) : QVariantList());
    return out;
}

QVariantList GameController::hands() const
{
    QVariantList out;
    for (int p = 0; p < Game::Players; ++p)
        out << QVariant(ids(m_game.hand(p)));
    return out;
}

QVariantList GameController::received() const
{
    return m_game.phase() == hearts::Phase::Play ? ids(m_received) : QVariantList();
}

QVariantList GameController::trick() const
{
    QVariantList out;
    for (const hearts::Play &p : m_game.trick()) {
        QVariantMap m;
        m.insert(QStringLiteral("card"), p.card.id());
        m.insert(QStringLiteral("player"), p.player);
        out << m;
    }
    return out;
}

QVariantList GameController::taken() const
{
    QVariantList out;
    for (int p = 0; p < Game::Players; ++p)
        out << QVariant(ids(m_game.taken(p)));
    return out;
}

bool GameController::humanTurn() const
{
    return m_started && m_game.phase() == hearts::Phase::Play && m_game.turn() == Human
           && !m_receivedTimer.isActive();
}

QVariantList GameController::scores() const
{
    QVariantList out;
    for (int p = 0; p < Game::Players; ++p)
        out << m_game.score(p);
    return out;
}

QVariantList GameController::handPoints() const
{
    QVariantList out;
    for (int p = 0; p < Game::Players; ++p)
        out << m_game.handPoints(p);
    return out;
}

QVariantList GameController::handScores() const
{
    QVariantList out;
    for (int p = 0; p < Game::Players; ++p)
        out << m_game.handScore(p);
    return out;
}

void GameController::setDifficulty(int difficulty)
{
    difficulty = qBound(0, difficulty, 2);
    if (difficulty == m_difficulty)
        return;
    m_difficulty = difficulty;
    m_settings.setValue(QStringLiteral("difficulty"), difficulty);
    emit difficultyChanged();
}

void GameController::setTarget(int target)
{
    target = target == Targets[0] ? Targets[0] : Targets[1];
    if (target == m_target)
        return;
    m_target = target;
    m_settings.setValue(QStringLiteral("target"), target);
    emit targetChanged();
}

void GameController::setSpeed(int speed)
{
    speed = qBound(0, speed, 3);
    if (speed == m_speed)
        return;
    m_speed = speed;
    m_settings.setValue(QStringLiteral("speed"), speed);
    emit speedChanged();
}

QVariantList GameController::stats() const
{
    QVariantList out;
    for (int level = 0; level < 3; ++level) {
        const QString key = QStringLiteral("stats/%1/").arg(level);
        QVariantMap m;
        m.insert(QStringLiteral("won"), m_settings.value(key + QStringLiteral("won"), 0).toInt());
        m.insert(QStringLiteral("lost"), m_settings.value(key + QStringLiteral("lost"), 0).toInt());
        m.insert(QStringLiteral("moons"), m_settings.value(key + QStringLiteral("moons"), 0).toInt());
        out << m;
    }
    return out;
}

void GameController::resetStats()
{
    m_settings.remove(QStringLiteral("stats"));
    emit statsChanged();
}

QString GameController::playerName(int player) const
{
    switch (player) {
    case 0:
        return tr("You");
    case 1:
        return tr("West");
    case 2:
        return tr("North");
    case 3:
        return tr("East");
    }
    return QString();
}

void GameController::newGame()
{
    m_botTimer.stop();
    m_receivedTimer.stop();
    m_bot.setDifficulty(static_cast<Difficulty>(m_difficulty));
    m_game.setTarget(m_target);
    m_game.newGame();
    m_started = true;
    m_recorded = false;
    m_scoredRound = 0;
    m_received.clear();
    afterAction();
}

bool GameController::pass(const QVariantList &cardIds)
{
    if (!m_started || !m_game.pass(Human, fromIds(cardIds)))
        return false;
    // The bots have passed already, so the cards have changed hands.
    m_received = m_game.passed(m_game.passSource(Human));
    m_receivedTimer.start();
    afterAction();
    return true;
}

bool GameController::play(int cardId)
{
    if (!humanTurn() || !m_game.play(Human, Card::fromId(cardId)))
        return false;
    m_received.clear();
    afterAction();
    return true;
}

bool GameController::nextHand()
{
    if (!m_game.nextHand())
        return false;
    m_receivedTimer.stop();
    m_received.clear();
    afterAction();
    return true;
}

bool GameController::canPlay(int cardId) const
{
    return humanTurn() && m_game.canPlayCard(Human, Card::fromId(cardId));
}

void GameController::afterAction()
{
    // The bots pass as soon as the cards are dealt.
    if (m_game.phase() == hearts::Phase::Pass) {
        for (int p = 1; p < Game::Players; ++p)
            if (!m_game.hasPassed(p))
                m_game.pass(p, m_bot.choosePass(m_game, p));
    }

    if ((m_game.phase() == hearts::Phase::Score || m_game.phase() == hearts::Phase::GameOver)
        && m_scoredRound != m_game.round()) {
        m_scoredRound = m_game.round();
        if (m_game.moonShooter() == Human) {
            const QString key = QStringLiteral("stats/%1/moons").arg(gameDifficulty());
            m_settings.setValue(key, m_settings.value(key, 0).toInt() + 1);
            emit statsChanged();
        }
    }
    if (m_game.phase() == hearts::Phase::GameOver)
        recordResult();

    if (m_game.phase() == hearts::Phase::Play && !m_receivedTimer.isActive()) {
        if (m_game.trickComplete()) {
            m_botTimer.setInterval(TrickHoldMs[m_speed]);
            m_botTimer.start();
        } else if (m_game.turn() != Human) {
            m_botTimer.setInterval(BotDelayMs[m_speed]);
            m_botTimer.start();
        }
    }
    emit changed();
}

void GameController::botStep()
{
    if (m_game.phase() != hearts::Phase::Play)
        return;
    if (m_game.trickComplete()) {
        m_game.collectTrick();
    } else {
        const int player = m_game.turn();
        if (player == Human)
            return;
        m_game.play(player, m_bot.choosePlay(m_game, player));
    }
    afterAction();
}

void GameController::recordResult()
{
    if (m_recorded)
        return;
    m_recorded = true;
    const QString key = QStringLiteral("stats/%1/").arg(gameDifficulty());
    const QString result = m_game.winner() == Human ? QStringLiteral("won") : QStringLiteral("lost");
    m_settings.setValue(key + result, m_settings.value(key + result, 0).toInt() + 1);
    emit statsChanged();
}
