import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root
    property string text: ""
    
    title: "OCR Result"
    anchors.centerIn: parent
    modal: true
    width: 500
    height: 400

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
            text: "Recognized Text"
            color: "white"
            font.bold: true
        }
    }

    contentItem: ScrollView {
        anchors.margins: 15
        clip: true
        TextArea {
            text: root.text
            readOnly: true
            color: "#11c70e"
            font.family: "Monospace"
            font.pixelSize: 14
            wrapMode: TextArea.Wrap
            background: null
        }
    }

    footer: DialogButtonBox {
        background: Rectangle { color: "transparent" }
        Button {
            text: "Close"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            contentItem: Text {
                text: parent.text
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                implicitWidth: 80
                implicitHeight: 35
                color: parent.down ? "#444444" : "#222222"
                radius: 5
            }
        }
    }
}
