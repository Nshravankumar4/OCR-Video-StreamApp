#ifndef VIDEOPROCESSOR_H
#define VIDEOPROCESSOR_H

#include <QImage>
#include <QMutex>
#include <QObject>
#include <QVideoFrame>
#include <QVideoSink>
#include <opencv2/core.hpp>
#include <atomic>
namespace tesseract {
  class TessBaseAPI;
}

class VideoProcessor : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVideoSink *videoSink READ videoSink WRITE setVideoSink NOTIFY
                 videoSinkChanged)
  Q_PROPERTY(QVideoSink *outputSink READ outputSink WRITE setOutputSink NOTIFY
                 outputSinkChanged)
  Q_PROPERTY(
      int colorMode READ colorMode WRITE setColorMode NOTIFY colorModeChanged)

public:
  explicit VideoProcessor(QObject *parent = nullptr);
  ~VideoProcessor();

  QVideoSink *videoSink() const { return m_videoSink; }
  void setVideoSink(QVideoSink *sink);

  QVideoSink *outputSink() const { return m_outputSink; }
  void setOutputSink(QVideoSink *sink);

  int colorMode() const { return m_colorMode; }
  void setColorMode(int mode);

  // Main capture and OCR flow triggered by F4 key press
  Q_INVOKABLE void captureAndOCR();

signals:
  void videoSinkChanged();
  void outputSinkChanged();
  void colorModeChanged();
  // Emitted when OCR completes, contains formatted result text
  void ocrResultReady(const QString &text);

private slots:
  // Called every time a new frame arrives from the camera
  void handleFrame(const QVideoFrame &frame);

private:
  // Run Tesseract OCR engine on a prepared grayscale image
  QString runTesseractOcr(const QImage &image);
  
  // Find tessdata directory containing eng.traineddata training file
  QString findTessdataDirectory() const;

  QVideoSink *m_videoSink = nullptr;
  QVideoSink *m_outputSink = nullptr;
  int m_colorMode =
      0; // 0: White/Black, 1: Black/White, 2: Black/Green, 3: Black/Yellow
  tesseract::TessBaseAPI *m_tessApi;
  bool m_isTesseractReady;

  QVideoFrame m_currentFrame;              // Latest frame from camera
  mutable QMutex m_frameMutex;             // Protects m_currentFrame
  mutable QMutex m_tessMutex;              // Protects Tesseract API calls
  std::atomic<bool> m_isCapturing{false};  // Prevents concurrent capture/OCR
  std::atomic<bool> m_isProcessing{false}; // Prevents concurrent frame processing
  QString m_lastError;                     // Stores initialization errors
};

#endif // VIDEOPROCESSOR_H
