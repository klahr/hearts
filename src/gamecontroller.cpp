#include "gamecontroller.h"

#include "cards/settings.h"
#include "cards/variant.h"

#include <QVariantMap>

#include <algorithm>

using namespace hearts;

namespace {

// How long the bots think, and how long a finished trick stays on the
// table, from slow to very fast.
const int BotDelayMs[] = {1100, 750, 450, 200};
const int TrickHoldMs[] = {1800, 1200, 800, 450};
const int Targets[] = {50, 100};

} // namespace

GameController::GameController(Session *online, QObject *parent)
    : QObject(parent)
    , m_online(online)
    , m_settings(cards::settingsPath(), QSettings::IniFormat)
{
    m_difficulty = qBound(0, m_settings.value(QStringLiteral("difficulty"), 1).toInt(), 2);
    m_target = m_settings.value(QStringLiteral("target"), 100).toInt() == 50 ? 50 : 100;
    m_speed = qBound(0, m_settings.value(QStringLiteral("speed"), 1).toInt(), 3);
    m_tapTwice = m_settings.value(QStringLiteral("tapTwice"), false).toBool();
    m_botTimer.setSingleShot(true);
    connect(&m_botTimer, &QTimer::timeout, this, &GameController::botStep);
    m_receivedTimer.setSingleShot(true);
    m_receivedTimer.setInterval(2000);
    connect(&m_receivedTimer, &QTimer::timeout, this, [this]() {
        m_received.clear();
        afterAction();
    });
    connect(m_online, &Session::reset, this, &GameController::replay);
    connect(m_online, &Session::action, this, [this](const QVariantMap &action) {
        if (m_online->active() && apply(action))
            afterAction();
    });
    connect(m_online, &Session::tableChanged, this, &GameController::onTableChanged);
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

QVariantList GameController::lastTrick() const
{
    return cards::lastTrick(m_game.played(), m_game.tricksPlayed(), Game::Players, m_game.leader());
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

void GameController::setTapTwice(bool on)
{
    if (on == m_tapTwice)
        return;
    m_tapTwice = on;
    m_settings.setValue(QStringLiteral("tapTwice"), on);
    emit tapTwiceChanged();
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
    if (online() && player > 0 && player < Game::Players)
        return m_online->playerName(m_online->toTable(player));
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

void GameController::start()
{
    if (online())
        return;
    m_botTimer.stop();
    m_receivedTimer.stop();
    m_game = hearts::Game();
    m_started = false;
    m_received.clear();
    m_replaying = true;
    bool ok = true;
    for (const QVariant &action : m_log.load())
        ok = ok && apply(action.toMap());
    m_replaying = false;
    if (!ok || !m_started) {
        newGame();
        return;
    }
    // Whatever was counted before the app closed is not counted again.
    if (m_game.phase() == hearts::Phase::Score || m_game.phase() == hearts::Phase::GameOver)
        m_scoredRound = m_game.round();
    m_recorded = m_game.phase() == hearts::Phase::GameOver;
    afterAction();
}

void GameController::newGame()
{
    if (online()) {
        if (!m_started || m_game.phase() == hearts::Phase::GameOver)
            sendNew();
        return;
    }
    m_botTimer.stop();
    m_receivedTimer.stop();
    m_log.clear();
    m_started = false;
    perform(newAction());
}

bool GameController::pass(const QVariantList &cardIds)
{
    if (!m_started || m_game.phase() != hearts::Phase::Pass || m_game.hasPassed(Human) || cardIds.size() != 3)
        return false;
    return perform({{QStringLiteral("t"), QStringLiteral("pass")},
                    {QStringLiteral("seat"), m_online->toTable(Human)},
                    {QStringLiteral("cards"), cardIds}});
}

bool GameController::play(int cardId)
{
    if (!canPlay(cardId))
        return false;
    return perform({{QStringLiteral("t"), QStringLiteral("play")},
                    {QStringLiteral("seat"), m_online->toTable(Human)},
                    {QStringLiteral("card"), cardId}});
}

bool GameController::nextHand()
{
    if (m_game.phase() != hearts::Phase::Score)
        return false;
    return perform({{QStringLiteral("t"), QStringLiteral("next")}, {QStringLiteral("hands"), dealHands()}});
}

bool GameController::canPlay(int cardId) const
{
    return humanTurn() && m_game.canPlayCard(Human, Card::fromId(cardId));
}

void GameController::afterAction()
{
    // The bots pass as soon as the cards are dealt.
    if (m_game.phase() == hearts::Phase::Pass && !online()) {
        for (int p = 1; p < Game::Players; ++p) {
            if (m_game.hasPassed(p))
                continue;
            const QVariantMap action = {{QStringLiteral("t"), QStringLiteral("pass")},
                                        {QStringLiteral("seat"), p},
                                        {QStringLiteral("cards"), ids(m_bot.choosePass(m_game, p))}};
            if (apply(action))
                m_log.append(action);
        }
    } else if (m_game.phase() == hearts::Phase::Pass && m_started && m_online->driver()
               && m_botPassRound != m_game.round()) {
        m_botPassRound = m_game.round();
        for (int p = 1; p < Game::Players; ++p)
            if (!m_game.hasPassed(p) && m_online->isBot(m_online->toTable(p)))
                m_online->send({{QStringLiteral("t"), QStringLiteral("pass")},
                                {QStringLiteral("seat"), m_online->toTable(p)},
                                {QStringLiteral("cards"), ids(m_bot.choosePass(m_game, p))}});
    }

    if (!online() && (m_game.phase() == hearts::Phase::Score || m_game.phase() == hearts::Phase::GameOver)
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

    // Online, one app at the table runs the bots and gathers the tricks.
    if (m_game.phase() == hearts::Phase::Play && !m_receivedTimer.isActive()
        && (!online() || (m_started && m_online->driver()))) {
        if (m_game.trickComplete()) {
            m_botTimer.setInterval(TrickHoldMs[m_speed]);
            m_botTimer.start();
        } else if (m_game.turn() != Human && (!online() || m_online->isBot(m_online->toTable(m_game.turn())))) {
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
    if (online()) {
        if (!m_online->driver())
            return;
        if (m_game.trickComplete()) {
            m_online->send({{QStringLiteral("t"), QStringLiteral("collect")}});
        } else {
            const int player = m_game.turn();
            if (player == Human || !m_online->isBot(m_online->toTable(player)))
                return;
            m_online->send({{QStringLiteral("t"), QStringLiteral("play")},
                            {QStringLiteral("seat"), m_online->toTable(player)},
                            {QStringLiteral("card"), m_bot.choosePlay(m_game, player).id()}});
        }
        return;
    }
    if (m_game.trickComplete()) {
        perform({{QStringLiteral("t"), QStringLiteral("collect")}});
    } else {
        const int player = m_game.turn();
        if (player == Human)
            return;
        perform({{QStringLiteral("t"), QStringLiteral("play")},
                 {QStringLiteral("seat"), player},
                 {QStringLiteral("card"), m_bot.choosePlay(m_game, player).id()}});
    }
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

QVariantList GameController::dealHands()
{
    Cards deck = fullDeck();
    std::shuffle(deck.begin(), deck.end(), m_rng);
    QVariantList hands;
    for (int seat = 0; seat < Game::Players; ++seat) {
        QVariantList hand;
        for (int i = seat; i < static_cast<int>(deck.size()); i += Game::Players)
            hand << deck[i].id();
        hands << QVariant(hand);
    }
    return hands;
}

// Card i goes to player i % 4, so your hand is dealt to seat 0.
Cards GameController::localDeck(const QVariantList &hands) const
{
    if (hands.size() != Game::Players)
        return Cards();
    Cards deck;
    for (int i = 0; i < Game::HandSize; ++i) {
        for (int p = 0; p < Game::Players; ++p) {
            const QVariantList hand = hands.at(m_online->toTable(p)).toList();
            if (hand.size() != Game::HandSize)
                return Cards();
            const int id = hand.at(i).toInt();
            if (id < 0 || id >= 52)
                return Cards();
            deck.push_back(Card::fromId(id));
        }
    }
    return deck;
}

void GameController::sendNew()
{
    m_online->send(newAction());
}

QVariantMap GameController::newAction()
{
    return {{QStringLiteral("t"), QStringLiteral("new")},
            {QStringLiteral("target"), m_target},
            {QStringLiteral("difficulty"), m_difficulty},
            {QStringLiteral("hands"), dealHands()}};
}

bool GameController::perform(const QVariantMap &action)
{
    if (online()) {
        m_online->send(action);
        return true;
    }
    if (!apply(action))
        return false;
    m_log.append(action);
    afterAction();
    return true;
}

bool GameController::apply(const QVariantMap &action)
{
    const QString t = action.value(QStringLiteral("t")).toString();
    if (t == QLatin1String("new")) {
        if (m_started && m_game.phase() != hearts::Phase::GameOver)
            return false;
        const Cards deck = localDeck(action.value(QStringLiteral("hands")).toList());
        if (deck.empty())
            return false;
        m_botTimer.stop();
        m_receivedTimer.stop();
        m_bot.setDifficulty(static_cast<Difficulty>(qBound(0, action.value(QStringLiteral("difficulty"), m_difficulty).toInt(), 2)));
        m_game.setTarget(action.value(QStringLiteral("target")).toInt() == Targets[0] ? Targets[0] : Targets[1]);
        m_game.newGame(deck);
        m_started = true;
        // Online games stay out of the statistics.
        m_recorded = online();
        m_scoredRound = 0;
        m_received.clear();
        m_botPassRound = -1;
        return true;
    }
    if (!m_started)
        return false;
    if (t == QLatin1String("pass")) {
        const int player = m_online->toLocal(action.value(QStringLiteral("seat")).toInt());
        Cards cards;
        if (player < 0 || !cardsFrom(action.value(QStringLiteral("cards")), &cards) || !m_game.pass(player, cards))
            return false;
        // The last pass: the cards have changed hands.
        if (m_game.phase() == hearts::Phase::Play && !m_replaying) {
            m_received = m_game.passed(m_game.passSource(Human));
            m_receivedTimer.start();
        }
        return true;
    }
    if (t == QLatin1String("play")) {
        const int player = m_online->toLocal(action.value(QStringLiteral("seat")).toInt());
        if (player < 0 || !m_game.play(player, Card::fromId(action.value(QStringLiteral("card")).toInt())))
            return false;
        if (player == Human)
            m_received.clear();
        return true;
    }
    if (t == QLatin1String("collect"))
        return m_game.collectTrick();
    if (t == QLatin1String("next")) {
        if (m_game.phase() != hearts::Phase::Score)
            return false;
        const Cards deck = localDeck(action.value(QStringLiteral("hands")).toList());
        if (deck.empty())
            return false;
        m_receivedTimer.stop();
        m_received.clear();
        m_game.dealFrom(deck);
        return true;
    }
    return false;
}

void GameController::replay()
{
    if (!online())
        return;
    m_botTimer.stop();
    m_receivedTimer.stop();
    m_game = hearts::Game();
    m_started = false;
    m_received.clear();
    m_botPassRound = -1;
    m_replaying = true;
    for (const QVariant &action : m_online->actions())
        apply(action.toMap());
    m_replaying = false;
    if (m_online->actions().isEmpty() && m_online->driver())
        sendNew();
    afterAction();
}

void GameController::onTableChanged()
{
    if (!online()) {
        // Left the table: back to your game against the bots.
        if (m_wasOnline) {
            m_wasOnline = false;
            start();
        }
        return;
    }
    if (!m_wasOnline) {
        // Nothing of the game against the bots carries over; the log follows.
        m_wasOnline = true;
        m_botTimer.stop();
        m_receivedTimer.stop();
        m_game = hearts::Game();
        m_started = false;
        m_received.clear();
    }
    // Someone may have come or gone, or a bot taken over.
    m_botPassRound = -1;
    if (m_online->synced() && m_online->actions().isEmpty() && m_online->driver())
        sendNew();
    afterAction();
}
