#pragma once

#include "card.h"

#include <random>

namespace hearts {

enum class Phase {
    Pass,     // everyone passes three cards
    Play,     // thirteen tricks
    Score,    // the hand is over and counted
    GameOver,
};

// Cycles with every hand, starting with the first.
enum class PassDirection { Left = 0, Right = 1, Across = 2, Hold = 3 };

struct Play {
    int player;
    Card card;
};

// Four players sit round the table in playing order: 0 is south (you),
// 1 west, 2 north and 3 east. Play and passing to the left go clockwise,
// to the next number.
class Game {
public:
    static constexpr int Players = 4;
    static constexpr int HandSize = 13;
    static constexpr int MoonPoints = 26;

    explicit Game(unsigned seed = std::random_device{}());

    // The game ends after the hand in which someone reaches this score.
    void setTarget(int points) { m_target = points; }
    int target() const { return m_target; }

    // Clears the scores and deals the first hand.
    void newGame();
    // As above, dealing from `deck` instead of a shuffled pack; used by tests.
    void newGame(const Cards &deck);
    // Deals the next hand from `deck`, card i to player i % 4.
    void dealFrom(const Cards &deck);
    // Restores scores, e.g. in tests.
    void setScores(const std::vector<int> &scores);

    // Lays aside three cards; once all four have passed they change hands.
    bool pass(int player, const Cards &cards);
    bool play(int player, const Card &card);
    // Gives a finished trick to its winner, who leads the next.
    bool collectTrick();
    // After the hand is counted, deals the next one.
    bool nextHand();

    bool canPlayCard(int player, const Card &card) const;
    Cards legalCards(int player) const;

    Phase phase() const { return m_phase; }
    int round() const { return m_round; }
    PassDirection passDirection() const { return static_cast<PassDirection>((m_round + 3) % 4); }
    // Who `player` passes to, or receives from; -1 on a hand without passing.
    int passTarget(int player) const;
    int passSource(int player) const;
    bool hasPassed(int player) const { return m_hasPassed[player]; }
    const Cards &passed(int player) const { return m_passed[player]; }

    const Cards &hand(int player) const { return m_hand[player]; }
    const std::vector<Play> &trick() const { return m_trick; }
    // Every card played this hand, in order, the current trick included.
    const std::vector<Play> &played() const { return m_played; }
    // The suit of the trick's first card, -1 before it is led.
    int ledSuit() const { return m_trick.empty() ? -1 : m_trick.front().card.suit; }
    int leader() const { return m_leader; }
    // Whose turn it is; -1 while a finished trick is still on the table.
    int turn() const { return m_turn; }
    bool trickComplete() const { return m_trick.size() == Players; }
    int trickWinner() const;
    int tricksPlayed() const { return m_tricks; }
    bool heartsBroken() const { return m_heartsBroken; }

    const Cards &taken(int player) const { return m_taken[player]; }
    // Points taken so far this hand.
    int handPoints(int player) const { return countPoints(m_taken[player]); }
    // What the finished hand added to each score, after a shot at the moon.
    int handScore(int player) const { return m_handScore[player]; }
    int moonShooter() const { return m_moonShooter; }
    int score(int player) const { return m_score[player]; }
    int previousScore(int player) const { return m_previous[player]; }
    int winner() const { return m_winner; }

    // Lets the bot try out guessed hands on a copy of the game.
    void replaceHand(int player, const Cards &cards) { m_hand[player] = cards; }

private:
    void startPlay();
    void scoreHand();
    bool holds(int player, const Card &card) const;
    bool holdsSuit(int player, int suit) const;

    std::mt19937 m_rng;
    int m_target = 100;
    Phase m_phase = Phase::Pass;
    int m_round = 0;
    Cards m_hand[Players];
    Cards m_passed[Players];
    bool m_hasPassed[Players] = {};
    std::vector<Play> m_trick;
    std::vector<Play> m_played;
    int m_leader = 0;
    int m_turn = -1;
    int m_tricks = 0;
    bool m_heartsBroken = false;
    Cards m_taken[Players];
    int m_handScore[Players] = {};
    int m_moonShooter = -1;
    int m_score[Players] = {};
    int m_previous[Players] = {};
    int m_winner = -1;
};

} // namespace hearts
