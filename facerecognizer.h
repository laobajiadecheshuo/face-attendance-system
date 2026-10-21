#ifndef FACERECOGNIZER_H
#define FACERECOGNIZER_H

#include <QObject>
#include <QImage>
#include <QTimer>
#include <QLabel>
#include <QVector>
#include <QString>
#include <QtSql>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "seeta/FaceDetector.h"
#include "seeta/FaceLandmarker.h"
#include "seeta/FaceRecognizer.h"
#include "seeta/QualityAssessor.h"
#include "seeta/QualityOfPoseEx.h"
#include "seeta/QualityOfLBN.h"
#include "seeta/Common/Struct.h"

class FaceRecognitionEngine : public QObject
{
    Q_OBJECT

public:
    // Cached face entry for one registered student
    struct FaceInfo {
        int64_t studID;
        QString name;
        float features[1024];
        QString faceImage;    // path to registered face photo
    };

    // Result of a recognition attempt
    struct RecognizeResult {
        int64_t studID = -1;
        QString name;
        float score = 0.0f;
        QImage snapshot;      // frame snapshot of the recognized face
    };

    explicit FaceRecognitionEngine(QObject *parent = nullptr);
    ~FaceRecognitionEngine();

    // --- Model initialization ---
    bool initModels(const QString &modelDir);

    // --- Feature cache (from studInfo.face_features BLOB) ---
    void loadCache(QSqlDatabase &db);
    void reloadCache(QSqlDatabase &db);
    int  cacheSize() const { return m_cache.size(); }

    // --- Camera ---
    bool openCamera(int id = 0);
    void closeCamera();
    bool cameraIsOpen() const { return m_capture && m_capture->isOpened(); }

    // --- Face detection + feature extraction on current frame ---
    // Returns true if a face was detected and features extracted
    bool detectAndExtract(float *outFeatures, QImage *outCrop = nullptr,
                          QString *errorDetail = nullptr);

    // --- Recognition: compare features against cache, return best match ---
    RecognizeResult recognize(const float *features);

    // --- Registration: check image for valid face ---
    // Returns 0=success, -1=no face, -2=multi faces, -3=quality fail
    int checkImage(const QString &imageFile, const QString &cropFile,
                   float *outFeatures, SeetaRect &outRect,
                   QString *error = nullptr);

    // --- Threshold ---
    float threshold() const { return m_threshold; }
    void  setThreshold(float v) { m_threshold = v; }

    // --- Get current preview frame ---
    QImage currentFrame() const { return m_currentFrame; }

signals:
    void sigFrameReady(const QImage &frame);
    void sigStatusMessage(const QString &msg);

private slots:
    void onTimerTick();    // QTimer-driven frame capture

private:
    QImage cvMatToQImage(const cv::Mat &mat);

    // SeetaFace6 models
    seeta::FaceDetector      *m_fd = nullptr;
    seeta::FaceLandmarker    *m_pd = nullptr;     // 5-point landmarks
    seeta::FaceRecognizer    *m_fr = nullptr;
    seeta::QualityAssessor   *m_qa = nullptr;
    seeta::QualityOfPoseEx   *m_poseex = nullptr;
    seeta::QualityOfLBN      *m_lbn = nullptr;

    // Camera
    cv::VideoCapture *m_capture = nullptr;
    QTimer  *m_timer = nullptr;
    QImage   m_currentFrame;

    // Feature cache
    QVector<FaceInfo> m_cache;

    // Settings
    float m_threshold = 0.65f;
    int   m_minFaceSize = 100;
    int   m_videoWidth = 640;
    int   m_videoHeight = 480;
};

#endif // FACERECOGNIZER_H
