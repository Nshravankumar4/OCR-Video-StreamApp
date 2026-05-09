# OCR-Video-StreamApp

OCR-Video-StreamApp is a real-time camera text recognition application built with Qt, OpenCV, and Tesseract.

The app opens your camera, shows a live monochrome preview using one of four color styles, and runs OCR on the current frame when you press F4 or click Capture.

<img width="1327" height="1015" alt="Menu" src="https://github.com/user-attachments/assets/4b5fe815-cdc9-4dab-b764-044f56e821be" />
<img width="1297" height="1008" alt="Capture" src="https://github.com/user-attachments/assets/1708086d-7706-407b-83ca-bdd484bfa0dd" />

## What the Application Does

1. Starts the default camera.
2. Receives live frames continuously.
3. Converts each frame to grayscale and applies the selected monochrome color mapping.
4. Renders the processed frame back to the UI as live preview.
5. On capture, takes the latest frame and saves a screenshot.
6. Extracts the center region of interest (ROI) for OCR.
7. Applies OCR preprocessing (denoise + adaptive threshold + optional inversion).
8. Runs Tesseract OCR on the processed image.
9. Displays recognized text in a result dialog.

## Color Modes

The app supports exactly four color sets:

1. Colors_first: white background + black foreground
2. Colors_Second: black background + white foreground
3. Colors_Third: black background + green foreground
4. Colors_Fourth: black background + yellow foreground

## Technology Stack

- Qt 6.5.3 (QML UI, Camera, VideoSink, QtConcurrent)
- OpenCV 4.8 (image conversion and preprocessing)
- Tesseract 5 (OCR engine)
- C++17
- CMake

## Project Structure

- main.cpp: Application entry point, QML engine setup, VideoProcessor type registration
- videoprocessor.h: VideoProcessor class interface (properties, slots, signals)
- videoprocessor.cpp: Core video processing and OCR implementation
- qml/Main.qml: Main window, camera preview, capture controls, shortcut handling
- qml/ColorDialog.qml: Color selection dialog
- qml/ResultDialog.qml: OCR result display dialog
- CMakeLists.txt: Build configuration and dependency linking

## Runtime Flow

### 1) Startup Flow

1. main.cpp creates QGuiApplication and QQmlApplicationEngine.
2. VideoProcessor type is registered for QML.
3. Main QML window loads.
4. VideoProcessor constructor initializes Tesseract and locates eng.traineddata.
5. QML binds camera input/output sinks and default color mode.

### 2) Live Preview Flow

1. Camera emits a new frame.
2. VideoProcessor::handleFrame stores the latest frame.
3. A background task converts frame data to OpenCV format.
4. buildMonochromePreview applies selected color mapping.
5. Processed frame is converted back to Qt video format.
6. Frame is pushed to output sink and displayed in UI.

### 3) Capture + OCR Flow

1. User presses F4 or clicks Capture.
2. VideoProcessor::captureAndOCR checks if OCR is already running.
3. Latest valid frame is copied safely.
4. Frame is converted to OpenCV BGR.
5. Screenshot is saved to screenshots directory.
6. Center ROI is extracted.
7. ROI is preprocessed for OCR.
8. runTesseractOcr sends image data to Tesseract.
9. Result text is emitted via signal.
10. ResultDialog displays OCR output.

## Core C++ Functions

- VideoProcessor::VideoProcessor
  - Initializes OCR engine and startup state.

- VideoProcessor::~VideoProcessor
  - Releases Tesseract resources.

- VideoProcessor::setVideoSink
  - Connects camera frame signal to frame handler.

- VideoProcessor::setOutputSink
  - Sets processed frame output target.

- VideoProcessor::setColorMode
  - Updates active monochrome mode.

- VideoProcessor::handleFrame
  - Processes each live frame for preview.

- VideoProcessor::captureAndOCR
  - Executes full capture, preprocessing, OCR, and result emission.

- VideoProcessor::runTesseractOcr
  - Runs OCR on prepared image and returns recognized text.

- VideoProcessor::findTessdataDirectory
  - Locates eng.traineddata from known paths.

- buildMonochromePreview (static helper)
  - Applies grayscale-to-color mapping.

- convertQImageToBgrMat (static helper)
  - Converts QImage to OpenCV BGR matrix.

## Build and Run

### Configure

cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

### Build

cmake --build build --config Release

### Run

Run the generated executable from the build output directory.

## Notes
<img width="1280" height="720" alt="OCR_Capture_20260510_012155_311" src="https://github.com/user-attachments/assets/39a0fa01-22d8-4ba1-a8c9-f077327107a7" />
<img width="1280" height="720" alt="OCR_Capture_20260510_012147_717" src="https://github.com/user-attachments/assets/6fae201f-8cb6-4e25-a646-4d7ad03d0bdf" />
<img width="1280" height="720" alt="OCR_Capture_20260510_012141_296" src="https://github.com/user-attachments/assets/dc3a2cc0-8dd8-4ad9-94ee-ea8014fce217" />
<img width="1280" height="720" alt="OCR_Capture_20260510_012134_152" src="https://github.com/user-attachments/assets/a7e3844e-fc60-4f30-b35c-640bd84c4b61" />

- OCR requires eng.traineddata to be present.
- Tesseract access is mutex-protected because the API is not thread-safe.
- Frame processing and OCR run in background tasks to keep the UI responsive.
