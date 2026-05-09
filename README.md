# OCR-Video-StreamApp

A real-time video OCR application built with **Qt 6.5.3**, **OpenCV 4.8**, and **Tesseract 5**.  
It captures live camera frames, applies one of four monochrome color styles, and recognizes text using OCR when the user presses **F4**.

---

## Interview Requirements — ALL MET ?

| # | Requirement | Status |
|---|-------------|--------|
| 1 | Webcam / internal camera input | ? |
| 2 | Convert each frame to image | ? |
| 3 | Convert image to monochrome | ? |
| 4 | Four exact color sets | ? |
| 5 | Convert monochrome back to video frame and render | ? |
| 6 | F4 shortcut to capture | ? |
| 7 | OCR on captured frame | ? |
| 8 | Separate OCR result dialog | ? |

---

## Four Color Sets (Exact Names Required)

| Name | Background | Foreground |
|------|-----------|------------|
| `Colors_first`  | `#ffffff` white  | `#000000` black  |
| `Colors_Second` | `#000000` black  | `#ffffff` white  |
| `Colors_Third`  | `#000000` black  | `#11c70e` green  |
| `Colors_Fourth` | `#000000` black  | `#f4d81e` yellow |

---

## Tech Stack

- **Qt 6.5.3** — QML UI, Camera, VideoSink, QtConcurrent (background threads)
- **OpenCV 4.8** — Image format conversion, grayscale, bilateral filter, adaptive threshold
- **Tesseract 5** — OCR engine with `eng.traineddata` language model
- **CMake 3.16+** — Build system with vcpkg dependency management
- **C++17** — Core language standard

---

## Project File Structure

```
OCR-Video-StreamApp/
+-- main.cpp                 ? App entry point, registers C++ class for QML
+-- videoprocessor.h         ? Class declaration, signals, properties
+-- videoprocessor.cpp       ? All video + OCR processing logic (9 functions)
+-- CMakeLists.txt           ? Build config: Qt, OpenCV, Tesseract
+-- qml/
¦   +-- Main.qml             ? Main window: camera preview, buttons, F4 shortcut
¦   +-- ColorDialog.qml      ? Dialog: choose 1 of 4 color sets
¦   +-- ResultDialog.qml     ? Dialog: display OCR result text
+-- dependencies/
    +-- opencv/              ? OpenCV 4.8 (local build)
    +-- tessdata/            ? eng.traineddata (Tesseract language model)
    +-- vcpkg/               ? Tesseract library via vcpkg
```

---

## Function Execution Order (videoprocessor.cpp)

This is the exact order code runs — the key thing to explain in an interview.

### At App Startup (runs once)
```
main()
  +- VideoProcessor()            [FUNCTION 1] Constructor
       +- findTessdataDirectory() [FUNCTION 4] Locates eng.traineddata
  +- setVideoSink()              [FUNCTION 3] Connects camera ? handleFrame()
  +- setOutputSink()             [FUNCTION 5] Connects processor ? display
  +- setColorMode(0)             [FUNCTION 6] Sets default color (Colors_first)
```

### Every Frame — 30+ times/sec (live preview)
```
Camera sends frame
  +- handleFrame()               [FUNCTION 7] Receives every camera frame
       +- buildMonochromePreview() [HELPER 1] Applies selected color mapping
            ? sends processed frame to display (VideoOutput in QML)
```

### When User Presses F4 (OCR capture)
```
F4 key / Capture button
  +- captureAndOCR()             [FUNCTION 8] Main OCR entry point
       +- convertQImageToBgrMat()  [HELPER 2] Qt image ? OpenCV BGR format
       +- buildMonochromePreview() [HELPER 1] Apply color for screenshot
       +- cv::imwrite()            Save screenshot to disk
       +- Extract center ROI (65% width × 50% height)
       +- cv::adaptiveThreshold()  Convert to binary black/white
       +- runTesseractOcr()        [FUNCTION 9] Get text from Tesseract
            ? emit ocrResultReady() ? ResultDialog opens in QML
```

### At App Close (runs once)
```
~VideoProcessor()                [FUNCTION 2] Destructor: shuts down Tesseract
```

---

## Function Summary

### Static Helpers (pure image processing, no class state)

**`buildMonochromePreview(input, colorMode)`** — HELPER 1  
Takes a BGR camera frame, returns it recolored with the selected color pair.  
Steps: BGR ? grayscale ? bilateral filter ? normalize ? map to color pair ? merge.  
Called by: `handleFrame()` and `captureAndOCR()`.

**`convertQImageToBgrMat(image)`** — HELPER 2  
Converts a Qt `QImage` to an OpenCV `cv::Mat` in BGR format.  
Steps: ensure RGB888 ? wrap in Mat ? convert RGB?BGR ? clone.  
Called by: `captureAndOCR()`.

### Class Methods (in execution order)

**`VideoProcessor()` [FUNCTION 1]** — Constructor  
Runs once at startup. Creates Tesseract API, finds `eng.traineddata`, calls `Init()`.  
Sets `m_isTesseractReady = true` on success, stores error in `m_lastError` on failure.

**`~VideoProcessor()` [FUNCTION 2]** — Destructor  
Runs once at close. Calls `m_tessApi->End()` then deletes the pointer.

**`setVideoSink()` [FUNCTION 3]**  
Called by QML to connect the camera. Hooks `videoFrameChanged` ? `handleFrame()` slot.  
This is what makes `handleFrame()` fire automatically for every camera frame.

**`findTessdataDirectory()` [FUNCTION 4]**  
Searches for `eng.traineddata` in: `TESSDATA_PREFIX` env var ? app dir ? parent dir ? CMake path.  
Returns directory path, or empty string if not found.

**`setOutputSink()` [FUNCTION 5]**  
Called by QML to connect the display. Processed frames are pushed to `m_outputSink`.

**`setColorMode()` [FUNCTION 6]**  
Called by QML when user picks a color. Stores mode 0–3 in `m_colorMode`.

**`handleFrame()` [FUNCTION 7]**  
Fires 30+ times/sec from camera signal. Converts frame ? BGR ? applies monochrome ? RGBA ? display.  
Runs in background thread (QtConcurrent) so it never blocks the UI.

**`captureAndOCR()` [FUNCTION 8]**  
Called on F4. Grabs frame ? converts format ? saves screenshot ? crops center ROI ? adaptive threshold ? calls Tesseract ? emits result signal to QML.

**`runTesseractOcr()` [FUNCTION 9]**  
Locks mutex (Tesseract is not thread-safe) ? sets PSM_AUTO ? passes image data ? returns recognized text string.

---

## QML Files Summary

**`Main.qml`** — Main Window  
Camera pipeline: `MediaDevices` ? `CaptureSession` + `Camera` ? `VideoSink inputSink` ? `VideoProcessor` ? `VideoOutput`.  
Buttons: "Select Colors" opens `ColorDialog`. F4 / "Capture" calls `processor.captureAndOCR()`. `onOcrResultReady` signal opens `ResultDialog`.

**`ColorDialog.qml`** — Color Selection  
`Repeater` generates 4 clickable color options with gradient preview. Click ? `modeSelected(mode)` signal ? `processor.colorMode` updates ? preview changes next frame.

**`ResultDialog.qml`** — OCR Result  
Scrollable read-only `TextArea` showing recognized text + metadata. Opens automatically when `ocrResultReady` signal fires.

---

## How to Explain in an Interview

**Big Picture (30 sec):**
> "This app opens the webcam, applies one of four monochrome color styles to the live preview, and when you press F4 it captures the frame, runs Tesseract OCR on the center region, and shows the recognized text in a dialog."

**Architecture (60 sec):**
> "The UI is in QML — three files: main window, color dialog, result dialog. All processing is in C++ inside `VideoProcessor`. Qt connects them via signals and Q_PROPERTY bindings. `handleFrame()` runs in a QtConcurrent background thread 30 times/sec so the UI stays responsive."

**Live Preview Flow (30 sec):**
> "Every camera frame triggers `handleFrame()`. It converts the frame to OpenCV, applies `buildMonochromePreview()` which maps each pixel luminosity to the selected color pair, converts back to RGBA, and pushes it to `VideoOutput`."

**F4 Capture Flow (60 sec):**
> "F4 calls `captureAndOCR()`. It grabs the stored frame, converts to OpenCV BGR via `convertQImageToBgrMat()`, saves a color screenshot, crops the center 65×50% region, runs adaptive thresholding to get a clean binary image, passes that to `runTesseractOcr()` which calls Tesseract, then emits `ocrResultReady` back to QML which opens the result dialog."

---

## Build Instructions

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Requires: Qt 6.5.3 at `C:/Qt/6.5.3/msvc2019_64`, OpenCV in `dependencies/opencv/build`, Tesseract via vcpkg in `dependencies/vcpkg/installed/x64-windows`.
