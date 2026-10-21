#include "facerecognizer.h"
#include <QFile>
#include <QDebug>

// Unicode-safe imread (same pattern as demo)
static cv::Mat imreadUnicode(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return cv::Mat();
    QByteArray buf = file.readAll();
    return cv::imdecode(cv::Mat(1, buf.size(), CV_8UC1, buf.data()), cv::IMREAD_COLOR);
}

FaceRecognitionEngine::FaceRecognitionEngine(QObject *parent)
    : QObject(parent)
{
}

FaceRecognitionEngine::~FaceRecognitionEngine()
{
    closeCamera();
    delete m_qa;    m_qa = nullptr;
    delete m_poseex; m_poseex = nullptr;
    delete m_lbn;   m_lbn = nullptr;
    delete m_fr;    m_fr = nullptr;
    delete m_pd;    m_pd = nullptr;
    delete m_fd;    m_fd = nullptr;
    qDebug() << "FaceRecognitionEngine released";
}

// ========== Model Initialization ==========

bool FaceRecognitionEngine::initModels(const QString &modelDir)
{
    std::string gpath = modelDir.toStdString();

    // 1. Face Detector
    seeta::ModelSetting fd_model;
    fd_model.append(gpath + "face_detector.csta");
    fd_model.set_device(seeta::ModelSetting::CPU);
    fd_model.set_id(0);
    m_fd = new seeta::FaceDetector(fd_model);
    m_fd->set(seeta::FaceDetector::PROPERTY_MIN_FACE_SIZE, m_minFaceSize);

    // 2. Face Landmarker (5-point)
    seeta::ModelSetting pd_model;
    pd_model.append(gpath + "face_landmarker_pts5.csta");
    pd_model.set_device(seeta::ModelSetting::CPU);
    pd_model.set_id(0);
    m_pd = new seeta::FaceLandmarker(pd_model);

    // 3. Face Recognizer
    seeta::ModelSetting fr_model;
    fr_model.append(gpath + "face_recognizer.csta");
    fr_model.set_device(seeta::ModelSetting::CPU);
    fr_model.set_id(0);
    m_fr = new seeta::FaceRecognizer(fr_model);

    // 4. Pose estimator
    seeta::ModelSetting posemodel;
    posemodel.set_device(SEETA_DEVICE_CPU);
    posemodel.set_id(0);
    posemodel.append(gpath + "pose_estimation.csta");
    m_poseex = new seeta::QualityOfPoseEx(posemodel);
    m_poseex->set(seeta::QualityOfPoseEx::YAW_LOW_THRESHOLD, 20);
    m_poseex->set(seeta::QualityOfPoseEx::YAW_HIGH_THRESHOLD, 10);
    m_poseex->set(seeta::QualityOfPoseEx::PITCH_LOW_THRESHOLD, 20);
    m_poseex->set(seeta::QualityOfPoseEx::PITCH_HIGH_THRESHOLD, 10);

    // 5. LBN (blur/noise)
    seeta::ModelSetting lbnmodel;
    lbnmodel.set_device(SEETA_DEVICE_CPU);
    lbnmodel.set_id(0);
    lbnmodel.append(gpath + "quality_lbn.csta");
    m_lbn = new seeta::QualityOfLBN(lbnmodel);
    m_lbn->set(seeta::QualityOfLBN::PROPERTY_BLUR_THRESH, 0.80);

    // 6. Quality Assessor
    m_qa = new seeta::QualityAssessor();
    m_qa->add_rule(seeta::INTEGRITY);
    m_qa->add_rule(seeta::RESOLUTION);
    m_qa->add_rule(seeta::BRIGHTNESS);
    m_qa->add_rule(seeta::CLARITY);
    m_qa->add_rule(seeta::POSE_EX, m_poseex, true);

    qDebug() << "FaceRecognitionEngine initialized, models from:" << modelDir;
    return true;
}

// ========== Feature Cache ==========

void FaceRecognitionEngine::loadCache(QSqlDatabase &db)
{
    m_cache.clear();

    QSqlQuery q(db);
    q.exec("SELECT studID, name, face_features, face_image FROM studInfo "
           "WHERE face_features IS NOT NULL");

    while (q.next()) {
        FaceInfo info;
        info.studID = q.value(0).toLongLong();
        info.name   = q.value(1).toString();
        info.faceImage = q.value(3).toString();

        QByteArray blob = q.value(2).toByteArray();
        if (blob.size() >= 1024 * (int)sizeof(float)) {
            memcpy(info.features, blob.constData(), 1024 * sizeof(float));
            m_cache.append(info);
        }
    }

    qDebug() << "Loaded" << m_cache.size() << "face features from database";
}

void FaceRecognitionEngine::reloadCache(QSqlDatabase &db)
{
    loadCache(db);
}

// ========== Camera ==========

bool FaceRecognitionEngine::openCamera(int id)
{
    if (m_capture && m_capture->isOpened()) {
        closeCamera();
    }

    m_capture = new cv::VideoCapture;
    m_capture->open(id);

    if (!m_capture->isOpened()) {
        delete m_capture;
        m_capture = nullptr;
        qDebug() << "Failed to open camera" << id;
        return false;
    }

    // Set resolution
    m_capture->set(cv::CAP_PROP_FRAME_WIDTH, m_videoWidth);
    m_capture->set(cv::CAP_PROP_FRAME_HEIGHT, m_videoHeight);

    // Start timer (~15 fps, ~66ms interval)
    if (!m_timer) {
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &FaceRecognitionEngine::onTimerTick);
    }
    m_timer->start(66);

    qDebug() << "Camera" << id << "opened";
    return true;
}

void FaceRecognitionEngine::closeCamera()
{
    if (m_timer) {
        m_timer->stop();
    }
    if (m_capture) {
        m_capture->release();
        delete m_capture;
        m_capture = nullptr;
    }
    qDebug() << "Camera closed";
}

void FaceRecognitionEngine::onTimerTick()
{
    if (!m_capture || !m_capture->isOpened())
        return;

    cv::Mat frame;
    if (!m_capture->read(frame) || frame.empty())
        return;

    // Flip horizontally for mirror effect
    cv::flip(frame, frame, 1);

    m_currentFrame = cvMatToQImage(frame);
    emit sigFrameReady(m_currentFrame);
}

QImage FaceRecognitionEngine::cvMatToQImage(const cv::Mat &mat)
{
    if (mat.empty()) return QImage();

    cv::Mat rgb;
    if (mat.channels() == 3) {
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
    } else {
        rgb = mat;
    }

    return QImage(rgb.data, rgb.cols, rgb.rows,
                  static_cast<int>(rgb.step),
                  QImage::Format_RGB888).copy();
}

// ========== Face Detection + Feature Extraction ==========

bool FaceRecognitionEngine::detectAndExtract(float *outFeatures,
                                              QImage *outCrop,
                                              QString *errorDetail)
{
    if (!m_fd || !m_pd || !m_fr || !m_qa) {
        if (errorDetail) *errorDetail = QStringLiteral("引擎未初始化");
        return false;
    }
    if (m_currentFrame.isNull()) {
        if (errorDetail) *errorDetail = QStringLiteral("无摄像头画面");
        return false;
    }

    // Convert QImage → cv::Mat → SeetaImageData
    QImage rgb = m_currentFrame.convertToFormat(QImage::Format_RGB888);
    cv::Mat mat(rgb.height(), rgb.width(), CV_8UC3,
                const_cast<uchar*>(rgb.bits()),
                static_cast<size_t>(rgb.bytesPerLine()));
    cv::Mat matBGR;
    cv::cvtColor(mat, matBGR, cv::COLOR_RGB2BGR);

    SeetaImageData img;
    img.width = matBGR.cols;
    img.height = matBGR.rows;
    img.channels = matBGR.channels();
    img.data = matBGR.data;

    // 1. Detect faces
    auto face_array = m_fd->detect(img);
    if (face_array.size <= 0) {
        if (errorDetail) *errorDetail = QStringLiteral("未检测到人脸");
        return false;
    }

    // Use the largest face
    const SeetaFaceInfo *bestFace = &face_array.data[0];
    for (int i = 1; i < face_array.size; i++) {
        int bestArea = bestFace->pos.width * bestFace->pos.height;
        int curArea = face_array.data[i].pos.width * face_array.data[i].pos.height;
        if (curArea > bestArea) bestFace = &face_array.data[i];
    }

    SeetaRect face = bestFace->pos;
    SeetaPointF points[5];
    m_pd->mark(img, face, points);

    // 2. Quality check
    m_qa->feed(img, face, points, 5);
    auto rBright = m_qa->query(seeta::BRIGHTNESS);
    auto rResol  = m_qa->query(seeta::RESOLUTION);
    auto rClar   = m_qa->query(seeta::CLARITY);
    auto rInteg  = m_qa->query(seeta::INTEGRITY);
    auto rPose   = m_qa->query(seeta::POSE_EX);

    if (rBright.level == 0 || rResol.level == 0 || rClar.level == 0 ||
        rInteg.level == 0 || rPose.level == 0)
    {
        if (errorDetail) {
            QStringList fails;
            if (rBright.level == 0) fails << QStringLiteral("亮度过低");
            if (rResol.level  == 0) fails << QStringLiteral("分辨率不足");
            if (rClar.level   == 0) fails << QStringLiteral("清晰度不足");
            if (rInteg.level  == 0) fails << QStringLiteral("人脸不完整");
            if (rPose.level   == 0) fails << QStringLiteral("姿态角度过大");
            *errorDetail = fails.join(", ");
        }
        return false;
    }

    // 3. Extract features
    seeta::ImageData cropface = m_fr->CropFaceV2(img, points);
    m_fr->ExtractCroppedFace(cropface, outFeatures);

    // 4. Generate crop image if requested
    if (outCrop) {
        cv::Mat cropMat(cropface.height, cropface.width,
                        CV_8UC(cropface.channels), cropface.data);
        cv::cvtColor(cropMat, cropMat, cv::COLOR_BGR2RGB);
        *outCrop = QImage(cropMat.data, cropMat.cols, cropMat.rows,
                          static_cast<int>(cropMat.step),
                          QImage::Format_RGB888).copy();
    }

    return true;
}

// ========== Recognition ==========

FaceRecognitionEngine::RecognizeResult
FaceRecognitionEngine::recognize(const float *features)
{
    RecognizeResult result;

    float bestScore = 0.0f;
    const FaceInfo *bestMatch = nullptr;

    for (const auto &fi : m_cache) {
        float score = m_fr->CalculateSimilarity(features, fi.features);
        if (score > bestScore) {
            bestScore = score;
            bestMatch = &fi;
        }
    }

    qDebug() << "Recognize: bestScore=" << bestScore
             << ", threshold=" << m_threshold
             << ", match=" << (bestMatch ? bestMatch->name : "NULL");

    if (bestMatch && bestScore >= m_threshold) {
        result.studID = bestMatch->studID;
        result.name   = bestMatch->name;
        result.score  = bestScore;
        result.snapshot = m_currentFrame;
    }

    return result;
}

// ========== Registration (checkImage) ==========

int FaceRecognitionEngine::checkImage(const QString &imageFile,
                                       const QString &cropFile,
                                       float *outFeatures,
                                       SeetaRect &outRect,
                                       QString *error)
{
    if (!m_fd || !m_pd || !m_fr || !m_qa) {
        if (error) *error = QStringLiteral("引擎未初始化");
        return -3;
    }

    cv::Mat mat = imreadUnicode(imageFile);
    if (mat.empty()) {
        if (error) *error = QStringLiteral("无法读取图片文件");
        return -1;
    }

    SeetaImageData img;
    img.width = mat.cols;
    img.height = mat.rows;
    img.channels = mat.channels();
    img.data = mat.data;

    // Detect
    auto face_array = m_fd->detect(img);
    if (face_array.size <= 0) {
        if (error) *error = QStringLiteral("未检测到人脸");
        return -2;
    } else if (face_array.size > 1) {
        if (error) *error = QStringLiteral("检测到多张人脸，请确保只有一人");
        return 1;
    }

    SeetaRect &face = face_array.data[0].pos;
    SeetaPointF points[5];
    m_pd->mark(img, face, points);

    // Quality check
    m_qa->feed(img, face, points, 5);
    auto r1 = m_qa->query(seeta::BRIGHTNESS);
    auto r2 = m_qa->query(seeta::RESOLUTION);
    auto r3 = m_qa->query(seeta::CLARITY);
    auto r4 = m_qa->query(seeta::INTEGRITY);
    auto rp = m_qa->query(seeta::POSE_EX);

    if (r1.level == 0 || r2.level == 0 || r3.level == 0 ||
        r4.level == 0 || rp.level == 0)
    {
        if (error) {
            QStringList fails;
            fails << QString("置信度=%1").arg(rp.score, 0, 'f', 1);
            if (r1.level == 0) fails << QString("亮度(%1)").arg(r1.score, 0, 'f', 1);
            if (r2.level == 0) fails << QString("分辨率(%1)").arg(r2.score, 0, 'f', 1);
            if (r3.level == 0) fails << QString("清晰度(%1)").arg(r3.score, 0, 'f', 1);
            if (r4.level == 0) fails << QString("完整性(%1)").arg(r4.score, 0, 'f', 1);
            if (rp.level == 0) fails << QString("姿态(%1)").arg(rp.score, 0, 'f', 1);
            *error = fails.join(", ");
        }
        return 2;
    }

    // Crop and extract
    seeta::ImageData cropface = m_fr->CropFaceV2(img, points);
    cv::Mat cropMat(cropface.height, cropface.width,
                    CV_8UC(cropface.channels), cropface.data);
    m_fr->ExtractCroppedFace(cropface, outFeatures);

    // Save crop to file
    cv::imwrite(cropFile.toLocal8Bit().constData(), cropMat);

    // Compute output rect
    outRect.x = face.x - face.width / 2;
    if (outRect.x < 0) outRect.x = 0;
    outRect.y = face.y - face.height / 2;
    if (outRect.y < 0) outRect.y = 0;
    int x2 = face.x + face.width * 3 / 2;
    if (x2 >= img.width) x2 = img.width - 1;
    int y2 = face.y + face.height * 3 / 2;
    if (y2 >= img.height) y2 = img.height - 1;
    outRect.width = x2 - outRect.x;
    outRect.height = y2 - outRect.y;

    return 0;
}
