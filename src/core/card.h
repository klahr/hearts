#pragma once

#include "cards/card.h"

namespace hearts {

using cards::Card;
using cards::Cards;
using cards::Suit;
using cards::Clubs;
using cards::Diamonds;
using cards::Hearts;
using cards::Spades;
using cards::parseCard;
using cards::parseCards;
using cards::fullDeck;

inline const Card TwoOfClubs(2, Clubs);
inline const Card QueenOfSpades(12, Spades);

// A heart is one point, the queen of spades thirteen.
inline int points(const Card &c) { return c.suit == Hearts ? 1 : (c == QueenOfSpades ? 13 : 0); }

int countPoints(const Cards &cards);

// Clubs, diamonds, spades, hearts, each from low to high, as a hand is held.
void sortHand(Cards &cards);

} // namespace hearts
