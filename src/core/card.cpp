#include "card.h"

#include <algorithm>

namespace hearts {

int countPoints(const Cards &cards)
{
    int total = 0;
    for (const Card &c : cards)
        total += points(c);
    return total;
}

void sortHand(Cards &cards)
{
    // Black and red suits alternate.
    static const int order[] = {0, 1, 3, 2};
    std::sort(cards.begin(), cards.end(), [](const Card &a, const Card &b) {
        if (a.suit != b.suit)
            return order[a.suit] < order[b.suit];
        return a.high() < b.high();
    });
}

} // namespace hearts
