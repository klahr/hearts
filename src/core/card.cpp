#include "card.h"

namespace hearts {

int countPoints(const Cards &cards)
{
    int total = 0;
    for (const Card &c : cards)
        total += points(c);
    return total;
}

} // namespace hearts
