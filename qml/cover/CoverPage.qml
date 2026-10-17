import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.hearts 1.0
import "../cards/cards.js" as Cards

CoverBackground {
    Column {
        anchors.centerIn: parent
        width: parent.width - 2 * Theme.paddingLarge
        spacing: Theme.paddingSmall

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Hearts")
            color: Theme.secondaryColor
            font.pixelSize: Theme.fontSizeSmall
        }

        Repeater {
            model: 4
            Item {
                width: parent.width
                height: scoreLabel.height
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: game.playerName(index)
                    color: Cards.playerColor(index)
                }
                Label {
                    id: scoreLabel
                    anchors.right: parent.right
                    text: game.scores[index]
                    color: Cards.playerColor(index)
                    font.pixelSize: Theme.fontSizeLarge
                    font.family: Theme.fontFamilyHeading
                }
            }
        }

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            text: {
                switch (game.phase) {
                case Hearts.Pass: return qsTr("Passing cards")
                case Hearts.Play: return qsTr("Trick %1 of 13").arg(Math.min(13, game.tricksPlayed + 1))
                case Hearts.Score: return qsTr("Hand %1 counted").arg(game.round)
                }
                return game.winner === 0 ? qsTr("You won") : qsTr("%1 won").arg(game.playerName(game.winner))
            }
        }
    }

    CoverActionList {
        enabled: game.phase === Hearts.GameOver
        CoverAction {
            iconSource: "image://theme/icon-cover-new"
            onTriggered: game.newGame()
        }
    }
}
