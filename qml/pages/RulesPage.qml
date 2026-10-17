import QtQuick 2.6
import Sailfish.Silica 1.0
import "../cards/cards.js" as Cards

Page {
    id: page

    // A player's name in their colour.
    function named(player) { return "<font color=\"" + Cards.playerColor(player) + "\">" + game.playerName(player) + "</font>" }

    allowedOrientations: Orientation.Portrait

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader { title: qsTr("Hearts") }

            Repeater {
                model: [
                    { title: qsTr("The game"), text: qsTr("Four players, each for themselves, try to take as few points as possible. Every heart taken is one point and the queen of spades thirteen. The game ends after the hand in which someone reaches 100 points (or 50, see the settings), and the lowest score wins. On a tie for lowest, another hand is played.") },
                    { title: qsTr("Passing"), text: qsTr("Each player gets thirteen cards and passes three of them on: to the left in the first hand, to the right in the second, across in the third, and none in the fourth. Then the cycle starts again.") },
                    { title: qsTr("The play"), text: qsTr("Whoever holds the two of clubs leads it to the first trick. Players must follow suit if they can; if not, they may play any card. The highest card of the suit led takes the trick, aces high, and its taker leads the next. No hearts or queen of spades may be played to the first trick, unless a hand holds nothing else. Hearts may not be led until a heart has been played, unless the leader holds only hearts.") },
                    { title: qsTr("Shooting the moon"), text: qsTr("Taking all thirteen hearts and the queen of spades in one hand is shooting the moon: the shooter scores nothing and everyone else 26.") },
                    { title: qsTr("Playing here"), text: qsTr("%1 sit at the bottom, with %2 to your left, %3 across and %4 to your right. To pass, tap three cards and then the button; the cards passed to you stay raised until you play. During the play, tap a card to play it; cards you may not play are dimmed. Each player's colour marks their score and the tricks they win.").arg(page.named(0)).arg(page.named(1)).arg(page.named(2)).arg(page.named(3)) }
                ]
                Column {
                    width: column.width
                    SectionHeader { text: modelData.title }
                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        wrapMode: Text.Wrap
                        textFormat: Text.StyledText
                        text: modelData.text
                        color: Theme.highlightColor
                        font.pixelSize: Theme.fontSizeSmall
                    }
                }
            }

            SectionHeader { text: qsTr("About") }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                text: qsTr("Hearts %1 for Sailfish OS.").arg(Qt.application.version)
                color: Theme.secondaryHighlightColor
                font.pixelSize: Theme.fontSizeSmall
            }
        }
        VerticalScrollDecorator { }
    }
}
