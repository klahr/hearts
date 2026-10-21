import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.hearts 1.0
import "../components"
import "../cards"
import "../cards/cards.js" as Cards
import "../online"

Page {
    id: page

    property var selected: []
    // One card size everywhere: seven overlapping cards fill a row of your hand.
    readonly property real cardWidth: Math.floor((width - 2 * Theme.horizontalPageMargin) / (1 + 5 * 0.76))
    readonly property real cardHeight: Math.round(cardWidth * 1.42)
    readonly property real margin: Theme.horizontalPageMargin
    // Your hand lies in two rows while it holds more than seven cards.
    readonly property real handRowStep: Math.round(cardHeight * 0.5)
    // How much of the side players' cards shows at the edge of the screen.
    readonly property real sideShown: Math.round(cardHeight * 0.42)
    readonly property bool scoring: game.phase === Hearts.Score || game.phase === Hearts.GameOver
    readonly property var received: game.received
    // In the count after a hand, each row starts after the name and ends
    // before the points.
    readonly property real scoreNameWidth: Math.round(cardWidth * 1.1)
    readonly property real scorePointsWidth: Math.round(cardWidth)

    readonly property var places: computePlaces()
    // Holding the table: the last trick shows over this one.
    readonly property bool peeking: peek.peeking && game.lastTrick.length > 0

    function lastTrickWinner() {
        var last = game.lastTrick
        for (var i = 0; i < last.length; ++i)
            if (last[i].won)
                return last[i].player
        return -1
    }

    allowedOrientations: Orientation.Portrait

    function toggle(id) {
        var s = selected.slice()
        var i = s.indexOf(id)
        if (i >= 0)
            s.splice(i, 1)
        else if (s.length < 3)
            s.push(id)
        else
            s = [s[1], s[2], id]
        selected = s
    }

    function startNewGame() {
        if (game.phase === Hearts.GameOver || game.round <= 1 && game.phase === Hearts.Pass && !game.humanPassed)
            game.newGame()
        else
            remorse.execute(qsTr("Starting a new game"), function() { game.newGame() })
    }

    // A player's name, or a line meant for you, in the player's colour.
    function coloured(player, text) { return "<font color=\"" + Cards.playerColor(player) + "\">" + text + "</font>" }
    function named(player) { return coloured(player, game.playerName(player)) }

    // A heart is one point, the queen of spades thirteen.
    function points(id) { return Cards.suit(id) === 2 ? 1 : (id === 50 ? 13 : 0) }

    function trickPoints() {
        var points = 0
        var trick = game.trick
        for (var i = 0; i < trick.length; ++i)
            points += page.points(trick[i].card)
        return points
    }

    // Where each player's card goes on the table.
    function trickSpot(player) {
        var x = (page.width - cardWidth) / 2
        var top = cluster.y + tableRow.y
        var middle = top + (tableRow.height - cardHeight) / 2
        switch (player) {
        case 1: return { x: x - cardWidth * 1.15, y: middle }
        case 2: return { x: x, y: top }
        case 3: return { x: x + cardWidth * 1.15, y: middle }
        }
        return { x: x, y: top + tableRow.height - cardHeight }
    }

    // Where the cards you pass gather, in front of the player receiving them.
    function passArea() {
        var off = Math.round(cardWidth * 0.3)
        var w = cardWidth + 2 * off
        var spot = trickSpot(game.passTarget)
        var x = Math.max(Theme.paddingSmall, Math.min(page.width - w - Theme.paddingSmall, spot.x + (cardWidth - w) / 2))
        return { x: x, y: spot.y, width: w, height: cardHeight, off: off }
    }

    // Where each player keeps the tricks taken: under their own cards, yours
    // behind the first row of your hand.
    function pileSpot(player) {
        var w = page.width
        var middle = cluster.y + tableRow.y + (tableRow.height - cardHeight) / 2
        switch (player) {
        case 1: return { x: sideShown - (cardHeight + cardWidth) / 2, y: middle, rot: 90 }
        case 2: return { x: (w - cardWidth) / 2, y: cluster.y + northRow.y, rot: 0 }
        case 3: return { x: w - sideShown + (cardHeight - cardWidth) / 2, y: middle, rot: 90 }
        }
        return { x: (w - cardWidth) / 2, y: cluster.y + handRow.y + Theme.paddingLarge, rot: 0 }
    }

    // The rows of points taken, when a hand is counted.
    function scoreRowY(player) {
        var top = cluster.y + northRow.y
        var height = tableRow.y + tableRow.height - northRow.y
        return top + player * (height - cardHeight) / 3
    }

    // Where every one of the 52 cards lies, in content coordinates.
    function computePlaces() {
        var cw = cardWidth
        var ch = cardHeight
        var w = page.width
        var hands = game.hands
        var tableY = cluster.y + tableRow.y
        var handY = cluster.y + handRow.y + Theme.paddingLarge
        // Cards fly out one after the other as they are dealt.
        var dealing = game.tricksPlayed === 0 && game.trick.length === 0
        var p = []
        var i, k, s, n, off, x0, ids, spot

        // Cards out of play are hidden where your hand is, and dealt from there.
        for (i = 0; i < 52; ++i)
            p.push({ x: (w - cw) / 2, y: handY, z: 1, down: true, rot: 0, role: "deck", delay: 0, hidden: true })

        if (scoring) {
            // Each player's hearts and queen in a row by their name.
            for (s = 0; s < 4; ++s) {
                ids = []
                var taken = game.taken[s]
                for (i = 0; i < taken.length; ++i)
                    if (page.points(taken[i]) > 0)
                        ids.push(taken[i])
                // The queen first, then the hearts from the ace down.
                ids.sort(function(a, b) { return (b === 50 ? 99 : Cards.high(b)) - (a === 50 ? 99 : Cards.high(a)) })
                n = ids.length
                var left = margin + scoreNameWidth
                off = n > 1 ? Math.min(cw * 0.5, (w - margin - scorePointsWidth - left - cw) / (n - 1)) : 0
                for (i = 0; i < n; ++i)
                    p[ids[i]] = { x: left + i * off, y: scoreRowY(s), z: 10 + s * 20 + i, down: false, rot: 0,
                                  role: "score", delay: s * 80 + i * 40 }
            }
            return p
        }

        // Tricks taken lie face down under the taker's cards.
        for (s = 0; s < 4; ++s) {
            spot = pileSpot(s)
            ids = game.taken[s]
            for (i = 0; i < ids.length; ++i)
                p[ids[i]] = { x: spot.x, y: spot.y, z: 5 + i, down: true, rot: spot.rot, role: "taken",
                              delay: 0, glowColor: Cards.playerColor(s) }
        }

        // Your cards, centred, in two rows while there are many; the ones
        // chosen to pass wait in the pass area.
        var choosing = game.phase === Hearts.Pass && !game.humanPassed
        ids = choosing ? hands[0].filter(function(id) { return page.selected.indexOf(id) < 0 }) : hands[0]
        n = ids.length
        var top = n > 7 ? Math.ceil(n / 2) : n
        off = top > 1 ? Math.min(cw * 0.76, (w - 2 * margin - cw) / (top - 1)) : 0
        for (i = 0; i < n; ++i) {
            var row = i < top ? 0 : 1
            var inRow = row === 0 ? top : n - top
            k = row === 0 ? i : i - top
            x0 = (w - cw - (inRow - 1) * off) / 2
            p[ids[i]] = { x: x0 + k * off, y: handY + row * handRowStep, z: 50 + i, down: false, rot: 0,
                          role: "hand", delay: dealing ? i * 30 : 0, glowColor: Cards.playerColor(game.passSource) }
        }

        // West and east hold their cards sideways at the edges, north
        // across the top.
        // The cards they pass stay in their hands until the exchange.
        for (s = 1; s <= 3; s += 2) {
            ids = hands[s].concat(game.passed[s])
            n = ids.length
            off = n > 1 ? Math.min(cw * 0.3, (tableRow.height - cw) / (n - 1)) : 0
            var centreX = s === 1 ? sideShown - ch / 2 : w - sideShown + ch / 2
            var centreY = tableY + (tableRow.height - (n - 1) * off) / 2
            for (i = 0; i < n; ++i)
                p[ids[i]] = { x: centreX - cw / 2, y: centreY + i * off - ch / 2, z: 30 + i, down: true, rot: 90,
                              role: "bot", delay: dealing ? i * 30 : 0 }
        }
        ids = hands[2].concat(game.passed[2])
        n = ids.length
        off = n > 1 ? Math.min(cw * 0.22, (w - 2 * margin - cw) / (n - 1)) : 0
        x0 = (w - cw - (n - 1) * off) / 2
        for (i = 0; i < n; ++i)
            p[ids[i]] = { x: x0 + i * off, y: cluster.y + northRow.y, z: 30 + i, down: true, rot: 0,
                          role: "bot", delay: dealing ? i * 30 : 0 }

        if (choosing) {
            var area = passArea()
            for (i = 0; i < page.selected.length; ++i)
                p[page.selected[i]] = { x: area.x + i * area.off, y: area.y, z: 80 + i, down: false, rot: 0,
                                        role: "passing", delay: 0 }
        }

        // The trick, each card in front of whoever played it; the winning
        // card lights up once all four are down.
        var trick = game.trick
        for (i = 0; i < trick.length; ++i) {
            spot = trickSpot(trick[i].player)
            p[trick[i].card] = { x: spot.x, y: spot.y, z: 100 + i, down: false, rot: 0, role: "trick", delay: 0,
                                 glow: trick[i].player === game.trickWinner,
                                 glowColor: Cards.playerColor(trick[i].player) }
        }

        // Held down on the table: the last trick back up, over this one.
        if (peeking) {
            for (i = 0; i < trick.length; ++i)
                p[trick[i].card].hidden = true
            var last = game.lastTrick
            for (i = 0; i < last.length; ++i) {
                spot = trickSpot(last[i].player)
                p[last[i].card] = { x: spot.x, y: spot.y, z: 300 + i, down: false, rot: 0, role: "last", delay: 0,
                                    glow: last[i].won, glowColor: Cards.playerColor(last[i].player) }
            }
        }
        return p
    }

    Connections {
        target: game
        onChanged: {
            if (game.phase !== Hearts.Pass || game.humanPassed)
                page.selected = []
        }
    }

    RemorsePopup { id: remorse }

    ConnectionBanner { }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: Math.max(page.height, cluster.y + cluster.height + Theme.paddingLarge)

        PullDownMenu {
            MenuItem {
                text: qsTr("About and rules")
                onClicked: pageStack.push(Qt.resolvedUrl("RulesPage.qml"))
            }
            MenuItem {
                text: qsTr("Settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }
            MenuItem {
                text: online.active ? qsTr("Online table") : qsTr("Play online")
                onClicked: pageStack.push(Qt.resolvedUrl(online.atTable ? "../online/TablePage.qml"
                                                                        : "../online/LobbyPage.qml"))
            }
            MenuItem {
                text: qsTr("New game")
                // Online, a new game starts only once this one is over.
                visible: !online.active || game.phase === Hearts.GameOver
                onClicked: page.startNewGame()
            }
        }

        Item {
            id: content
            width: parent.width
            height: parent.contentHeight

            // ---------------------------------------------------- board
            Row {
                id: board
                x: margin
                // Below the notch, on phones with one.
                y: Screen.topCutout.height + Theme.paddingLarge
                width: parent.width - 2 * margin

                Repeater {
                    model: 4
                    PlayerScore {
                        width: board.width / 4
                        name: game.playerName(index)
                        score: game.scores[index]
                        target: game.gameTarget
                        handPoints: game.phase === Hearts.Play ? game.handPoints[index] : 0
                        active: game.phase === Hearts.Play && game.turn === index
                        color: Cards.playerColor(index)
                    }
                }
            }

            // Everything else sits together in the middle of the screen, so
            // your hand is within easy reach of the thumb.
            Column {
                id: cluster
                width: parent.width
                y: board.y + board.height
                   + Math.max(Theme.paddingLarge,
                              (page.height - board.y - board.height
                               - (height - info.height + messageBlock.height)) * 0.4)
                spacing: Theme.paddingMedium

                // North's cards (drawn by the card layer).
                Item {
                    id: northRow
                    width: parent.width
                    height: cardHeight
                }

                // The trick, with west and east at the sides.
                Item {
                    id: tableRow
                    width: parent.width
                    height: Math.round(cardHeight * 1.8)
                }

                // Messages, the count of the hand, or the result.
                Item {
                    id: info
                    width: parent.width
                    height: game.phase === Hearts.Score ? scorePanel.height
                                                        : game.phase === Hearts.GameOver ? overPanel.height
                                                                                         : messageBlock.height

                    Column {
                        id: messageBlock
                        x: margin
                        width: parent.width - 2 * margin
                        opacity: page.scoring ? 0 : 1
                        visible: opacity > 0
                        Behavior on opacity { FadeAnimation { } }

                        Label {
                            width: parent.width
                            height: fontMetrics.height
                            horizontalAlignment: Text.AlignHCenter
                            truncationMode: TruncationMode.Fade
                            textFormat: Text.StyledText
                            color: Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeSmall
                            text: {
                                switch (game.phase) {
                                case Hearts.Pass:
                                    if (game.humanPassed)
                                        return online.active ? qsTr("Waiting for the others to pass…") : ""
                                    return [qsTr("Choose three cards for %1, on your left."),
                                            qsTr("Choose three cards for %1, on your right."),
                                            qsTr("Choose three cards for %1, across.")][Math.min(game.passDirection, 2)]
                                            .arg(page.named(game.passTarget))
                                case Hearts.Play:
                                    if (page.peeking)
                                        return page.lastTrickWinner() === 0 ? page.coloured(0, qsTr("You took the last trick."))
                                                                            : qsTr("%1 took the last trick.").arg(page.named(page.lastTrickWinner()))
                                    if (game.trickWinner >= 0) {
                                        var points = page.trickPoints()
                                        if (game.trickWinner === 0)
                                            return page.coloured(0, points > 0 ? qsTr("You take %n point(s).", "", points)
                                                                               : qsTr("You take the trick."))
                                        return points > 0
                                                ? qsTr("%1 takes %n point(s).", "", points).arg(page.named(game.trickWinner))
                                                : qsTr("%1 takes the trick.").arg(page.named(game.trickWinner))
                                    }
                                    if (game.humanTurn) {
                                        if (game.tricksPlayed === 0 && game.trick.length === 0)
                                            return page.coloured(0, qsTr("Lead the two of clubs."))
                                        if (game.trick.length > 0)
                                            return page.coloured(0, qsTr("Your turn. Tap a card to play it."))
                                        return page.coloured(0, game.heartsBroken ? qsTr("Your lead.")
                                                                                  : qsTr("Your lead. Hearts are not broken yet."))
                                    }
                                    if (page.received.length > 0)
                                        return qsTr("%1 passed you the raised cards.").arg(page.named(game.passSource))
                                    if (game.passDirection === 3 && game.tricksPlayed === 0)
                                        return qsTr("No passing this hand.")
                                    return qsTr("%1 is playing…").arg(page.named(game.turn))
                                }
                                return ""
                            }
                        }

                        FontMetrics {
                            id: fontMetrics
                            font.pixelSize: Theme.fontSizeSmall
                        }
                    }

                    Column {
                        id: scorePanel
                        x: margin
                        width: parent.width - 2 * margin
                        opacity: game.phase === Hearts.Score ? 1 : 0
                        visible: opacity > 0
                        Behavior on opacity { FadeAnimation { } }

                        Label {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.Wrap
                            text: game.moonShooter === 0 ? qsTr("You shot the moon!")
                                  : game.moonShooter > 0 ? qsTr("%1 shot the moon!").arg(game.playerName(game.moonShooter))
                                  : qsTr("Hand %1").arg(game.round)
                            color: game.moonShooter >= 0 ? Cards.playerColor(game.moonShooter) : Theme.highlightColor
                            font.pixelSize: Theme.fontSizeLarge
                            font.family: Theme.fontFamilyHeading
                        }
                        Label {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            text: [qsTr("Next hand: pass left"), qsTr("Next hand: pass right"),
                                   qsTr("Next hand: pass across"), qsTr("Next hand: no passing")][game.round % 4]
                            color: Theme.secondaryHighlightColor
                            font.pixelSize: Theme.fontSizeSmall
                        }
                    }

                    Column {
                        id: overPanel
                        x: margin
                        width: parent.width - 2 * margin
                        opacity: game.phase === Hearts.GameOver ? 1 : 0
                        visible: opacity > 0
                        Behavior on opacity { FadeAnimation { duration: 400 } }

                        Label {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            text: game.winner === 0 ? qsTr("You won")
                                                    : qsTr("%1 won").arg(game.playerName(game.winner))
                            color: Cards.playerColor(game.winner)
                            font.pixelSize: Theme.fontSizeHuge
                            font.family: Theme.fontFamilyHeading
                        }
                        Label {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.Wrap
                            text: qsTr("Lowest score with %1 points").arg(game.winner >= 0 ? game.scores[game.winner] : 0)
                            color: Theme.secondaryHighlightColor
                            font.pixelSize: Theme.fontSizeMedium
                        }
                    }
                }

                // Space for your hand (cards are drawn by the card layer).
                Item {
                    id: handRow
                    width: parent.width
                    height: cardHeight + handRowStep + Theme.paddingLarge
                }

                Item {
                    width: parent.width
                    height: Theme.itemSizeSmall

                    Row {
                        anchors.centerIn: parent
                        spacing: Theme.paddingLarge

                        Button {
                            visible: game.phase === Hearts.Pass && !game.humanPassed
                            enabled: page.selected.length === 3
                            text: online.active ? qsTr("To %1").arg(game.playerName(game.passTarget))
                                                : [qsTr("To West"), qsTr("To East"), qsTr("To North")][Math.min(game.passDirection, 2)]
                            onClicked: game.pass(page.selected)
                        }
                        Button {
                            visible: game.phase === Hearts.Score
                            text: qsTr("Next hand")
                            onClicked: game.nextHand()
                        }
                        Button {
                            visible: game.phase === Hearts.GameOver
                            text: qsTr("New game")
                            onClicked: game.newGame()
                        }
                    }
                }
            }

            // The area your passed cards gather in, by the player receiving them.
            ChoiceArea {
                area: page.passArea()
                color: Cards.playerColor(game.passTarget)
                text: qsTr("Three cards for %1").arg(page.named(game.passTarget))
                empty: page.selected.length === 0
                shown: game.phase === Hearts.Pass && !game.humanPassed
            }

            // Names and points by the rows of the count.
            Repeater {
                model: 4
                Item {
                    x: margin
                    y: page.scoreRowY(index)
                    z: 400
                    width: page.width - 2 * margin
                    height: cardHeight / 2
                    opacity: page.scoring ? 1 : 0
                    visible: opacity > 0
                    Behavior on opacity { FadeAnimation { } }

                    Label {
                        width: page.scoreNameWidth - Theme.paddingMedium
                        y: Theme.paddingSmall
                        text: game.playerName(index)
                        color: Cards.playerColor(index)
                        font.pixelSize: Theme.fontSizeMedium
                        truncationMode: TruncationMode.Fade
                    }
                    Label {
                        anchors.right: parent.right
                        y: Theme.paddingSmall
                        text: game.moonShooter === index ? "☾" : "+" + game.handScores[index]
                        color: Cards.playerColor(index)
                        font.pixelSize: Theme.fontSizeLarge
                        font.family: Theme.fontFamilyHeading
                    }
                }
            }

            // Hold the table to see the last trick.
            PeekArea {
                id: peek
                y: cluster.y + tableRow.y
                width: parent.width
                height: tableRow.height
                enabled: game.phase === Hearts.Play && game.lastTrick.length > 0
            }

            // ---------------------------------------------------- cards
            TapToPlay {
                id: tapToPlay
                twice: game.tapTwice
            }

            // All 52 cards live here and glide to wherever the game puts them.
            Repeater {
                model: 52
                TableCard {
                    readonly property bool inHand: place.role === "hand"
                    readonly property bool passing: place.role === "passing"

                    place: page.places[index]
                    width: cardWidth
                    cardId: index
                    selected: inHand && page.selected.indexOf(index) >= 0 || tapToPlay.raised === index
                    // The cards passed to you, and the card winning the trick.
                    glowing: place.glow === true || (inHand && page.received.indexOf(index) >= 0)
                    glowColor: place.glowColor !== undefined ? place.glowColor : Theme.highlightColor
                    // game.trick makes these follow the play.
                    dimmed: inHand && game.humanTurn && game.trick.length >= 0 && !game.canPlay(index)
                    enabled: passing || inHand && (game.phase === Hearts.Pass ? !game.humanPassed
                                                                   : game.humanTurn && game.trick.length >= 0
                                                                     && game.canPlay(index))

                    onEnabledChanged: if (!enabled) tapToPlay.lower(index)
                    onClicked: {
                        if (game.phase === Hearts.Pass)
                            page.toggle(index)
                        else if (tapToPlay.tap(index))
                            game.play(index)
                    }
                }
            }
        }
    }
}
