#ifndef VIDEOPROCESSOR_H
#define VIDEOPROCESSOR_H

#include <QImage>
#include <QMutex>
#include <QObject>
#include <QVideoFrame>
#include <QVideoSink>
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

  Q_INVOKABLE void captureAndOCR();

signals:
  void videoSinkChanged();
  void outputSinkChanged();
  void colorModeChanged();
  void ocrResultReady(const QString &text);

private slots:
  void handleFrame(const QVideoFrame &frame);

private:
  QString performOCR(const QImage &image);
  QString resolveTessdataPath() const;

  QVideoSink *m_videoSink = nullptr;
  QVideoSink *m_outputSink = nullptr;
  int m_colorMode =
      -1; // -1: Original camera, 0: White/Black, 1: Black/White, 2: Black/Green, 3: Black/Yellow
  tesseract::TessBaseAPI *m_tessApi;
  bool m_isTesseractReady;

  QVideoFrame m_currentFrame;
  mutable QMutex m_frameMutex;
  mutable QMutex m_tessMutex;
  std::atomic<bool> m_isCapturing{false};
  std::atomic<bool> m_isProcessing{false};
  QString m_lastError;
};

#endif // VIDEOPROCESSOR_H
