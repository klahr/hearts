// Rule tests for the hearts engine. Plain C++ so they build anywhere:
//   make -C tests

#include "../src/core/bot.h"
#include "../src/core/game.h"
#include "cards/testing.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

using namespace hearts;

namespace {

bool holds(const Game &g, int player, const std::string &card)
{
    const Cards &h = g.hand(player);
    return std::find(h.begin(), h.end(), parseCard(card)) != h.end();
}

// A deck that deals each player the cards given for them; the rest of the
// pack fills up the hands in order.
Cards stackedDeck(const std::vector<std::string> &given)
{
    Cards hands[Game::Players];
    Cards used;
    for (size_t p = 0; p < given.size(); ++p) {
        hands[p] = parseCards(given[p]);
        used.insert(used.end(), hands[p].begin(), hands[p].end());
    }
    int p = 0;
    for (const Card &c : fullDeck()) {
        if (std::find(used.begin(), used.end(), c) != used.end())
            continue;
        while (hands[p].size() == Game::HandSize)
            ++p;
        hands[p].push_back(c);
    }
    Cards deck;
    for (int i = 0; i < Game::HandSize; ++i)
        for (int q = 0; q < Game::Players; ++q)
            deck.push_back(hands[q][i]);
    return deck;
}

// A game on its fourth hand, where nobody passes, dealt from `deck`.
Game holding(const Cards &deck)
{
    Game g(1);
    g.newGame(deck);
    g.dealFrom(deck);
    g.dealFrom(deck);
    g.dealFrom(deck);
    return g;
}

bool play(Game &g, int player, const std::string &card)
{
    return g.play(player, parseCard(card));
}

// Plays a whole trick, starting with whoever's turn it is.
void trick(Game &g, const std::string &cards)
{
    for (const Card &c : parseCards(cards))
        CHECK(g.play(g.turn(), c));
    CHECK(g.collectTrick());
}

TEST(cards_have_points_and_aces_high)
{
    CHECK_EQ(points(parseCard("QS")), 13);
    CHECK_EQ(points(parseCard("2H")), 1);
    CHECK_EQ(points(parseCard("AH")), 1);
    CHECK_EQ(points(parseCard("KS")), 0);
    CHECK_EQ(points(parseCard("QC")), 0);
    CHECK_EQ(parseCard("AD").high(), 14);
    CHECK_EQ(parseCard("KD").high(), 13);
    CHECK_EQ(countPoints(fullDeck()), 26);

    Cards hand = parseCards("AH 2H KS QD 2C AC 3S");
    sortAlternating(hand);
    CHECK(hand == parseCards("2C AC QD 3S KS 2H AH"));
}

TEST(passing_cycles_left_right_across_hold)
{
    Game g(1);
    g.newGame();
    CHECK(g.phase() == Phase::Pass);
    CHECK(g.passDirection() == PassDirection::Left);
    CHECK_EQ(g.passTarget(0), 1);
    CHECK_EQ(g.passTarget(3), 0);
    CHECK_EQ(g.passSource(0), 3);
    const Cards deck = fullDeck();
    g.dealFrom(deck);
    CHECK(g.passDirection() == PassDirection::Right);
    CHECK_EQ(g.passTarget(0), 3);
    CHECK_EQ(g.passSource(0), 1);
    g.dealFrom(deck);
    CHECK(g.passDirection() == PassDirection::Across);
    CHECK_EQ(g.passTarget(1), 3);
    g.dealFrom(deck);
    CHECK(g.passDirection() == PassDirection::Hold);
    CHECK_EQ(g.passTarget(0), -1);
    CHECK(g.phase() == Phase::Play);
    g.dealFrom(deck);
    CHECK(g.passDirection() == PassDirection::Left);
}

TEST(passing_exchanges_three_cards)
{
    Game g(1);
    g.newGame(stackedDeck({"2C 3C 4C", "5D 6D 7D", "8H 9H 10H", "JS QS KS"}));
    CHECK(!g.pass(0, parseCards("2C 3C")));          // too few
    CHECK(!g.pass(0, parseCards("2C 3C 3C")));       // the same card twice
    CHECK(!g.pass(0, parseCards("2C 3C 5D")));       // not in the hand
    CHECK(g.pass(0, parseCards("2C 3C 4C")));
    CHECK(!g.pass(0, parseCards("5C 6C 7C")));       // only once
    CHECK(g.phase() == Phase::Pass);
    CHECK_EQ(g.hand(0).size(), 10u);
    CHECK(g.pass(1, parseCards("5D 6D 7D")));
    CHECK(g.pass(2, parseCards("8H 9H 10H")));
    CHECK(g.pass(3, parseCards("JS QS KS")));

    CHECK(g.phase() == Phase::Play);
    for (int p = 0; p < Game::Players; ++p)
        CHECK_EQ(g.hand(p).size(), 13u);
    CHECK(holds(g, 1, "2C") && holds(g, 1, "4C"));
    CHECK(holds(g, 2, "6D"));
    CHECK(holds(g, 3, "9H"));
    CHECK(holds(g, 0, "QS"));
    // The two of clubs went west, so west leads.
    CHECK_EQ(g.leader(), 1);
    CHECK_EQ(g.turn(), 1);
}

TEST(two_of_clubs_leads_the_first_trick)
{
    Game g = holding(stackedDeck({"3C 4C", "AS", "2C"}));
    CHECK_EQ(g.turn(), 2);
    CHECK(g.legalCards(2) == parseCards("2C"));
    CHECK(!play(g, 0, "3C"));   // not your turn
    CHECK(play(g, 2, "2C"));
    CHECK_EQ(g.turn(), 3);
}

TEST(must_follow_suit)
{
    Game g = holding(stackedDeck({"3C 4C 5D", "AS", "2C", "6C"}));
    CHECK(play(g, 2, "2C"));
    CHECK(play(g, 3, "6C"));
    CHECK(!play(g, 0, "5D"));
    CHECK(g.canPlayCard(0, parseCard("3C")));
    CHECK(play(g, 0, "4C"));
}

TEST(no_points_on_the_first_trick)
{
    // South has no clubs: it may throw a diamond, but not a heart or the queen.
    Game g = holding(stackedDeck({"QS 2H 3H 2D 3D 4D 5D 6D 7D 8D 9D 10D JD", "", "2C", "3C"}));
    CHECK(play(g, 2, "2C"));
    CHECK(play(g, 3, "3C"));
    CHECK(!g.canPlayCard(0, parseCard("QS")));
    CHECK(!g.canPlayCard(0, parseCard("2H")));
    CHECK(g.canPlayCard(0, parseCard("2D")));

    // Nothing but points in the hand: then they may go.
    Game h = holding(stackedDeck({"QS AH KH QH JH 10H 9H 8H 7H 6H 5H 4H 3H", "", "2C", "3C"}));
    CHECK(play(h, 2, "2C"));
    CHECK(play(h, 3, "3C"));
    CHECK(h.canPlayCard(0, parseCard("QS")));
    CHECK(h.canPlayCard(0, parseCard("AH")));
}

TEST(hearts_are_not_led_until_broken)
{
    Game g = holding(stackedDeck({"AC 2H 2D", "3C", "2C", "5C"}));
    // South wins the first trick with the ace and leads.
    trick(g, "2C 5C AC 3C");
    CHECK_EQ(g.turn(), 0);
    CHECK(!g.heartsBroken());
    CHECK(!g.canPlayCard(0, parseCard("2H")));
    CHECK(g.canPlayCard(0, parseCard("2D")));
}

TEST(hearts_led_when_nothing_else)
{
    Game g = holding(stackedDeck({"AC 2H 3H 4H 5H 6H 7H 8H 9H 10H JH QH KH", "", "2C", "3C"}));
    trick(g, "2C 3C AC 4C");
    CHECK_EQ(g.turn(), 0);
    CHECK(g.canPlayCard(0, parseCard("2H")));
}

TEST(a_heart_breaks_hearts)
{
    // East has neither clubs nor diamonds.
    Game g = holding(stackedDeck({"AC 2D", "KC", "2C", "2H 3H 4H 5H 6H 7H 8H 9H 10H JH 4S 5S 6S"}));
    trick(g, "2C 4S AC KC");
    CHECK(play(g, 0, "2D"));
    CHECK(g.play(1, g.legalCards(1).front()));
    CHECK(g.play(2, g.legalCards(2).front()));
    CHECK(!g.heartsBroken());
    CHECK(play(g, 3, "2H"));
    CHECK(g.heartsBroken());
    const int winner = g.trickWinner();
    CHECK(g.collectTrick());
    CHECK_EQ(g.handPoints(winner), 1);
}

TEST(highest_of_the_suit_led_wins)
{
    // East throws a diamond: higher, but not of the suit led.
    Game g = holding(stackedDeck({"3C", "KC", "2C", "AD 2S 3S 4S 5S 6S 7S 8S 9S 10S JS 2H 3H"}));
    CHECK(play(g, 2, "2C"));
    CHECK_EQ(g.trickWinner(), -1);
    CHECK(play(g, 3, "AD"));
    CHECK(play(g, 0, "3C"));
    CHECK(play(g, 1, "KC"));
    CHECK(g.trickComplete());
    CHECK_EQ(g.turn(), -1);
    CHECK_EQ(g.trickWinner(), 1);
    CHECK(!play(g, 1, "AC"));
    CHECK(g.collectTrick());
    CHECK_EQ(g.turn(), 1);
    CHECK_EQ(g.leader(), 1);
    CHECK_EQ(g.tricksPlayed(), 1);
}

TEST(aces_are_high)
{
    Game g = holding(stackedDeck({"AC", "KC", "2C", "QC"}));
    trick(g, "2C QC AC KC");
    CHECK_EQ(g.leader(), 0);
    CHECK_EQ(g.taken(0).size(), 4u);
}

// Plays out the rest of the hand with careful bots.
void finish(Game &g)
{
    while (g.phase() == Phase::Play) {
        if (g.trickComplete())
            g.collectTrick();
        else
            CHECK(g.play(g.turn(), Bot::cautiousPlay(g, g.turn())));
    }
}

TEST(points_add_up_to_twenty_six)
{
    for (unsigned seed = 0; seed < 20; ++seed) {
        Game g(seed);
        g.newGame();
        Bot bot(Difficulty::Medium, seed);
        for (int p = 0; p < Game::Players; ++p)
            CHECK(g.pass(p, bot.choosePass(g, p)));
        finish(g);
        CHECK(g.phase() == Phase::Score);
        int taken = 0;
        int total = 0;
        for (int p = 0; p < Game::Players; ++p) {
            CHECK_EQ(g.taken(p).size() % 4, 0u);
            taken += g.handPoints(p);
            total += g.handScore(p);
        }
        CHECK_EQ(taken, 26);
        CHECK(total == 26 || (g.moonShooter() >= 0 && total == 78));
    }
}

TEST(moon_scores_twenty_six_to_the_others)
{
    // West holds all the clubs and leads the two; nobody else can follow,
    // so west wins every trick and takes every heart and the queen.
    Game g = holding(stackedDeck({"AH KH QH JH 10H 9H 8H 7H 6H 5H 4H 3H 2H",
                                  "2C 3C 4C 5C 6C 7C 8C 9C 10C JC QC KC AC",
                                  "2D 3D 4D 5D 6D 7D 8D 9D 10D JD QD KD AD"}));
    finish(g);
    CHECK_EQ(g.handPoints(1), 26);
    CHECK_EQ(g.moonShooter(), 1);
    CHECK_EQ(g.handScore(1), 0);
    CHECK_EQ(g.handScore(0), 26);
    CHECK_EQ(g.handScore(2), 26);
    CHECK_EQ(g.handScore(3), 26);
    CHECK_EQ(g.score(3), 26);
}

TEST(lowest_score_wins_at_the_target)
{
    Game g = holding(stackedDeck({"AH KH QH JH 10H 9H 8H 7H 6H 5H 4H 3H 2H",
                                  "2C 3C 4C 5C 6C 7C 8C 9C 10C JC QC KC AC",
                                  "2D 3D 4D 5D 6D 7D 8D 9D 10D JD QD KD AD"}));
    g.setScores({80, 10, 50, 60});
    finish(g);
    CHECK(g.phase() == Phase::GameOver);
    CHECK_EQ(g.winner(), 1);
    CHECK_EQ(g.score(0), 106);
    CHECK_EQ(g.previousScore(0), 80);
}

TEST(tie_for_lowest_plays_on)
{
    Game g = holding(stackedDeck({"AH KH QH JH 10H 9H 8H 7H 6H 5H 4H 3H 2H",
                                  "2C 3C 4C 5C 6C 7C 8C 9C 10C JC QC KC AC",
                                  "2D 3D 4D 5D 6D 7D 8D 9D 10D JD QD KD AD"}));
    // West shoots: everyone else gets 26.
    g.setScores({80, 40, 14, 60});
    finish(g);
    CHECK(g.phase() == Phase::Score);
    CHECK_EQ(g.score(1), 40);
    CHECK_EQ(g.score(2), 40);
    CHECK_EQ(g.winner(), -1);
    CHECK(g.nextHand());
    CHECK_EQ(g.round(), 5);
}

TEST(target_can_be_lowered)
{
    Game g = holding(stackedDeck({"AH KH QH JH 10H 9H 8H 7H 6H 5H 4H 3H 2H",
                                  "2C 3C 4C 5C 6C 7C 8C 9C 10C JC QC KC AC",
                                  "2D 3D 4D 5D 6D 7D 8D 9D 10D JD QD KD AD"}));
    g.setTarget(50);
    g.setScores({0, 0, 30, 0});
    finish(g);
    CHECK(g.phase() == Phase::GameOver);
    CHECK_EQ(g.winner(), 1);
}

// Plays a whole game with one bot per seat, checking every move is legal.
void playOut(Game &g, std::vector<Bot> &bots)
{
    int guard = 0;
    while (g.phase() != Phase::GameOver && ++guard < 10000) {
        switch (g.phase()) {
        case Phase::Pass:
            for (int p = 0; p < Game::Players; ++p) {
                const Cards cards = bots[p].choosePass(g, p);
                CHECK_EQ(cards.size(), 3u);
                CHECK(g.pass(p, cards));
            }
            break;
        case Phase::Play:
            if (g.trickComplete()) {
                CHECK(g.collectTrick());
            } else {
                const int p = g.turn();
                const Card c = bots[p].choosePlay(g, p);
                CHECK(g.canPlayCard(p, c));
                if (!g.play(p, c))
                    return;
            }
            break;
        case Phase::Score:
            CHECK(g.nextHand());
            break;
        case Phase::GameOver:
            break;
        }
    }
    CHECK(g.phase() == Phase::GameOver);
}

TEST(bots_play_whole_games_legally)
{
    for (int level = 0; level < 3; ++level) {
        for (unsigned seed = 0; seed < 3; ++seed) {
            Game g(seed);
            g.newGame();
            std::vector<Bot> bots;
            for (int p = 0; p < Game::Players; ++p)
                bots.emplace_back(static_cast<Difficulty>(level), seed * 10 + p);
            playOut(g, bots);
            CHECK(g.winner() >= 0);
        }
    }
}

TEST(medium_passes_the_danger)
{
    Bot bot(Difficulty::Medium, 1);
    Game g(1);
    g.newGame(stackedDeck({"QS AS 2S 3H 4H 5H 6H 7D 8D 9D 10D JD 2C"}));
    const Cards pass = bot.choosePass(g, 0);
    CHECK(std::find(pass.begin(), pass.end(), parseCard("QS")) != pass.end());
    CHECK(std::find(pass.begin(), pass.end(), parseCard("AS")) != pass.end());
}

// South has no diamonds; west wins the first trick and leads.
const std::vector<std::string> QueenDeal = {"4C QS 2S 3S 4S 5S 6S 7S 8S 9S 10S JS 2H", "AC AD KS", "2C", "3C"};

TEST(medium_drops_the_queen_under_the_king)
{
    Game g = holding(stackedDeck(QueenDeal));
    trick(g, "2C 3C 4C AC");
    CHECK(play(g, 1, "KS"));
    CHECK(g.play(2, g.legalCards(2).front()));
    CHECK(g.play(3, g.legalCards(3).front()));
    CHECK(Bot::cautiousPlay(g, 0) == QueenOfSpades);
}

TEST(medium_throws_the_queen_when_void)
{
    Game g = holding(stackedDeck(QueenDeal));
    trick(g, "2C 3C 4C AC");
    CHECK(play(g, 1, "AD"));
    CHECK(g.play(2, g.legalCards(2).front()));
    CHECK(g.play(3, g.legalCards(3).front()));
    CHECK(Bot::cautiousPlay(g, 0) == QueenOfSpades);
}

TEST(medium_does_not_lead_the_queen)
{
    // South wins the first trick and leads low, not the queen.
    Game g = holding(stackedDeck({"AC QS 3S 4D 5D 6D 7D 8D 9D 10D JD QD KD", "", "2C", "3C"}));
    trick(g, "2C 3C AC 4C");
    CHECK_EQ(g.turn(), 0);
    CHECK(Bot::cautiousPlay(g, 0) == parseCard("3S"));
}

// Average points per hand that `strong` takes in seat 0 against three `weak`.
double averagePoints(Difficulty strong, Difficulty weak, int hands, unsigned seed)
{
    double total[2] = {};
    for (int i = 0; i < hands; ++i) {
        Game g(seed + i);
        g.newGame();
        std::vector<Bot> bots;
        for (int p = 0; p < Game::Players; ++p)
            bots.emplace_back(p == 0 ? strong : weak, seed + 100 * i + p);
        for (int p = 0; p < Game::Players; ++p)
            g.pass(p, bots[p].choosePass(g, p));
        while (g.phase() == Phase::Play) {
            if (g.trickComplete())
                g.collectTrick();
            else
                g.play(g.turn(), bots[g.turn()].choosePlay(g, g.turn()));
        }
        total[0] += g.handScore(0);
        for (int p = 1; p < Game::Players; ++p)
            total[1] += g.handScore(p) / 3.0;
    }
    return (total[0] - total[1]) / hands;
}

TEST(stronger_levels_take_fewer_points)
{
    const int hands = 120;
    const double mediumVsEasy = averagePoints(Difficulty::Medium, Difficulty::Easy, hands, 100);
    const double hardVsMedium = averagePoints(Difficulty::Hard, Difficulty::Medium, hands, 200);
    const double easyVsMedium = averagePoints(Difficulty::Easy, Difficulty::Medium, hands, 300);
    std::printf("  points per hand against the others: medium-easy %.2f, hard-medium %.2f, easy-medium %.2f\n",
                mediumVsEasy, hardVsMedium, easyVsMedium);
    CHECK(mediumVsEasy < -2);
    CHECK(hardVsMedium < -0.5);
    CHECK(easyVsMedium > 2);
}

} // namespace

int main()
{
    return cards::testing::run();
}
