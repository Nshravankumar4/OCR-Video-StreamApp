import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Dialog for selecting 1 of 4 monochrome color sets
Dialog {
    id: root
    title: "Select Monochrome Color Set"
    anchors.centerIn: parent
    modal: true
    width: 470
    height: 360

    // Emitted when color selected, passes mode (0-3)
    signal modeSelected(int mode)

    background: Rectangle {
        color: "#f7f7f7"
        radius: 8
        border.color: "#c2c2c2"
    }

    contentItem: ColumnLayout {
        spacing: 18
        anchors.margins: 20

        Text {
            text: "Choose Color Mapping:"
            color: "#1f1f1f"
            font.pixelSize: 24
            font.bold: true
            Layout.bottomMargin: 10
        }

        // Generate 4 color option buttons
        Repeater {
            model: [
                { name: "Colors First", mode: 0, bg: "#ffffff", fg: "#000000" },
                { name: "Colors Second", mode: 1, bg: "#000000", fg: "#ffffff" },
                { name: "Colors Third", mode: 2, bg: "#000000", fg: "#11c70e" },
                { name: "Colors Fourth", mode: 3, bg: "#000000", fg: "#f4d81e" }
            ]

            // One clickable color option
            delegate: Rectangle {
                Layout.fillWidth: true
                height: 28

                border.color: "#8f8f8f"
                border.width: 1

                // Gradient preview of the color set
                gradient: Gradient {
                    GradientStop { position: 0.0; color: modelData.bg }
                    GradientStop { position: 0.5; color: Qt.darker(modelData.bg, 1.25) }
                    GradientStop { position: 1.0; color: modelData.fg }
                }

                Text {
                    anchors.centerIn: parent
                    text: modelData.name
                    color: modelData.mode === 0 ? "#111111" : modelData.fg
                    font.pixelSize: 18
                    font.bold: true
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.modeSelected(modelData.mode)  // Emit signal
                        root.close()  // Close dialog
                    }
                }
            }
        }
    }
}
