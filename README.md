# OCR Video Stream App (Qt + QML + OpenCV + Tesseract)

A simple multimedia application that:
- Reads live camera video (webcam/internal camera)
- Converts each frame to monochrome with user-selected color sets
- Renders processed video in real time
- Captures a frame on `F4`
- Runs OCR on the captured frame
- Shows recognized text in a separate dialog

This project is designed to match the interview task requirements.

## 1) What This Project Does

### Live video pipeline
1. Camera frame is received in QML and forwarded to C++ (`VideoProcessor`).
2. Frame is converted to OpenCV image (`cv::Mat`).
3. If a color mode is selected, frame is converted to monochrome and recolored.
4. Processed frame is sent back to QML and displayed.

### OCR flow
1. User presses `F4` (or clicks Capture).
2. Current frame is captured and saved as a PNG.
3. Frame is preprocessed for OCR.
4. Tesseract recognizes text.
5. Result is displayed in a dialog.

## 2) Color Set Mapping (Task-Exact)

The dialog offers these exact sets:

- `Colors_first`  : `#ffffff`, `#000000`
- `Colors_Second` : `#000000`, `#ffffff`
- `Colors_Third`  : `#000000`, `#11c70e`
- `Colors_Fourth` : `#000000`, `#f4d81e`

Notes:
- Monochrome color conversion is applied to the **full frame** (not face-only).
- This is correct according to the wording: "Convert image to monochrome image".

## 3) Project Structure

- `main.cpp` - App startup, QML module loading
- `videoprocessor.h/.cpp` - Frame processing + OCR logic
- `qml/Main.qml` - Main UI, camera, controls, shortcut
- `qml/ColorDialog.qml` - Color set chooser dialog
- `qml/ResultDialog.qml` - OCR text result dialog
- `CMakeLists.txt` - Build configuration
- `dependencies/` - Local project dependencies

## 4) Requirements

- Windows (currently configured and tested)
- Qt 6.5.3 (msvc2019_64)
- CMake 3.16+
- OpenCV (included under `dependencies/opencv`)
- Tesseract + tessdata (project-local under `dependencies/vcpkg/installed/x64-windows`)

## 5) Build and Run

### Option A: Qt Creator (recommended)
1. Open folder: `D:/Linux/Isolvetest`
2. Select kit: Qt 6.5.3 + MSVC x64
3. Configure project
4. Build target: `appOCRVideoApp`
5. Run

### Option B: Command line
```powershell
cmake -S D:/Linux/Isolvetest -B D:/Linux/build-Isolvetest-HMI_Win_64-Debug
cmake --build D:/Linux/build-Isolvetest-HMI_Win_64-Debug --target appOCRVideoApp
```

## 6) How to Use

1. Launch app.
2. Default mode is original camera view.
3. Click **Color Sets** and choose one mode.
4. Press `F4` to capture and run OCR.
5. OCR result appears in the result dialog.
6. Captured image is saved in build output:
   - `.../app folder/screenshots/OCR_Capture_yyyyMMdd_HHmmss_zzz.png`

## 7) Interview Requirement Mapping

- Webcam/internal camera input: ✅
- Frame -> image conversion: ✅
- Monochrome conversion: ✅
- Color selection dialog with 4 sets: ✅
- Render monochrome frame: ✅
- `F4` capture shortcut: ✅
- OCR on captured image: ✅
- Separate OCR result dialog: ✅

## 8) Troubleshooting

### A) "No text recognized in this frame"
- OCR needs clear, readable text in camera frame.
- Test using printed document or phone screen text.
- Keep text in center and hold steady before pressing `F4`.

### B) Tesseract initialization error
- Ensure `eng.traineddata` exists in one of:
  - app runtime `tessdata/`
  - `dependencies/tessdata/`
  - `dependencies/vcpkg/installed/x64-windows/share/tessdata/`

### C) `QtConcurrent::run` not found
- Ensure source includes:
  - `#include <QtConcurrent/QtConcurrentRun>`
- Ensure `Qt6::Concurrent` is linked in CMake.

### D) `type_traits` not found (MSVC)
- This is a compiler environment issue (not app logic).
- Use a proper MSVC Qt kit in Qt Creator, or open a Developer Command Prompt.

## 9) Notes

- This code is intentionally kept straightforward for interview explanation.
- Advanced OCR filtering was removed to keep flow easy to explain.
- Current behavior prioritizes clarity and requirement match over heavy optimization.
"# OCR-Video-Stream-App" 
# OCR-Video-StreamApp
