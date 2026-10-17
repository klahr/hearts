#pragma once

#include "game.h"

#include <random>

namespace hearts {

enum class Difficulty { Easy = 0, Medium = 1, Hard = 2 };

class Bot {
public:
    explicit Bot(Difficulty difficulty = Difficulty::Medium, unsigned seed = std::random_device{}());

    void setDifficulty(Difficulty d) { m_difficulty = d; }
    Difficulty difficulty() const { return m_difficulty; }

    // Three cards to pass from the player's hand.
    Cards choosePass(const Game &game, int player);
    // A legal card to play; only valid on the player's turn.
    Card choosePlay(const Game &game, int player);

    // The card a careful player would play: duck under the trick, throw the
    // queen and high cards away when void, lead low. Also plays out every
    // hand the hard bot tries.
    static Card cautiousPlay(const Game &game, int player);

private:
    Cards passRandom(const Cards &hand);
    Cards passDangerous(const Cards &hand);
    Card playSimulated(const Game &game, int player, const Cards &legal);
    // Deals the cards `player` cannot see to the others, consistent with
    // the suits they have shown out of and the cards passed to them.
    bool guessHands(const Game &game, int player, Cards hands[Game::Players]);

    Difficulty m_difficulty;
    std::mt19937 m_rng;
};

} // namespace hearts
