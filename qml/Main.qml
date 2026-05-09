import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import OCRVideoApp

// Main app window: camera preview + color selection + OCR capture
Window {
    width: 1024
    height: 768
    visible: true
    title: qsTr("Video OCR Complete Solution - Qt 6.5.3")
    color: "#dcdcdc"

    // Get list of available cameras
    MediaDevices { id: mediaDevices }

    // Capture session: camera → inputSink (frames to C++)
    CaptureSession {
        id: captureSession
        camera: Camera {
            id: camera
            cameraDevice: mediaDevices.defaultVideoInput
            active: true  // Start camera
        }
        videoOutput: VideoSink {
            id: inputSink  // Frames go here
        }
    }

    // VideoProcessor: connects C++ backend with QML
    VideoProcessor {
        id: processor
        videoSink: inputSink  // Raw camera frames
        outputSink: videoOutput.videoSink  // Processed frames for display
        colorMode: 0  // Default: Colors_first
        onOcrResultReady: (result) => {  // OCR result received
            resultDialog.text = result;
            resultDialog.open();
        }
    }

    // Main vertical layout
    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        // Video preview area
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Rectangle {
                anchors.fill: parent
                color: "#000000"
                border.color: "#2b2b2b"
            }

            VideoOutput {
                id: videoOutput
                anchors.fill: parent
                fillMode: VideoOutput.PreserveAspectFit
            }
        }

        // Color selection button
        Button {
            Layout.fillWidth: true
            Layout.leftMargin: 6
            Layout.rightMargin: 6
            text: "Select Monochrome Colors"
            onClicked: colorDialog.open()
            contentItem: Text {
                text: parent.text
                color: "#2a2a2a"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 18
            }
            background: Rectangle {
                implicitHeight: 32
                color: parent.down ? "#d7e3f2" : "#e5edf8"
                border.color: "#8eb3d7"
                radius: 2
            }
        }

        // Status and capture button area
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 120

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 14
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 56
                text: "LIVE WEBCAM | F4-OCR | Colors Ready"
                color: "#00a12d"
                font.bold: true
                font.pixelSize: 18
            }

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 14
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20
                text: colorDialog.visible ? "Colors Dialog Open | Select Set 1-4" : ""
                color: "#00a12d"
                font.bold: true
                font.pixelSize: 17
            }

            Button {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.rightMargin: 14
                anchors.bottomMargin: 16
                text: "Capture (F4)"
                onClicked: processor.captureAndOCR()  // Trigger OCR
                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.bold: true
                }
                background: Rectangle {
                    implicitWidth: 140
                    implicitHeight: 40
                    color: parent.down ? "#0f7f09" : "#11c70e"
                    border.color: "#0b6f07"
                    radius: 4
                }
            }
        }
    }

    // F4 shortcut handler
    Shortcut {
        sequence: "F4"
        context: Qt.ApplicationShortcut
        autoRepeat: false
        onActivated: processor.captureAndOCR()  // Call OCR on F4 press
    }

    // Color selection dialog
    ColorDialog {
        id: colorDialog
        onModeSelected: (mode) => processor.colorMode = mode
    }

    // Result dialog: shows OCR text after capture
    ResultDialog {
        id: resultDialog
    }
}
