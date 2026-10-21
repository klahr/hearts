#pragma once

#include "core/bot.h"
#include "core/game.h"
#include "cards/gamelog.h"
#include "online/session.h"

#include <QObject>
#include <QSettings>
#include <QTimer>
#include <QVariantList>

#include <random>

class GameController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int phase READ phase NOTIFY changed)
    Q_PROPERTY(int round READ round NOTIFY changed)
    Q_PROPERTY(int passDirection READ passDirection NOTIFY changed)
    Q_PROPERTY(int passTarget READ passTarget NOTIFY changed)
    Q_PROPERTY(int passSource READ passSource NOTIFY changed)
    Q_PROPERTY(bool humanPassed READ humanPassed NOTIFY changed)
    Q_PROPERTY(QVariantList passed READ passed NOTIFY changed)
    Q_PROPERTY(QVariantList hands READ hands NOTIFY changed)
    Q_PROPERTY(QVariantList received READ received NOTIFY changed)
    Q_PROPERTY(QVariantList trick READ trick NOTIFY changed)
    Q_PROPERTY(QVariantList lastTrick READ lastTrick NOTIFY changed)
    Q_PROPERTY(int trickWinner READ trickWinner NOTIFY changed)
    Q_PROPERTY(int tricksPlayed READ tricksPlayed NOTIFY changed)
    Q_PROPERTY(QVariantList taken READ taken NOTIFY changed)
    Q_PROPERTY(int turn READ turn NOTIFY changed)
    Q_PROPERTY(bool humanTurn READ humanTurn NOTIFY changed)
    Q_PROPERTY(bool heartsBroken READ heartsBroken NOTIFY changed)
    Q_PROPERTY(QVariantList scores READ scores NOTIFY changed)
    Q_PROPERTY(QVariantList handPoints READ handPoints NOTIFY changed)
    Q_PROPERTY(QVariantList handScores READ handScores NOTIFY changed)
    Q_PROPERTY(int moonShooter READ moonShooter NOTIFY changed)
    Q_PROPERTY(int winner READ winner NOTIFY changed)
    Q_PROPERTY(int gameTarget READ gameTarget NOTIFY changed)
    Q_PROPERTY(bool started READ started NOTIFY changed)
    Q_PROPERTY(int gameDifficulty READ gameDifficulty NOTIFY changed)
    Q_PROPERTY(int difficulty READ difficulty WRITE setDifficulty NOTIFY difficultyChanged)
    Q_PROPERTY(int target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(QVariantList stats READ stats NOTIFY statsChanged)
    Q_PROPERTY(int speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(bool tapTwice READ tapTwice WRITE setTapTwice NOTIFY tapTwiceChanged)

public:
    enum Phase { Pass, Play, Score, GameOver };
    Q_ENUM(Phase)

    explicit GameController(Session *online, QObject *parent = nullptr);

    int phase() const;
    int round() const { return m_game.round(); }
    // 0 = left, 1 = right, 2 = across, 3 = no passing.
    int passDirection() const { return static_cast<int>(m_game.passDirection()); }
    int passTarget() const { return m_game.passTarget(Human); }
    int passSource() const { return m_game.passSource(Human); }
    bool humanPassed() const { return m_game.hasPassed(Human); }
    // The cards each player has laid aside to pass, until they change hands.
    QVariantList passed() const;
    // Every player's cards, yours sorted; ids let the table animate each
    // card from where it lies.
    QVariantList hands() const;
    // The cards passed to you, until you play your first card.
    QVariantList received() const;
    // The cards on the table, each with who played it.
    QVariantList trick() const;
    // The trick gathered before, to look back at.
    QVariantList lastTrick() const;
    int trickWinner() const { return m_game.trickWinner(); }
    int tricksPlayed() const { return m_game.tricksPlayed(); }
    QVariantList taken() const;
    int turn() const { return m_game.turn(); }
    bool humanTurn() const;
    bool heartsBroken() const { return m_game.heartsBroken(); }
    QVariantList scores() const;
    QVariantList handPoints() const;
    QVariantList handScores() const;
    int moonShooter() const { return m_game.moonShooter(); }
    int winner() const { return m_game.winner(); }
    int gameTarget() const { return m_game.target(); }
    bool started() const { return m_started; }
    int gameDifficulty() const { return static_cast<int>(m_bot.difficulty()); }
    int difficulty() const { return m_difficulty; }
    void setDifficulty(int difficulty);
    // The score that ends the game: 50 or 100.
    int target() const { return m_target; }
    void setTarget(int target);
    QVariantList stats() const;
    // 0 = slow ... 3 = very fast; how long the bots think and tricks stay.
    int speed() const { return m_speed; }
    void setSpeed(int speed);
    // A card is played by a second tap; the first raises it.
    bool tapTwice() const { return m_tapTwice; }
    void setTapTwice(bool on);

    // Carries on the game against the bots from when the app last closed,
    // or deals a new one.
    Q_INVOKABLE void start();
    Q_INVOKABLE void newGame();
    Q_INVOKABLE bool pass(const QVariantList &cardIds);
    Q_INVOKABLE bool play(int cardId);
    Q_INVOKABLE bool nextHand();
    Q_INVOKABLE bool canPlay(int cardId) const;
    Q_INVOKABLE void resetStats();

    Q_INVOKABLE QString playerName(int player) const;

signals:
    void changed();
    void difficultyChanged();
    void targetChanged();
    void statsChanged();
    void speedChanged();
    void tapTwiceChanged();

private:
    static constexpr int Human = 0;

    void afterAction();
    void botStep();
    void recordResult();

    // Online, every move is one of the table's log, with its seats; the
    // session turns them round so that you sit at seat 0 here.
    bool online() const { return m_online->active(); }
    // Makes a move: online it goes to the table's log and comes back; against
    // the bots it is made at once and written down.
    bool perform(const QVariantMap &action);
    // Applies a move from the log; one the rules refuse is dropped, the same
    // way on every player's phone.
    bool apply(const QVariantMap &action);
    // Plays the whole log from the start, as on joining or coming back.
    void replay();
    void onTableChanged();
    // A shuffled deal, each table seat's hand.
    QVariantList dealHands();
    hearts::Cards localDeck(const QVariantList &hands) const;
    void sendNew();
    // A new game with the settings, as a move.
    QVariantMap newAction();

    Session *m_online;
    // The moves of the game against the bots.
    cards::GameLog m_log;
    hearts::Game m_game;
    // Plays all three other seats; it only knows what the seat may see.
    hearts::Bot m_bot;
    QSettings m_settings;
    QTimer m_botTimer;
    // Holds the play while the cards passed to you are shown.
    QTimer m_receivedTimer;
    hearts::Cards m_received;
    int m_difficulty = 1;
    int m_target = 100;
    int m_speed = 1;
    bool m_tapTwice = false;
    bool m_started = false;
    bool m_recorded = false;
    int m_scoredRound = 0; // the last hand counted for the statistics
    std::mt19937 m_rng{std::random_device{}()};
    bool m_wasOnline = false;
    bool m_replaying = false;
    // The hand the bots' passes were sent for, by the app that runs them.
    int m_botPassRound = -1;
};
