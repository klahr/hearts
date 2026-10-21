#include "game.h"

#include <algorithm>

namespace hearts {

Game::Game(unsigned seed)
    : m_rng(seed)
{
}

void Game::newGame()
{
    Cards deck = fullDeck();
    std::shuffle(deck.begin(), deck.end(), m_rng);
    newGame(deck);
}

void Game::newGame(const Cards &deck)
{
    for (int p = 0; p < Players; ++p) {
        m_score[p] = 0;
        m_previous[p] = 0;
    }
    m_winner = -1;
    m_round = 0;
    dealFrom(deck);
}

void Game::dealFrom(const Cards &deck)
{
    ++m_round;
    for (int p = 0; p < Players; ++p) {
        m_hand[p].clear();
        m_passed[p].clear();
        m_hasPassed[p] = false;
        m_taken[p].clear();
        m_handScore[p] = 0;
    }
    m_trick.clear();
    m_played.clear();
    m_moonShooter = -1;
    m_turn = -1;
    for (int i = 0; i < Players * HandSize; ++i)
        m_hand[i % Players].push_back(deck[i]);
    for (Cards &hand : m_hand)
        sortAlternating(hand);
    if (passDirection() == PassDirection::Hold)
        startPlay();
    else
        m_phase = Phase::Pass;
}

void Game::setScores(const std::vector<int> &scores)
{
    for (int p = 0; p < Players; ++p)
        m_score[p] = m_previous[p] = scores[p];
}

int Game::passTarget(int player) const
{
    switch (passDirection()) {
    case PassDirection::Left:
        return (player + 1) % Players;
    case PassDirection::Right:
        return (player + 3) % Players;
    case PassDirection::Across:
        return (player + 2) % Players;
    case PassDirection::Hold:
        break;
    }
    return -1;
}

int Game::passSource(int player) const
{
    for (int p = 0; p < Players; ++p)
        if (p != player && passTarget(p) == player)
            return p;
    return -1;
}

bool Game::pass(int player, const Cards &cards)
{
    if (m_phase != Phase::Pass || m_hasPassed[player] || cards.size() != 3)
        return false;
    for (size_t i = 0; i < cards.size(); ++i) {
        if (!holds(player, cards[i]))
            return false;
        for (size_t j = 0; j < i; ++j)
            if (cards[i] == cards[j])
                return false;
    }
    for (const Card &c : cards)
        removeCard(m_hand[player], c);
    m_passed[player] = cards;
    m_hasPassed[player] = true;

    for (int p = 0; p < Players; ++p)
        if (!m_hasPassed[p])
            return true;
    for (int p = 0; p < Players; ++p) {
        Cards &target = m_hand[passTarget(p)];
        target.insert(target.end(), m_passed[p].begin(), m_passed[p].end());
    }
    for (Cards &hand : m_hand)
        sortAlternating(hand);
    startPlay();
    return true;
}

void Game::startPlay()
{
    m_phase = Phase::Play;
    m_tricks = 0;
    m_heartsBroken = false;
    m_trick.clear();
    m_played.clear();
    for (int p = 0; p < Players; ++p)
        if (holds(p, TwoOfClubs))
            m_leader = p;
    m_turn = m_leader;
}

bool Game::holds(int player, const Card &card) const
{
    return contains(m_hand[player], card);
}

bool Game::holdsSuit(int player, int suit) const
{
    for (const Card &c : m_hand[player])
        if (c.suit == suit)
            return true;
    return false;
}

bool Game::canPlayCard(int player, const Card &card) const
{
    if (m_phase != Phase::Play || m_turn != player || !holds(player, card))
        return false;
    const bool firstTrick = m_tricks == 0;
    if (m_trick.empty()) {
        if (firstTrick)
            return card == TwoOfClubs;
        // Hearts are led once broken, or when nothing else is left.
        if (card.suit == Hearts && !m_heartsBroken)
            return !holdsSuit(player, Clubs) && !holdsSuit(player, Diamonds) && !holdsSuit(player, Spades);
        return true;
    }
    if (holdsSuit(player, ledSuit()))
        return card.suit == ledSuit();
    // No points on the first trick, unless the hand holds nothing else.
    if (firstTrick && points(card) > 0) {
        for (const Card &c : m_hand[player])
            if (points(c) == 0)
                return false;
    }
    return true;
}

Cards Game::legalCards(int player) const
{
    Cards out;
    for (const Card &c : m_hand[player])
        if (canPlayCard(player, c))
            out.push_back(c);
    return out;
}

bool Game::play(int player, const Card &card)
{
    if (!canPlayCard(player, card))
        return false;
    removeCard(m_hand[player], card);
    m_trick.push_back({player, card});
    m_played.push_back({player, card});
    if (card.suit == Hearts)
        m_heartsBroken = true;
    m_turn = trickComplete() ? -1 : (player + 1) % Players;
    return true;
}

int Game::trickWinner() const
{
    if (!trickComplete())
        return -1;
    const Play *best = &m_trick.front();
    for (const Play &p : m_trick)
        if (p.card.suit == best->card.suit && p.card.high() > best->card.high())
            best = &p;
    return best->player;
}

bool Game::collectTrick()
{
    if (m_phase != Phase::Play || !trickComplete())
        return false;
    const int winner = trickWinner();
    for (const Play &p : m_trick)
        m_taken[winner].push_back(p.card);
    m_trick.clear();
    ++m_tricks;
    m_leader = winner;
    m_turn = winner;
    if (m_tricks == HandSize)
        scoreHand();
    return true;
}

void Game::scoreHand()
{
    m_turn = -1;
    m_moonShooter = -1;
    for (int p = 0; p < Players; ++p) {
        m_handScore[p] = handPoints(p);
        if (m_handScore[p] == MoonPoints)
            m_moonShooter = p;
    }
    // Shooting the moon: all the points to everybody else instead.
    if (m_moonShooter >= 0)
        for (int p = 0; p < Players; ++p)
            m_handScore[p] = p == m_moonShooter ? 0 : MoonPoints;

    int high = 0;
    for (int p = 0; p < Players; ++p) {
        m_previous[p] = m_score[p];
        m_score[p] += m_handScore[p];
        high = std::max(high, m_score[p]);
    }
    m_phase = Phase::Score;
    if (high < m_target)
        return;
    // The lowest score wins; on a tie for lowest, play another hand.
    const int low = *std::min_element(m_score, m_score + Players);
    if (std::count(m_score, m_score + Players, low) == 1) {
        m_winner = static_cast<int>(std::min_element(m_score, m_score + Players) - m_score);
        m_phase = Phase::GameOver;
    }
}

bool Game::nextHand()
{
    if (m_phase != Phase::Score)
        return false;
    Cards deck = fullDeck();
    std::shuffle(deck.begin(), deck.end(), m_rng);
    dealFrom(deck);
    return true;
}

} // namespace hearts
