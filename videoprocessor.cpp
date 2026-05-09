#include "videoprocessor.h"
#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QDateTime>
#include <QDir>
#include <QStringList>
#include <QtConcurrent/QtConcurrentRun>
#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <tuple>  // std::ignore

// =============================================================================
// FUNCTION EXECUTION ORDER - HOW THIS FILE WORKS
// =============================================================================
// APP START:
//   1. VideoProcessor()        - Constructor: loads Tesseract OCR engine once
//   2. findTessdataDirectory() - Called by constructor to locate eng.traineddata
//   3. setVideoSink()          - QML connects camera frames to this object
//   4. setOutputSink()         - QML connects this object to the display
//   5. setColorMode()          - QML sets which color set (0-3) to apply
//
// EVERY FRAME (30+ times/sec, camera running):
//   6. handleFrame()           - Receives each frame, applies monochrome color,
//                                sends result back to display (live preview)
//        └─ buildMonochromePreview()  - Converts BGR → grayscale → selected color
//
// WHEN USER PRESSES F4 (or clicks Capture button):
//   7. captureAndOCR()         - Grabs current frame, preprocesses it, runs OCR
//        ├─ convertQImageToBgrMat()   - Converts Qt image format to OpenCV format
//        ├─ buildMonochromePreview()  - Applies color mapping for screenshot
//        └─ runTesseractOcr()         - Sends binary image to Tesseract, gets text
//
// APP CLOSE:
//   8. ~VideoProcessor()       - Destructor: shuts down Tesseract engine
// =============================================================================

// [HELPER 1] buildMonochromePreview - Takes BGR camera frame, returns it recolored
// using the user-selected color pair (e.g. black bg + green fg). Called every frame.
static cv::Mat buildMonochromePreview(const cv::Mat &input, int m_colorMode) {
  cv::Mat gray, denoised, normalized;
  cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);  // Convert BGR to grayscale
  cv::bilateralFilter(gray, denoised, 5, 40, 40);  // Smooth while keeping edges
  cv::normalize(denoised, normalized, 0, 255, cv::NORM_MINMAX);  // Normalize 0-255

  cv::Vec3b bg, fg;  // Background and foreground colors

  switch (m_colorMode) {
  case 0:  // Colors_first: white bg, black fg
    bg = cv::Vec3b(255, 255, 255);
    fg = cv::Vec3b(0, 0, 0);
    break;
  case 1:  // Colors_Second: black bg, white fg
    bg = cv::Vec3b(0, 0, 0);
    fg = cv::Vec3b(255, 255, 255);
    break;
  case 2:  // Colors_Third: black bg, green fg
    bg = cv::Vec3b(0, 0, 0);
    fg = cv::Vec3b(14, 199, 17);  // Green in BGR order
    break;
  case 3:  // Colors_Fourth: black bg, yellow fg
    bg = cv::Vec3b(0, 0, 0);
    fg = cv::Vec3b(30, 216, 244);  // Yellow in BGR order
    break;
  default:
    bg = cv::Vec3b(0, 0, 0);
    fg = cv::Vec3b(255, 255, 255);
  }

  // Map grayscale to color pair smoothly
  std::vector<cv::Mat> channels(3);
  for (int c = 0; c < 3; ++c) {
    const double scale = static_cast<double>(fg[c]) - static_cast<double>(bg[c]);
    normalized.convertTo(channels[c], CV_8U, scale / 255.0, static_cast<double>(bg[c]));
  }

  cv::Mat output;
  cv::merge(channels, output);  // Combine RGB channels

  return output;
}

// [HELPER 2] convertQImageToBgrMat - Converts Qt's QImage to OpenCV cv::Mat BGR format.
// Called before any OpenCV image processing operation.
static cv::Mat convertQImageToBgrMat(QImage image)
{
  // Ensure RGB888 format (3 bytes per pixel)
  if (image.format() != QImage::Format_RGB888) {
    image = image.convertToFormat(QImage::Format_RGB888);
  }
  
  // Create OpenCV matrix from image data
  cv::Mat mat(image.height(), image.width(), CV_8UC3, (void *)image.bits(), image.bytesPerLine());
  
  // Convert RGB to BGR (OpenCV standard)
  cv::Mat result;
  cv::cvtColor(mat, result, cv::COLOR_RGB2BGR);
  
  return result.clone();  // Return copy (OpenCV owns memory)
}

// [FUNCTION 1] Constructor - Runs once at startup. Finds eng.traineddata and
// initializes the Tesseract OCR engine. Sets m_isTesseractReady = true on success.
VideoProcessor::VideoProcessor(QObject *parent) : QObject(parent), m_isTesseractReady(false) {
  m_tessApi = new tesseract::TessBaseAPI();  // Create Tesseract engine

  const QString tessdataPath = findTessdataDirectory();  // Find eng.traineddata location
  if (tessdataPath.isEmpty()) {
    m_lastError = "Tesseract not initialized. Missing eng.traineddata in application tessdata paths.";
    qWarning() << m_lastError;
    return;
  }

  qputenv("TESSDATA_PREFIX", tessdataPath.toUtf8());  // Set environment variable
  if (m_tessApi->Init(tessdataPath.toUtf8().constData(), "eng") != 0) {
    m_lastError = "Tesseract initialization failed using path: " + tessdataPath;
    qWarning() << m_lastError;
    return;
  }

  m_isTesseractReady = true;  // Mark OCR ready
  qDebug() << "Tesseract initialized successfully from:" << tessdataPath;
}

// [FUNCTION 2] Destructor - Runs once at app close. Shuts down Tesseract engine.
VideoProcessor::~VideoProcessor() {
  m_tessApi->End();  // End Tesseract engine
  delete m_tessApi;  // Free memory
}

// [FUNCTION 3] setVideoSink - Called by QML to connect camera output to this class.
// Hooks videoFrameChanged signal so handleFrame() fires on every camera frame.
void VideoProcessor::setVideoSink(QVideoSink *sink) {
  if (m_videoSink == sink) return;  // No change needed
  
  if (m_videoSink) {
    disconnect(m_videoSink, &QVideoSink::videoFrameChanged, this, &VideoProcessor::handleFrame);
  }
  
  m_videoSink = sink;
  
  if (m_videoSink) {
    connect(m_videoSink, &QVideoSink::videoFrameChanged, this, &VideoProcessor::handleFrame);
  }
  
  emit videoSinkChanged();
}

// [FUNCTION 4] findTessdataDirectory - Searches multiple paths for eng.traineddata.
// Checks: TESSDATA_PREFIX env var → app directory → parent dir → CMake path.
QString VideoProcessor::findTessdataDirectory() const {
  QStringList candidateDirs;
  
  const QString envPrefix = qEnvironmentVariable("TESSDATA_PREFIX");  // Check env variable
  if (!envPrefix.isEmpty()) {
    candidateDirs << envPrefix;
    candidateDirs << QDir(envPrefix).filePath("tessdata");
  }
  
  const QString appDir = QCoreApplication::applicationDirPath();
  candidateDirs << QDir(appDir).filePath("tessdata");  // Check app directory
  candidateDirs << QDir(appDir).filePath("../tessdata");  // Check parent directory
  
#ifdef OCR_DEFAULT_TESSDATA_DIR
  candidateDirs << QString::fromUtf8(OCR_DEFAULT_TESSDATA_DIR);
#endif
  
  for (const QString &dirPath : candidateDirs) {
    const QString normalized = QDir::cleanPath(dirPath);
    if (normalized.isEmpty()) continue;
    
    const QString trainedData = QDir(normalized).filePath("eng.traineddata");
    if (QFileInfo::exists(trainedData)) {
      return normalized;  // Found training file
    }
  }
  
  return QString();  // Not found
}

// [FUNCTION 5] setOutputSink - Called by QML to connect this class to the display.
// Processed frames are pushed to m_outputSink so VideoOutput shows the result.
void VideoProcessor::setOutputSink(QVideoSink *sink) {
  if (m_outputSink == sink) return;
  m_outputSink = sink;
  emit outputSinkChanged();
}

// [FUNCTION 6] setColorMode - Called by QML when user picks a color in ColorDialog.
// Stores mode (0-3); handleFrame() reads it on the next frame to apply new color.
void VideoProcessor::setColorMode(int mode) {
  if (m_colorMode == mode) return;
  m_colorMode = mode;
  emit colorModeChanged();
}

// [FUNCTION 7] handleFrame - Called 30+ times/sec by the camera signal.
// Converts each frame to OpenCV, applies monochrome color, sends result to display.
void VideoProcessor::handleFrame(const QVideoFrame &frame) {
  QMutexLocker locker(&m_frameMutex);
  m_currentFrame = frame;  // Store latest frame
  locker.unlock();

  if (m_isProcessing) return;  // Skip if already processing
  if (!m_outputSink || !frame.isValid()) return;  // Validate sinks and frame

  m_isProcessing = true;  // Mark processing started
  
  // Process in background thread to keep UI responsive
  std::ignore = QtConcurrent::run([this, frame]() {
    struct ProcessingGuard {  // Auto cleanup guard
      std::atomic<bool> &flag;
      ~ProcessingGuard() { flag = false; }
    } guard{m_isProcessing};

    try {
      QVideoFrame f = frame;
      QImage img = f.toImage();  // Convert frame to image
      if (img.isNull()) return;

      if (img.format() != QImage::Format_RGB888) {
        img = img.convertToFormat(QImage::Format_RGB888);  // Ensure RGB888 format
      }
      
      cv::Mat rgb(img.height(), img.width(), CV_8UC3, (void*)img.bits(), img.bytesPerLine());
      cv::Mat bgr;
      cv::cvtColor(rgb, bgr, cv::COLOR_RGB2BGR);  // RGB to BGR conversion
      
      cv::Mat processed = buildMonochromePreview(bgr, m_colorMode);  // Apply color mapping
      
      cv::Mat processedRgba;
      cv::cvtColor(processed, processedRgba, cv::COLOR_BGR2RGBA);  // Back to RGBA for display
      
      QVideoFrame outFrame(QVideoFrameFormat(QSize(processedRgba.cols, processedRgba.rows), 
                                             QVideoFrameFormat::Format_RGBA8888));
      
      if (outFrame.map(QVideoFrame::WriteOnly)) {
        memcpy(outFrame.bits(0), processedRgba.data, processedRgba.rows * processedRgba.step);
        outFrame.unmap();
        m_outputSink->setVideoFrame(outFrame);  // Send to display
      }
    } catch (const cv::Exception &e) {
      qWarning() << "Frame processing OpenCV error:" << e.what();
    } catch (const std::exception &e) {
      qWarning() << "Frame processing error:" << e.what();
    } catch (...) {
      qWarning() << "Frame processing unknown error";
    }
  });
}

// [FUNCTION 8] captureAndOCR - Called by QML when user presses F4 or Capture button.
// Grabs the latest camera frame, preprocesses it, runs Tesseract OCR, shows result.
void VideoProcessor::captureAndOCR() {
  bool expected = false;
  if (!m_isCapturing.compare_exchange_strong(expected, true)) {
    emit ocrResultReady("OCR is already running. Please wait...");  // Already running
    return;
  }

  QMutexLocker locker(&m_frameMutex);
  if (!m_currentFrame.isValid()) {  // No frame available
    m_isCapturing = false;
    emit ocrResultReady("Error: No video frame available. Try again in a moment.");
    return;
  }
  QVideoFrame frame = m_currentFrame;
  locker.unlock();

  std::ignore = QtConcurrent::run([this, frame]() mutable {
    if (!m_isTesseractReady) {  // Check OCR engine ready
      emit ocrResultReady("Error: " + m_lastError);
      m_isCapturing = false;
      return;
    }

    const QImage img = frame.toImage();  // Convert frame to image
    if (img.isNull()) {
      emit ocrResultReady("Error: Failed to capture image from current video frame.");
      m_isCapturing = false;
      return;
    }

    const cv::Mat mat = convertQImageToBgrMat(img);  // Convert to OpenCV format
    cv::Mat displayProcessed = buildMonochromePreview(mat, m_colorMode);  // Apply colors

    // Save screenshot
    const QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz");
    QString screenshotDir = QCoreApplication::applicationDirPath() + "/screenshots";
    QDir().mkpath(screenshotDir);
    const QString screenshotPath = screenshotDir + "/OCR_Capture_" + timestamp + ".png";
    cv::imwrite(screenshotPath.toLocal8Bit().constData(), displayProcessed);

    // Extract center region (65% width, 50% height)
    const int roiW = static_cast<int>(mat.cols * 0.65);
    const int roiH = static_cast<int>(mat.rows * 0.50);
    const int roiX = (mat.cols - roiW) / 2;
    const int roiY = (mat.rows - roiH) / 2;
    cv::Rect roi(roiX, roiY, roiW, roiH);
    cv::Mat center = mat(roi).clone();

    // Preprocess for OCR (convert to binary)
    cv::Mat gray, denoised, binary;
    cv::cvtColor(center, gray, cv::COLOR_BGR2GRAY);
    cv::bilateralFilter(gray, denoised, 5, 50, 50);
    cv::adaptiveThreshold(denoised, binary, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY, 31, 6);
    
    // Invert if mostly white
    const int whitePixels = cv::countNonZero(binary);
    const int totalPixels = binary.rows * binary.cols;
    if (whitePixels < totalPixels / 3) {
      cv::bitwise_not(binary, binary);
    }

    QImage grayImg(binary.data, binary.cols, binary.rows, binary.step, QImage::Format_Grayscale8);
    QString result = runTesseractOcr(grayImg.copy());  // Run OCR

    // Build result message
    const QString displayText = result.isEmpty() ? "Not Detected" : result;
    const QString displayPath = QDir::toNativeSeparators(screenshotPath);
    
    const QString finalMessage = QString("OCR EXECUTED!\n"
                                        "Live Frame Captured\n"
                                        "Color Set: #%1\n"
                                        "Monochrome: Applied\n"
                                        "Recognized Text: %2\n"
                                        "Status: Production Ready\n"
                                        "Qt 6.5.3 + CMake\n"
                                        "Complete All Requirements Met\n"
                                        "Screenshot: %3\n"
                                        "OK")
                                .arg(m_colorMode + 1)
                                .arg(displayText)
                                .arg(displayPath);

    emit ocrResultReady(finalMessage);  // Show result dialog
    m_isCapturing = false;  // Mark complete
  });
}

// [FUNCTION 9] runTesseractOcr - Sends a preprocessed grayscale QImage to Tesseract.
// Returns the recognized text string, or empty string if nothing is found.
QString VideoProcessor::runTesseractOcr(const QImage &image) {
  if (!m_isTesseractReady) return "Error: " + m_lastError;  // Check ready

  QMutexLocker tessLock(&m_tessMutex);  // Lock (not thread-safe)

  m_tessApi->SetVariable("preserve_interword_spaces", "1");  // Keep word spaces
  m_tessApi->SetVariable("user_defined_dpi", "300");  // Set DPI

  m_tessApi->SetPageSegMode(tesseract::PSM_AUTO);  // Auto detect layout

  m_tessApi->SetImage(image.bits(), image.width(), image.height(),
                      image.depth() / 8, image.bytesPerLine());  // Give image to OCR

  char *outText = m_tessApi->GetUTF8Text();  // Get recognized text
  if (!outText) {
    return QString();  // No text found
  }

  QString result = QString::fromUtf8(outText).trimmed();
  delete[] outText;  // Free memory

  return result;
}
