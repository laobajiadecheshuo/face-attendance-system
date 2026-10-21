#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSql>
#include <QMap>
#include <QLabel>

#include "facerecognizer.h"

class QLineEdit;
class QComboBox;
class QDateEdit;
class QPushButton;
class QAction;
class CameraWidget;

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool openDatabase(const QString &aFile);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // --- Toolbar ---
    void on_actOpenDB_triggered();
    void on_actRefresh_triggered();

    // --- Tab 0: Check-in ---
    void on_checkinBtn_clicked();
    void on_autoCheckinBtn_toggled(bool checked);
    void onEngineFrame(const QImage &frame);

    // --- Tab 1: Register ---
    void on_openRegisterBtn_clicked();

    // --- Tab 2: Records ---
    void onRecordsTabActivated();

    // Records CRUD
    void on_actRecAppend_triggered();
    void on_actRecInsert_triggered();
    void on_actRecDelete_triggered();
    void on_actRecEdit_triggered();
    void on_actSubmit_triggered();
    void on_actRevert_triggered();
    void on_actFields_triggered();
    void on_recordsSearchClicked();
    void on_recordsTableView_doubleClicked(const QModelIndex &index);

    // --- Tab 3: Statistics ---
    void on_tabWidget_currentChanged(int index);
    void refreshStatistics();
    void refreshTodayList();
    void refreshRegisteredList();
    void onResetAll();

private:
    void initDatabase();
    void initEngine();
    void initCheckinTab();
    void initRecordsTab();
    void initStatisticsTab();
    void clearModels();
    void loadStudentCache();

    // Check-in logic
    void initTodayAttendance();
    bool hasCheckedInToday(int64_t studID);
    bool recordAttendance(int64_t studID, float confidence,
                          const QString &faceImage);

    // Dark theme
    void applyDarkTheme();

    Ui::MainWindow *ui;

    // Database
    QSqlDatabase m_db;
    QString m_connName = "attendanceConn";

    // Face engine
    FaceRecognitionEngine *m_engine;

    // Camera preview widget (embedded in Tab 0)
    CameraWidget *m_cameraWidget;

    // Records tab (Tab 2)
    QSqlRelationalTableModel *m_attModel = nullptr;
    QItemSelectionModel       *m_selModel = nullptr;

    // Records toolbar actions
    QAction *m_actRecAppend = nullptr;
    QAction *m_actRecInsert = nullptr;
    QAction *m_actRecDelete = nullptr;
    QAction *m_actRecEdit   = nullptr;
    QAction *m_actSubmit    = nullptr;
    QAction *m_actRevert    = nullptr;
    QAction *m_actFields    = nullptr;

    // Records search widgets (added to records tab)
    QLineEdit   *m_searchStudID = nullptr;
    QComboBox   *m_searchStatus = nullptr;
    QDateEdit   *m_searchDateFrom = nullptr;
    QDateEdit   *m_searchDateTo = nullptr;
    QPushButton *m_searchBtn = nullptr;

    // Student cache: studID → name
    QMap<int64_t, QString> m_studentNames;

    // Check-in state
    bool m_autoCheckin = false;
    bool m_checkinBusy = false;
    QTimer  *m_autoTimer = nullptr;
};

#endif // MAINWINDOW_H
