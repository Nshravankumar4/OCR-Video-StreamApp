import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import OCRVideoApp

Window {
    width: 1024
    height: 768
    visible: true
    title: qsTr("OCR Multimedia App")
    color: "#121212"


    MediaDevices { id: mediaDevices }

    CaptureSession {
        id: captureSession
        camera: Camera {
            id: camera
            cameraDevice: mediaDevices.defaultVideoInput
            active: true
        }
        videoOutput: VideoSink {
            id: inputSink
        }
    }

    VideoProcessor {
        id: processor
        videoSink: inputSink
        outputSink: videoOutput.videoSink
        colorMode: -1
        onOcrResultReady: (result) => {
            resultDialog.text = result;
            resultDialog.open();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header
        Rectangle {
            Layout.fillWidth: true
            height: 60
            color: "#1e1e1e"
            
            Text {
                anchors.centerIn: parent
                text: "OCR Video Stream"
                color: "#ffffff"
                font.pixelSize: 24
                font.bold: true
            }
        }

        // Main View
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            VideoOutput {
                id: videoOutput
                anchors.fill: parent
                fillMode: VideoOutput.PreserveAspectFit
            }

            // Controls Overlay
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.margins: 20
                width: 300
                height: 80
                color: "#aa000000"
                radius: 15
                border.color: "#333333"

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 20

                    Button {
                        text: "Color Sets"
                        onClicked: colorDialog.open()
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            implicitWidth: 100
                            implicitHeight: 40
                            color: parent.down ? "#444444" : "#222222"
                            radius: 5
                        }
                    }

                    Button {
                        text: "Capture (F4)"
                        onClicked: processor.captureAndOCR()
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            implicitWidth: 100
                            implicitHeight: 40
                            color: parent.down ? "#0e6309" : "#11c70e"
                            radius: 5
                        }
                    }
                }
            }
        }
    }

    // Key Handling (F4 Shortcut)
    Shortcut {
        sequence: "F4"
        onActivated: processor.captureAndOCR()
    }

    ColorDialog {
        id: colorDialog
        onModeSelected: (mode) => processor.colorMode = mode
    }

    ResultDialog {
        id: resultDialog
    }
}
