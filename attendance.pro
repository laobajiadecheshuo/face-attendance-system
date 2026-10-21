#-------------------------------------------------
# Student Attendance Management System
# Combines SeetaFace6 face recognition + Qt SQLite CRUD
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = attendance
TEMPLATE = app

CONFIG += c++11

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    facerecognizer.cpp \
    camerawidget.cpp \
    registerdialog.cpp \
    attendancedialog.cpp

HEADERS += \
    mainwindow.h \
    facerecognizer.h \
    camerawidget.h \
    registerdialog.h \
    attendancedialog.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    res.qrc

# SDK paths: set SEETA_SDK_ROOT to override, otherwise use the default location
SEETA_SDK = $$(SEETA_SDK_ROOT)
isEmpty(SEETA_SDK) {
    SEETA_SDK = "D:/claude code/program_kaggle/program_from_czw/facial_recognition"
}
ROOT = $$SEETA_SDK

INCLUDEPATH += "$${ROOT}/opencv/build/include" \
               "$${ROOT}/sf6.0_windows/sf6.0_windows/include"

CONFIG(debug, debug|release) {
LIBS += "-L$${ROOT}/opencv/build/x64/vc16/lib" -lopencv_world4120d \
        "-L$${ROOT}/sf6.0_windows/sf6.0_windows/lib/x64" \
        -lSeetaFaceDetector600d -lSeetaFaceLandmarker600d \
        -lSeetaFaceAntiSpoofingX600d -lSeetaFaceTracking600d \
        -lSeetaFaceRecognizer610d -lSeetaQualityAssessor300d \
        -lSeetaPoseEstimation600d
} else {
LIBS += "-L$${ROOT}/opencv/build/x64/vc16/lib" -lopencv_world4120 \
        "-L$${ROOT}/sf6.0_windows/sf6.0_windows/lib/x64" \
        -lSeetaFaceDetector600 -lSeetaFaceLandmarker600 \
        -lSeetaFaceAntiSpoofingX600 -lSeetaFaceTracking600 \
        -lSeetaFaceRecognizer610 -lSeetaQualityAssessor300 \
        -lSeetaPoseEstimation600
}
