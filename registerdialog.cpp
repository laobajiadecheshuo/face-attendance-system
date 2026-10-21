#include "registerdialog.h"
#include "facerecognizer.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QDebug>

RegisterDialog::RegisterDialog(FaceRecognitionEngine *engine,
                                 QSqlDatabase &db,
                                 QWidget *parent)
    : QDialog(parent),
      m_engine(engine),
      m_db(db),
      m_studentCombo(new QComboBox(this)),
      m_nameEdit(new QLineEdit(this)),
      m_previewLabel(new QLabel(this)),
      m_captureBtn(new QPushButton(QStringLiteral("📷 采集人脸"), this)),
      m_registerBtn(new QPushButton(QStringLiteral("✅ 确认注册"), this)),
      m_capturedLabel(new QLabel(this)),
      m_statusLabel(new QLabel(this)),
      m_buttonBox(new QDialogButtonBox(QDialogButtonBox::Cancel, this))
{
    setWindowTitle(QStringLiteral("人脸注册"));
    setMinimumSize(700, 500);
    setStyleSheet(R"(
        QDialog { background-color: #1e1e1e; }
        QLabel { color: #9d9d9d; font-size: 13px; }
        QGroupBox {
            color: #e0e0e0; font-weight: bold; font-size: 13px;
            border: 1px solid #3e3e3e; border-radius: 6px;
            margin-top: 10px; padding-top: 18px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            padding: 0 8px;
        }
        QLineEdit {
            background-color: #2d2d2d; color: #d4d4d4;
            border: 1px solid #3e3e3e; border-radius: 4px;
            padding: 6px 10px; font-size: 13px;
        }
        QLineEdit:focus { border-color: #007acc; }
        QLineEdit:hover { border-color: #555555; }
        QComboBox {
            background-color: #2d2d2d; color: #d4d4d4;
            border: 1px solid #3e3e3e; border-radius: 4px;
            padding: 6px 10px; font-size: 13px; min-width: 140px;
        }
        QComboBox:hover { border-color: #555555; }
        QComboBox:focus { border-color: #007acc; }
        QComboBox::drop-down { border: none; background: transparent; }
        QComboBox QAbstractItemView {
            background-color: #252526; color: #d4d4d4;
            selection-background-color: #094771; border: 1px solid #3e3e3e;
        }
        QPushButton {
            background-color: #0e639c; color: #ffffff;
            font-weight: bold; font-size: 13px;
            border: none; border-radius: 4px;
            padding: 8px 20px;
        }
        QPushButton:hover { background-color: #1177bb; }
        QPushButton:pressed { background-color: #0d5685; }
        QPushButton:disabled { background-color: #3d3d3d; color: #6e6e6e; }
        QPushButton:checked { background-color: #389543; color: #ffffff; }
    )");

    // --- Student info section ---
    m_nameEdit->setReadOnly(true);
    m_nameEdit->setPlaceholderText(QStringLiteral("选择学号后自动显示"));

    QFormLayout *form = new QFormLayout;
    form->addRow(QStringLiteral("学号:"), m_studentCombo);
    form->addRow(QStringLiteral("姓名:"), m_nameEdit);

    QGroupBox *infoGroup = new QGroupBox(QStringLiteral("学生信息"), this);
    infoGroup->setLayout(form);

    // --- Camera preview ---
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumSize(320, 240);
    m_previewLabel->setStyleSheet(
        "QLabel { background-color: #1a1a1a; border: 1px solid #3e3e3e; "
        "border-radius: 6px; color: #555; }");
    m_previewLabel->setText(QStringLiteral("摄像头未开启"));

    QGroupBox *camGroup = new QGroupBox(QStringLiteral("摄像头预览"), this);
    QVBoxLayout *camLayout = new QVBoxLayout;
    camLayout->addWidget(m_previewLabel);
    camLayout->addWidget(m_captureBtn, 0, Qt::AlignCenter);
    camGroup->setLayout(camLayout);

    // --- Captured face preview ---
    m_capturedLabel->setAlignment(Qt::AlignCenter);
    m_capturedLabel->setFixedSize(180, 200);
    m_capturedLabel->setStyleSheet(
        "QLabel { background-color: #1a1a1a; border: 1px solid #3e3e3e; "
        "border-radius: 6px; color: #555; }");
    m_capturedLabel->setText(QStringLiteral("待采集"));

    QGroupBox *capGroup = new QGroupBox(QStringLiteral("采集结果"), this);
    QVBoxLayout *capLayout = new QVBoxLayout;
    capLayout->addWidget(m_capturedLabel, 0, Qt::AlignCenter);
    capLayout->addWidget(m_statusLabel, 0, Qt::AlignCenter);
    capLayout->addWidget(m_registerBtn, 0, Qt::AlignCenter);
    capGroup->setLayout(capLayout);

    // --- Bottom ---
    QHBoxLayout *bottomLayout = new QHBoxLayout;
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_buttonBox);

    // --- Main layout (left: camera, right: capture result) ---
    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addWidget(camGroup, 1);
    topLayout->addWidget(capGroup, 0);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(infoGroup);
    mainLayout->addLayout(topLayout, 1);
    mainLayout->addLayout(bottomLayout);

    // --- State ---
    m_registerBtn->setEnabled(false);
    m_captureBtn->setEnabled(false);

    // --- Connections ---
    connect(m_studentCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RegisterDialog::onStudentSelected);
    connect(m_captureBtn, &QPushButton::clicked,
            this, &RegisterDialog::onCaptureClicked);
    connect(m_registerBtn, &QPushButton::clicked,
            this, &RegisterDialog::onRegisterClicked);
    connect(m_buttonBox, &QDialogButtonBox::rejected,
            this, &RegisterDialog::reject);

    // Listen to engine frames for preview
    connect(m_engine, &FaceRecognitionEngine::sigFrameReady,
            this, &RegisterDialog::onEngineFrame);

    // Load students
    loadStudents();

    // Determine faces directory
    m_facesDir = QCoreApplication::applicationDirPath() + "/attendance_faces/";
    QDir().mkpath(m_facesDir);

    // Open camera if not already open
    if (!m_engine->cameraIsOpen()) {
        m_engine->openCamera(0);
    }
    m_captureBtn->setEnabled(true);

    qDebug() << "RegisterDialog opened";
}

void RegisterDialog::loadStudents()
{
    m_students.clear();
    m_studentCombo->clear();
    m_studentCombo->addItem(QStringLiteral("-- 选择学生 --"), QVariant());

    QSqlQuery q(m_db);
    q.exec("SELECT studID, name FROM studInfo ORDER BY studID");
    while (q.next()) {
        int64_t id = q.value(0).toLongLong();
        QString name = q.value(1).toString();
        m_students.insert(id, name);
        m_studentCombo->addItem(
            QString("%1 - %2").arg(id).arg(name),
            QVariant::fromValue<qint64>(id));
    }

    qDebug() << "Loaded" << m_students.size() << "students for registration";
}

void RegisterDialog::onStudentSelected(int index)
{
    Q_UNUSED(index);
    QVariant data = m_studentCombo->currentData();
    if (data.isValid()) {
        int64_t studID = data.toLongLong();
        m_nameEdit->setText(m_students.value(studID));
    } else {
        m_nameEdit->clear();
    }
}

void RegisterDialog::onEngineFrame(const QImage &frame)
{
    if (frame.isNull()) return;
    QPixmap pix = QPixmap::fromImage(
        frame.scaled(m_previewLabel->size(), Qt::KeepAspectRatio,
                     Qt::SmoothTransformation));
    m_previewLabel->setPixmap(pix);
}

void RegisterDialog::onCaptureClicked()
{
    m_statusLabel->setText(QStringLiteral("正在采集..."));
    m_statusLabel->setStyleSheet("color: #cca700; font-size: 13px;");
    QCoreApplication::processEvents();

    // Save current frame to temp file
    QString tempFile = m_facesDir + "temp_capture.jpg";
    QImage frame = m_engine->currentFrame();
    if (frame.isNull()) {
        m_statusLabel->setText(QStringLiteral("❌ 无摄像头画面"));
        m_statusLabel->setStyleSheet("color: #f44747; font-size: 13px;");
        return;
    }
    frame.save(tempFile, "JPEG", 90);

    // Run checkImage
    QString cropFile = m_facesDir + "temp_crop.jpg";
    QString error;
    int ret = m_engine->checkImage(tempFile, cropFile,
                                    m_capturedFeatures, m_capturedRect, &error);
    if (ret == 0) {
        // Success
        m_capturedImagePath = tempFile;
        m_hasCapturedFace = true;
        m_registerBtn->setEnabled(true);

        // Show cropped face
        QPixmap pix(cropFile);
        if (!pix.isNull()) {
            m_capturedCrop = QImage(cropFile);
            m_capturedLabel->setPixmap(
                pix.scaled(m_capturedLabel->size(), Qt::KeepAspectRatio,
                           Qt::SmoothTransformation));
        }
        m_statusLabel->setText(QStringLiteral("✅ 人脸采集成功，请确认注册"));
        m_statusLabel->setStyleSheet("color: #4ec9b0; font-size: 13px;");
    } else {
        // Failed
        m_hasCapturedFace = false;
        m_registerBtn->setEnabled(false);
        m_capturedLabel->setText(QStringLiteral("采集失败"));
        QString reason;
        if (ret == -1) reason = QStringLiteral("无法读取图片");
        else if (ret == -2) reason = QStringLiteral("未检测到人脸");
        else if (ret == -3) reason = QStringLiteral("人脸引擎未初始化，请检查模型路径");
        else if (ret == 1) reason = QStringLiteral("检测到多张人脸");
        else if (ret == 2) reason = error;
        m_statusLabel->setText(QStringLiteral("❌ ") + reason);
        m_statusLabel->setStyleSheet("color: #f44747; font-size: 13px;");
    }
}

void RegisterDialog::onRegisterClicked()
{
    if (!m_hasCapturedFace) return;

    QVariant data = m_studentCombo->currentData();
    if (!data.isValid()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("请先选择学生"));
        return;
    }

    int64_t studID = data.toLongLong();
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("请选择有姓名的学生"));
        return;
    }

    // Serialize features → BLOB
    QByteArray blob;
    blob.resize(1024 * sizeof(float));
    memcpy(blob.data(), m_capturedFeatures, 1024 * sizeof(float));

    // Save face image to faces directory
    QString faceFilename = QString("face_%1_%2.jpg")
        .arg(studID)
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    QString facePath = m_facesDir + faceFilename;
    m_engine->currentFrame().save(facePath, "JPEG", 90);

    // Update database
    QSqlQuery q(m_db);
    q.prepare("UPDATE studInfo SET face_features=?, face_image=? WHERE studID=?");
    q.addBindValue(QVariant(blob));
    q.addBindValue(facePath);
    q.addBindValue(studID);

    if (!q.exec()) {
        QMessageBox::critical(this, QStringLiteral("错误"),
            QStringLiteral("数据库更新失败:\n%1").arg(q.lastError().text()));
        return;
    }

    // Reload engine cache
    m_engine->reloadCache(m_db);

    emit registrationComplete(studID);

    QMessageBox::information(this, QStringLiteral("成功"),
        QStringLiteral("学生 %1 (%2) 人脸注册成功！").arg(name).arg(studID));

    // Reset for next registration
    m_hasCapturedFace = false;
    m_registerBtn->setEnabled(false);
    m_capturedLabel->setText(QStringLiteral("待采集"));
    m_statusLabel->setText(QStringLiteral("已注册，可继续注册下一位"));
    m_statusLabel->setStyleSheet("color: #4ec9b0; font-size: 13px;");
}

void RegisterDialog::validateAndAccept()
{
    accept();
}

void RegisterDialog::setControlsEnabled(bool enabled)
{
    m_captureBtn->setEnabled(enabled);
    m_registerBtn->setEnabled(enabled && m_hasCapturedFace);
}
