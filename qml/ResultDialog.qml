import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Dialog for displaying OCR recognition result
Dialog {
    id: root
    property string text: ""  // OCR result text property
    
    title: "OCR Recognition Result"
    anchors.centerIn: parent
    modal: true
    width: 620
    height: 560

    background: Rectangle {
        color: "#f7f7f7"
        radius: 8
        border.color: "#c2c2c2"
    }

    // Scrollable text area
    contentItem: ScrollView {
        anchors.margins: 14
        clip: true
        
        TextArea {
            text: root.text
            readOnly: true  // Read-only display
            color: "#1f1f1f"
            font.family: "Monospace"
            font.pixelSize: 16
            wrapMode: TextArea.Wrap
            background: Rectangle {
                color: "#e8f1fb"
                border.color: "#8fb2d5"
                radius: 2
            }
        }
    }

    // Close button footer
    footer: DialogButtonBox {
        background: Rectangle { color: "transparent" }
        Button {
            text: "Close"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            contentItem: Text {
                text: parent.text
                color: "#1f1f1f"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                implicitWidth: 90
                implicitHeight: 35
                color: parent.down ? "#dfe8f4" : "#f4f7fc"
                border.color: "#8fb2d5"
                radius: 3
            }
        }
    }
}
