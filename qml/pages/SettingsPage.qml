import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    allowedOrientations: Orientation.Portrait

    RemorsePopup { id: remorse }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            PageHeader { title: qsTr("Settings") }

            ComboBox {
                label: qsTr("Bots")
                currentIndex: game.difficulty
                description: [
                    qsTr("Pass and play at random, within the rules."),
                    qsTr("Pass the dangerous cards, duck under the trick and throw the queen away when they can."),
                    qsTr("Keep track of who is out of which suit and think every card through to the end of the hand.")
                ][currentIndex] + " " + qsTr("Takes effect in the next game.")
                menu: ContextMenu {
                    MenuItem { text: qsTr("Easy") }
                    MenuItem { text: qsTr("Medium") }
                    MenuItem { text: qsTr("Hard") }
                }
                onCurrentIndexChanged: game.difficulty = currentIndex
            }

            ComboBox {
                label: qsTr("Game to")
                currentIndex: game.target === 50 ? 0 : 1
                description: qsTr("The game ends when someone reaches this score; the lowest score wins.")
                             + " " + qsTr("Takes effect in the next game.")
                menu: ContextMenu {
                    MenuItem { text: qsTr("50 points") }
                    MenuItem { text: qsTr("100 points") }
                }
                onCurrentIndexChanged: game.target = currentIndex === 0 ? 50 : 100
            }

            ComboBox {
                label: qsTr("Play speed")
                currentIndex: game.speed
                description: qsTr("How long the bots take to play and a finished trick stays on the table.")
                menu: ContextMenu {
                    MenuItem { text: qsTr("Slow") }
                    MenuItem { text: qsTr("Normal") }
                    MenuItem { text: qsTr("Fast") }
                    MenuItem { text: qsTr("Very fast") }
                }
                onCurrentIndexChanged: game.speed = currentIndex
            }

            SectionHeader { text: qsTr("Statistics") }

            Repeater {
                model: game.stats
                DetailItem {
                    label: [qsTr("Easy"), qsTr("Medium"), qsTr("Hard")][index]
                    value: qsTr("%1 won, %2 lost").arg(modelData.won).arg(modelData.lost)
                           + (modelData.moons > 0 ? " · " + qsTr("%n moon(s)", "", modelData.moons) : "")
                }
            }

            Item { width: 1; height: Theme.paddingLarge }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Reset statistics")
                onClicked: remorse.execute(qsTr("Resetting statistics"), function() { game.resetStats() })
            }
        }
        VerticalScrollDecorator { }
    }
}
