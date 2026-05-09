import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root
    title: "Select Color Set"
    anchors.centerIn: parent
    modal: true
    width: 400
    height: 450

    signal modeSelected(int mode)

    background: Rectangle {
        color: "#1e1e1e"
        radius: 10
        border.color: "#333333"
    }

    header: Rectangle {
        height: 50
        color: "#252525"
        radius: 10
        Text {
            anchors.centerIn: parent
            text: "Monochrome Color Sets"
            color: "white"
            font.bold: true
        }
    }

    contentItem: ColumnLayout {
        spacing: 15
        anchors.margins: 20

        Repeater {
            model: [
                { name: "Colors_first", bg: "#ffffff", fg: "#000000" },
                { name: "Colors_Second", bg: "#000000", fg: "#ffffff" },
                { name: "Colors_Third", bg: "#000000", fg: "#11c70e" },
                { name: "Colors_Fourth", bg: "#000000", fg: "#f4d81e" }
            ]

            delegate: ItemDelegate {
                Layout.fillWidth: true
                height: 60
                onClicked: {
                    root.modeSelected(index)
                    root.close()
                }

                background: Rectangle {
                    color: parent.hovered ? "#333333" : "#252525"
                    radius: 5
                    border.color: parent.down ? "#11c70e" : "transparent"
                }

                contentItem: RowLayout {
                    spacing: 20
                    Rectangle {
                        width: 40
                        height: 40
                        color: modelData.bg
                        border.color: "#444444"
                        radius: 20
                        Text {
                            anchors.centerIn: parent
                            text: "A"
                            color: modelData.fg
                            font.bold: true
                        }
                    }
                    Text {
                        text: modelData.name
                        color: "white"
                        font.pixelSize: 16
                    }
                }
            }
        }
    }
}
