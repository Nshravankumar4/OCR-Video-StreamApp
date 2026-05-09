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

static cv::Mat processFrameData(const cv::Mat &input, int m_colorMode) {
  if (m_colorMode < 0) {
    // Default mode: show original camera frame (no monochrome mapping).
    return input.clone();
  }

  cv::Mat gray, enhanced, smoothed, binary;
  cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
  cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
  clahe->apply(gray, enhanced);
  cv::GaussianBlur(enhanced, smoothed, cv::Size(3, 3), 0.0);
  cv::adaptiveThreshold(smoothed, binary, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                        cv::THRESH_BINARY, 31, 6);

  cv::Vec3b bg, fg;

  switch (m_colorMode) {
  case 0: // Colors_first (#ffffff, #000000)
    bg = cv::Vec3b(255, 255, 255);
    fg = cv::Vec3b(0, 0, 0);
    break;
  case 1: // Colors_Second (#000000, #ffffff)
    bg = cv::Vec3b(0, 0, 0);
    fg = cv::Vec3b(255, 255, 255);
    break;
  case 2: // Colors_Third (#000000, #11c70e)
    bg = cv::Vec3b(0, 0, 0);
    fg = cv::Vec3b(14, 199, 17); // #11c70e is RGB, OpenCV is BGR: 0e, c7, 11
    break;
  case 3: // Colors_Fourth (#000000, #f4d81e)
    bg = cv::Vec3b(0, 0, 0);
    fg = cv::Vec3b(30, 216, 244); // #f4d81e is RGB, OpenCV is BGR: 1e, d8, f4
    break;
  default:
    bg = cv::Vec3b(0, 0, 0);
    fg = cv::Vec3b(255, 255, 255);
  }

  cv::Mat output(binary.size(), CV_8UC3, bg);
  output.setTo(fg, binary);

  return output;
}

static cv::Mat preprocessForOCR(const cv::Mat &input)
{
  cv::Mat gray, denoised, binary;
  cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
  cv::bilateralFilter(gray, denoised, 5, 50, 50);
  cv::adaptiveThreshold(denoised, binary, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                        cv::THRESH_BINARY, 31, 6);

  const int whitePixels = cv::countNonZero(binary);
  const int totalPixels = binary.rows * binary.cols;
  if (whitePixels < totalPixels / 3) {
    cv::bitwise_not(binary, binary);
  }

  return binary;
}

VideoProcessor::VideoProcessor(QObject *parent) : QObject(parent), m_isTesseractReady(false) {
  m_tessApi = new tesseract::TessBaseAPI();

  const QString tessdataPath = resolveTessdataPath();
  if (tessdataPath.isEmpty()) {
    m_lastError = "Tesseract not initialized. Missing eng.traineddata in application tessdata paths.";
    qWarning() << m_lastError;
    return;
  }

  qputenv("TESSDATA_PREFIX", tessdataPath.toUtf8());
  if (m_tessApi->Init(tessdataPath.toUtf8().constData(), "eng") != 0) {
    m_lastError = "Tesseract initialization failed using path: " + tessdataPath;
    qWarning() << m_lastError;
    return;
  }

  m_isTesseractReady = true;
  qDebug() << "Tesseract initialized successfully from:" << tessdataPath;
}

VideoProcessor::~VideoProcessor() {
  m_tessApi->End();
  delete m_tessApi;
}

QString VideoProcessor::resolveTessdataPath() const
{
  QStringList candidateDirs;

  const QString envPrefix = qEnvironmentVariable("TESSDATA_PREFIX");
  if (!envPrefix.isEmpty()) {
    candidateDirs << envPrefix;
    candidateDirs << QDir(envPrefix).filePath("tessdata");
  }

  const QString appDir = QCoreApplication::applicationDirPath();
  candidateDirs << QDir(appDir).filePath("tessdata");
  candidateDirs << QDir(appDir).filePath("../tessdata");

#ifdef OCR_DEFAULT_TESSDATA_DIR
  candidateDirs << QString::fromUtf8(OCR_DEFAULT_TESSDATA_DIR);
#endif

  for (const QString &dirPath : candidateDirs) {
    const QString normalized = QDir::cleanPath(dirPath);
    if (normalized.isEmpty()) {
      continue;
    }

    const QString trainedData = QDir(normalized).filePath("eng.traineddata");
    if (QFileInfo::exists(trainedData)) {
      return normalized;
    }
  }

  return QString();
}

void VideoProcessor::setVideoSink(QVideoSink *sink) {
  if (m_videoSink == sink)
    return;

  if (m_videoSink) {
    disconnect(m_videoSink, &QVideoSink::videoFrameChanged, this,
               &VideoProcessor::handleFrame);
  }

  m_videoSink = sink;

  if (m_videoSink) {
    connect(m_videoSink, &QVideoSink::videoFrameChanged, this,
            &VideoProcessor::handleFrame);
  }

  emit videoSinkChanged();
}

void VideoProcessor::setOutputSink(QVideoSink *sink) {
  if (m_outputSink == sink)
    return;
  m_outputSink = sink;
  emit outputSinkChanged();
}

void VideoProcessor::setColorMode(int mode) {
  if (m_colorMode == mode)
    return;
  m_colorMode = mode;
  emit colorModeChanged();
}

void VideoProcessor::handleFrame(const QVideoFrame &frame) {
  QMutexLocker locker(&m_frameMutex);
  m_currentFrame = frame;
  locker.unlock();

  if (m_isProcessing) return; // Still busy with last frame

  if (!m_outputSink || !frame.isValid())
    return;

  m_isProcessing = true;
  
  // Process in background thread to keep GUI smooth
  QtConcurrent::run([this, frame]() {
    QVideoFrame f = frame;
    QImage img = f.toImage();
    if (!img.isNull()) {
        // Ensure image is in a format OpenCV likes (RGB888)
        if (img.format() != QImage::Format_RGB888) {
            img = img.convertToFormat(QImage::Format_RGB888);
        }
        
        cv::Mat rgb(img.height(), img.width(), CV_8UC3, (void*)img.bits(), img.bytesPerLine());
        cv::Mat bgr;
        // Clone to ensure we own the memory before modifying
        cv::cvtColor(rgb, bgr, cv::COLOR_RGB2BGR);
        
        cv::Mat processed = processFrameData(bgr, m_colorMode);
        
        cv::Mat processedX;
        cv::cvtColor(processed, processedX, cv::COLOR_BGR2RGBA);
        
        QVideoFrame outFrame(QVideoFrameFormat(QSize(processedX.cols, processedX.rows), QVideoFrameFormat::Format_RGBA8888));
        if (outFrame.map(QVideoFrame::WriteOnly)) {
          memcpy(outFrame.bits(0), processedX.data, processedX.rows * processedX.step);
          outFrame.unmap();
          m_outputSink->setVideoFrame(outFrame);
        }
    }
    m_isProcessing = false;
  });
}


void VideoProcessor::captureAndOCR() {
  bool expected = false;
  if (!m_isCapturing.compare_exchange_strong(expected, true)) {
    return;
  }

  QMutexLocker locker(&m_frameMutex);
  if (!m_currentFrame.isValid()) {
    m_isCapturing = false;
    return;
  }

  QVideoFrame frame = m_currentFrame;
  locker.unlock();

  QtConcurrent::run([this, frame]() mutable {
    if (!m_isTesseractReady) {
      emit ocrResultReady("Error: " + m_lastError);
      m_isCapturing = false;
      return;
    }

    QImage img = frame.toImage();
    if (img.isNull()) {
      emit ocrResultReady("Error: Failed to capture image from current video frame.");
      m_isCapturing = false;
      return;
    }

    // Convert QImage to cv::Mat
    cv::Mat mat;
    switch (img.format()) {
    case QImage::Format_RGB888:
      mat = cv::Mat(img.height(), img.width(), CV_8UC3, (void *)img.bits(),
                    img.bytesPerLine());
      cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
      break;
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
      mat = cv::Mat(img.height(), img.width(), CV_8UC4, (void *)img.bits(),
                    img.bytesPerLine());
      cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR);
      break;
    default:
      img = img.convertToFormat(QImage::Format_RGB888);
      mat = cv::Mat(img.height(), img.width(), CV_8UC3, (void *)img.bits(),
                    img.bytesPerLine());
      cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
      break;
    }

    // Save what user sees (selected monochrome color set) for debugging/reference.
    cv::Mat displayProcessed = processFrameData(mat, m_colorMode);
    const QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz");
    
    // Save into a 'screenshots' folder inside the build directory
    QString screenshotDir = QCoreApplication::applicationDirPath() + "/screenshots";
    QDir().mkpath(screenshotDir); // Ensure the folder exists
    
    QString imagePath = screenshotDir + "/OCR_Capture_" + timestamp + ".png";
    cv::imwrite(imagePath.toLocal8Bit().constData(), displayProcessed);
    
    qDebug() << "Screenshot saved to:" << imagePath;

    // OCR first on center ROI, then fallback to full frame.
    const int roiW = static_cast<int>(mat.cols * 0.8);
    const int roiH = static_cast<int>(mat.rows * 0.7);
    const int roiX = (mat.cols - roiW) / 2;
    const int roiY = (mat.rows - roiH) / 2;
    const cv::Rect roi(roiX, roiY, roiW, roiH);
    cv::Mat center = mat(roi).clone();

    cv::Mat binaryCenter = preprocessForOCR(center);
    QString result = performOCR(QImage(binaryCenter.data, binaryCenter.cols, binaryCenter.rows,
                                       binaryCenter.step, QImage::Format_Grayscale8).copy());

    if (result.trimmed().isEmpty()) {
      cv::Mat invertedCenter;
      cv::bitwise_not(binaryCenter, invertedCenter);
      result = performOCR(QImage(invertedCenter.data, invertedCenter.cols, invertedCenter.rows,
                                 invertedCenter.step, QImage::Format_Grayscale8).copy());
    }

    if (result.trimmed().isEmpty()) {
      cv::Mat binaryFull = preprocessForOCR(mat);
      result = performOCR(QImage(binaryFull.data, binaryFull.cols, binaryFull.rows,
                                 binaryFull.step, QImage::Format_Grayscale8).copy());
    }

    if (result.trimmed().isEmpty()) {
      result = "No text recognized in this frame. Point camera to a document/screen text and hold steady. Screenshot saved to: " + imagePath;
    } else {
      result += "\n\nSaved image: " + imagePath;
    }
    emit ocrResultReady(result);
    m_isCapturing = false;
  });
}

QString VideoProcessor::performOCR(const QImage &image) {
  if (!m_isTesseractReady) return "Error: " + m_lastError;

  QMutexLocker tessLock(&m_tessMutex);
  m_tessApi->SetPageSegMode(tesseract::PSM_AUTO);
  m_tessApi->SetVariable("preserve_interword_spaces", "1");
  m_tessApi->SetImage(image.bits(), image.width(), image.height(),
                      image.depth() / 8, image.bytesPerLine());
  char *outText = m_tessApi->GetUTF8Text();
  QString result;
  if (outText) {
    result = QString::fromUtf8(outText).trimmed();
    delete[] outText;
  }
  return result;
}
