#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QtSql>
#include <QMap>

#include "seeta/Common/Struct.h"

class FaceRecognitionEngine;

class RegisterDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RegisterDialog(FaceRecognitionEngine *engine,
                             QSqlDatabase &db,
                             QWidget *parent = nullptr);

signals:
    void registrationComplete(int64_t studID);

private slots:
    void onStudentSelected(int index);
    void onCaptureClicked();
    void onRegisterClicked();
    void validateAndAccept();
    void onEngineFrame(const QImage &frame);

private:
    void loadStudents();
    void setControlsEnabled(bool enabled);
    void updatePreviewFace();

    FaceRecognitionEngine *m_engine;
    QSqlDatabase m_db;

    // Student selection
    QComboBox *m_studentCombo;
    QLineEdit *m_nameEdit;

    // Camera preview
    QLabel *m_previewLabel;

    // Capture
    QPushButton *m_captureBtn;
    QPushButton *m_registerBtn;
    QLabel *m_capturedLabel;
    QLabel *m_statusLabel;

    QDialogButtonBox *m_buttonBox;

    // Captured data
    float      m_capturedFeatures[1024];
    SeetaRect  m_capturedRect;
    QString    m_capturedImagePath;
    QImage     m_capturedCrop;
    bool       m_hasCapturedFace = false;

    // Student data
    QMap<int64_t, QString> m_students;   // studID → name
    QString m_facesDir;
};

#endif // REGISTERDIALOG_H
