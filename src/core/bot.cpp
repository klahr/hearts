#include "bot.h"

#include <algorithm>

namespace hearts {

namespace {

const int HardSamples = 24;

bool wasPlayed(const Game &game, const Card &card)
{
    for (const Play &p : game.played())
        if (p.card == card)
            return true;
    return false;
}

// How much a card is worth getting rid of when passing.
int passDanger(const Card &card, const Cards &hand)
{
    const bool queen = contains(hand, QueenOfSpades);
    int guards = 0; // spades below the queen
    for (const Card &c : hand)
        if (c.suit == Spades && c.high() < 12)
            ++guards;
    if (card == QueenOfSpades)
        return guards >= 4 ? 5 : 100;
    if (card.suit == Spades && card.high() > 12) {
        if (queen)
            return guards >= 4 ? 10 : 90;
        return guards >= 3 ? 15 : 80;
    }
    if (card.suit == Spades)
        return card.high();
    int danger = card.suit == Hearts ? card.high() * 3 - 15 : card.high() * 2;
    // Short suits are worth emptying, to throw points away later.
    const int n = countSuit(hand, card.suit);
    if (card.suit != Hearts && n <= 3)
        danger += (4 - n) * 6;
    return danger;
}

// The highest card, but not the queen of spades if there is anything else.
Card highestSafe(const Cards &cards)
{
    const Card *best = nullptr;
    for (const Card &c : cards)
        if (c != QueenOfSpades && (!best || c.high() > best->high()))
            best = &c;
    return best ? *best : cards.front();
}

Card lowestSafe(const Cards &cards)
{
    const Card *best = nullptr;
    for (const Card &c : cards)
        if (c != QueenOfSpades && (!best || c.high() < best->high()))
            best = &c;
    return best ? *best : cards.front();
}

} // namespace

Bot::Bot(Difficulty difficulty, unsigned seed)
    : m_difficulty(difficulty)
    , m_rng(seed)
{
}

Cards Bot::choosePass(const Game &game, int player)
{
    if (m_difficulty == Difficulty::Easy)
        return passRandom(game.hand(player));
    return passDangerous(game.hand(player));
}

Cards Bot::passRandom(const Cards &hand)
{
    Cards shuffled = hand;
    std::shuffle(shuffled.begin(), shuffled.end(), m_rng);
    return Cards(shuffled.begin(), shuffled.begin() + 3);
}

Cards Bot::passDangerous(const Cards &hand)
{
    Cards left = hand;
    Cards out;
    while (out.size() < 3) {
        auto worst = std::max_element(left.begin(), left.end(), [&left](const Card &a, const Card &b) {
            return passDanger(a, left) < passDanger(b, left);
        });
        out.push_back(*worst);
        left.erase(worst);
    }
    return out;
}

Card Bot::choosePlay(const Game &game, int player)
{
    const Cards legal = game.legalCards(player);
    if (legal.size() == 1)
        return legal.front();
    switch (m_difficulty) {
    case Difficulty::Easy:
        return legal[std::uniform_int_distribution<size_t>(0, legal.size() - 1)(m_rng)];
    case Difficulty::Medium:
        return cautiousPlay(game, player);
    case Difficulty::Hard:
        break;
    }
    return playSimulated(game, player, legal);
}

Card Bot::cautiousPlay(const Game &game, int player)
{
    const Cards legal = game.legalCards(player);
    if (legal.size() == 1)
        return legal.front();
    const Cards &hand = game.hand(player);
    const std::vector<Play> &trick = game.trick();
    const bool queenOut = !contains(hand, QueenOfSpades) && !wasPlayed(game, QueenOfSpades);

    if (trick.empty()) {
        // Lead low, and draw the queen out with low spades while it is
        // still at large; never lead into it with the ace or king.
        const Card *best = nullptr;
        int bestCost = 0;
        for (const Card &c : legal) {
            int cost = c.high();
            if (c == QueenOfSpades)
                cost += 30;
            else if (c.suit == Spades && queenOut)
                cost += c.high() > 12 ? 20 : -3;
            else if (c.suit == Hearts)
                cost += 2;
            if (!best || cost < bestCost) {
                best = &c;
                bestCost = cost;
            }
        }
        return *best;
    }

    const int led = game.ledSuit();
    if (legal.front().suit == led) {
        if (game.tricksPlayed() == 0)
            return highestSafe(legal); // nothing to lose on the first trick
        int winning = 0;
        for (const Play &p : trick)
            if (p.card.suit == led)
                winning = std::max(winning, p.card.high());
        Cards under;
        for (const Card &c : legal)
            if (c.high() < winning)
                under.push_back(c);
        if (!under.empty()) {
            // Duck with the highest card that still loses; this also drops
            // the queen under the ace or king.
            return *std::max_element(under.begin(), under.end(),
                                     [](const Card &a, const Card &b) { return a.high() < b.high(); });
        }
        // Last to play takes the trick anyway, so get rid of a high card.
        if (trick.size() == Game::Players - 1)
            return highestSafe(legal);
        return lowestSafe(legal);
    }

    // Void in the suit led: throw away the worst card.
    const Card *best = nullptr;
    int bestValue = 0;
    for (const Card &c : legal) {
        int value = c.high();
        if (c == QueenOfSpades)
            value += 100;
        else if (c.suit == Spades && c.high() > 12 && queenOut)
            value += 50;
        else if (c.suit == Hearts)
            value += 10;
        else
            value += (13 - countSuit(hand, c.suit)) / 3;
        if (!best || value > bestValue) {
            best = &c;
            bestValue = value;
        }
    }
    return *best;
}

bool Bot::guessHands(const Game &game, int player, Cards hands[Game::Players])
{
    bool shownOut[Game::Players][4] = {};
    const std::vector<Play> &played = game.played();
    for (size_t i = 0; i < played.size(); ++i) {
        const int led = played[i - i % Game::Players].card.suit;
        if (played[i].card.suit != led)
            shownOut[played[i].player][led] = true;
    }

    int need[Game::Players];
    for (int p = 0; p < Game::Players; ++p) {
        hands[p].clear();
        need[p] = p == player ? 0 : static_cast<int>(game.hand(p).size());
    }
    hands[player] = game.hand(player);

    // The cards passed on are still where they went, unless played since.
    const int target = game.passTarget(player);
    if (target >= 0) {
        for (const Card &c : game.passed(player)) {
            if (!wasPlayed(game, c)) {
                hands[target].push_back(c);
                --need[target];
            }
        }
    }

    Cards unseen;
    for (const Card &c : fullDeck())
        if (!contains(hands[player], c) && !wasPlayed(game, c) && (target < 0 || !contains(hands[target], c)))
            unseen.push_back(c);

    for (int attempt = 0; attempt < 20; ++attempt) {
        const bool strict = attempt < 19;
        Cards deal[Game::Players];
        int left[Game::Players];
        std::copy(need, need + Game::Players, left);
        Cards order = unseen;
        std::shuffle(order.begin(), order.end(), m_rng);
        auto open = [&](int p, const Card &c) { return left[p] > 0 && !(strict && shownOut[p][c.suit]); };
        auto choices = [&](const Card &c) {
            int n = 0;
            for (int p = 0; p < Game::Players; ++p)
                n += open(p, c);
            return n;
        };
        // The most constrained cards first.
        std::stable_sort(order.begin(), order.end(),
                         [&](const Card &a, const Card &b) { return choices(a) < choices(b); });
        bool ok = true;
        for (const Card &c : order) {
            int total = 0;
            for (int p = 0; p < Game::Players; ++p)
                if (open(p, c))
                    total += left[p];
            if (total == 0) {
                ok = false;
                break;
            }
            int pick = std::uniform_int_distribution<int>(0, total - 1)(m_rng);
            for (int p = 0; p < Game::Players; ++p) {
                if (!open(p, c))
                    continue;
                if (pick < left[p]) {
                    deal[p].push_back(c);
                    --left[p];
                    break;
                }
                pick -= left[p];
            }
        }
        if (!ok)
            continue;
        for (int p = 0; p < Game::Players; ++p)
            hands[p].insert(hands[p].end(), deal[p].begin(), deal[p].end());
        return true;
    }
    return false;
}

Card Bot::playSimulated(const Game &game, int player, const Cards &legal)
{
    // Plays each card out over a number of guessed deals, everyone playing
    // carefully from there, and keeps the one that does best against the
    // others on average.
    std::vector<double> cost(legal.size(), 0.0);
    Cards hands[Game::Players];
    for (int s = 0; s < HardSamples; ++s) {
        if (!guessHands(game, player, hands))
            return cautiousPlay(game, player);
        for (size_t i = 0; i < legal.size(); ++i) {
            Game sim = game;
            for (int p = 0; p < Game::Players; ++p)
                if (p != player)
                    sim.replaceHand(p, hands[p]);
            sim.play(player, legal[i]);
            while (sim.phase() == Phase::Play) {
                if (sim.trickComplete())
                    sim.collectTrick();
                else
                    sim.play(sim.turn(), cautiousPlay(sim, sim.turn()));
            }
            int others = 0;
            for (int p = 0; p < Game::Players; ++p)
                if (p != player)
                    others += sim.handScore(p);
            cost[i] += sim.handScore(player) - others / double(Game::Players - 1);
        }
    }
    return legal[std::min_element(cost.begin(), cost.end()) - cost.begin()];
}

} // namespace hearts
