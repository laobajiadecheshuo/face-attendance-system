#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "camerawidget.h"
#include "registerdialog.h"
#include "attendancedialog.h"

#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QPushButton>
#include <QToolBar>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlField>
#include <QDebug>
#include <QStatusBar>
#include <QApplication>
#include <QHeaderView>
#include <QDir>
#include <QDateTime>
#include <QTimer>
#include <QThread>
#include <QCloseEvent>
#include <QMetaType>

// =====================================================================
// Constructor
// =====================================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Dark theme
    applyDarkTheme();

    // Toolbar + menubar: hidden for now
    ui->mainToolBar->hide();
    ui->menuBar->hide();

    // Initialize database
    initDatabase();

    // Initialize face engine
    initEngine();

    // Initialize tabs
    initCheckinTab();
    initRecordsTab();
    initStatisticsTab();

    // Load student names cache
    loadStudentCache();

    // Connect tab change signal
    connect(ui->tabWidget, &QTabWidget::currentChanged,
            this, &MainWindow::on_tabWidget_currentChanged);

    // Initial refresh
    refreshTodayList();
    refreshRegisteredList();
    refreshStatistics();

    qDebug() << "MainWindow initialized";
}

MainWindow::~MainWindow()
{
    clearModels();
    if (m_engine) {
        m_engine->closeCamera();
    }
    delete ui;
}

// =====================================================================
// Dark Theme (copied from samp9_4Grade)
// =====================================================================

void MainWindow::applyDarkTheme()
{
    qApp->setStyleSheet(R"(
        /* === Global === */
        QMainWindow { background-color: #1e1e1e; }
        QWidget { font-family: "Segoe UI", "Microsoft YaHei", sans-serif; }
        QLabel { color: #9d9d9d; font-size: 13px; }

        /* === Status Bar === */
        QStatusBar {
            background-color: #007acc;
            color: #ffffff; font-weight: bold; font-size: 12px;
            border: none;
        }
        QStatusBar::item { border: none; }

        /* === Table View === */
        QTableView {
            background-color: #252526; color: #d4d4d4;
            gridline-color: #3e3e3e; border: 1px solid #3e3e3e;
            border-radius: 4px; font-size: 13px;
            selection-background-color: #094771; selection-color: #ffffff;
        }
        QTableView::item { padding: 6px 10px; border-bottom: 1px solid #333333; }
        QTableView::item:selected { background-color: #094771; color: #ffffff; }
        QTableCornerButton::section { background-color: #2d2d2d; border: none; }

        /* === Table Header === */
        QHeaderView::section {
            background-color: #2d2d2d;
            color: #e0e0e0; font-weight: bold; font-size: 12px;
            padding: 8px 10px; border: none;
            border-right: 1px solid #3e3e3e; border-bottom: 2px solid #007acc;
        }
        QHeaderView::section:hover { background-color: #383838; }

        /* === Tab Widget === */
        QTabWidget::pane { border: 1px solid #3e3e3e; background-color: #1e1e1e; }
        QTabBar::tab {
            background-color: #2d2d2d; color: #9d9d9d;
            padding: 10px 24px; font-size: 13px; font-weight: bold;
            border: 1px solid #3e3e3e; border-bottom: none;
            border-top-left-radius: 4px; border-top-right-radius: 4px;
            margin-right: 2px;
        }
        QTabBar::tab:selected {
            background-color: #1e1e1e; color: #ffffff;
            border-bottom: 2px solid #007acc;
            border-top: 2px solid #007acc;
        }
        QTabBar::tab:hover:!selected { color: #d4d4d4; background-color: #383838; }

        /* === Toolbar (hidden) === */
        QToolBar {
            background: #252526; border-bottom: 1px solid #3e3e3e;
            spacing: 4px; padding: 3px 6px;
        }
        QToolBar::separator { background-color: #3e3e3e; width: 1px; margin: 4px 6px; }
        QToolButton {
            color: #d4d4d4; font-weight: bold; font-size: 13px;
            padding: 5px 14px; border-radius: 4px; background: transparent;
        }
        QToolButton:hover { background-color: #333333; color: #ffffff; }
        QToolButton:pressed { background-color: #007acc; }

        /* === Input Fields === */
        QLineEdit {
            background-color: #2d2d2d; color: #d4d4d4;
            border: 1px solid #3e3e3e; border-radius: 4px;
            padding: 6px 10px; font-size: 13px;
        }
        QLineEdit:focus { border-color: #007acc; }
        QLineEdit:hover { border-color: #555555; }

        QComboBox, QDateEdit, QTimeEdit {
            background-color: #2d2d2d; color: #d4d4d4;
            border: 1px solid #3e3e3e; border-radius: 4px;
            padding: 6px 10px; font-size: 13px; min-width: 100px;
        }
        QComboBox:hover, QDateEdit:hover, QTimeEdit:hover { border-color: #555555; }
        QComboBox:focus, QDateEdit:focus, QTimeEdit:focus { border-color: #007acc; }
        QComboBox::drop-down {
            border: none; background-color: transparent;
        }
        QComboBox QAbstractItemView {
            background-color: #252526; color: #d4d4d4;
            selection-background-color: #094771; border: 1px solid #3e3e3e;
            border-radius: 2px; padding: 4px 0;
        }

        /* === Double Spin Box === */
        QDoubleSpinBox {
            background-color: #2d2d2d; color: #d4d4d4;
            border: 1px solid #3e3e3e; border-radius: 4px;
            padding: 6px 10px; font-size: 13px;
        }
        QDoubleSpinBox:focus { border-color: #007acc; }
        QDoubleSpinBox:hover { border-color: #555555; }

        /* === Buttons === */
        QPushButton {
            background-color: #0e639c; color: #ffffff;
            font-weight: bold; font-size: 13px;
            border: none; border-radius: 4px;
            padding: 8px 20px;
        }
        QPushButton:hover { background-color: #1177bb; }
        QPushButton:pressed { background-color: #0d5685; }
        QPushButton:disabled {
            background-color: #3d3d3d; color: #6e6e6e;
        }
        QPushButton:checked { background-color: #389543; color: #ffffff; }
    )");
}

// =====================================================================
// Database Initialization
// =====================================================================

bool MainWindow::openDatabase(const QString &aFile)
{
    if (QSqlDatabase::contains(m_connName)) {
        QSqlDatabase old = QSqlDatabase::database(m_connName);
        if (old.isOpen()) old.close();
        QSqlDatabase::removeDatabase(m_connName);
    }

    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connName);
    m_db.setDatabaseName(aFile);
    if (m_db.open()) {
        initDatabase(); // create tables if needed
        loadStudentCache();
        m_engine->reloadCache(m_db);
        refreshTodayList();
        refreshRegisteredList();
        refreshStatistics();
        ui->statusBar->showMessage(
            tr("数据库已打开: %1").arg(aFile), 3000);
        return true;
    } else {
        QMessageBox::warning(this, tr("错误"),
            tr("无法打开数据库: %1").arg(aFile));
        return false;
    }
}

void MainWindow::initDatabase()
{
    // Use demodb.db3 from project root (exe is in debug/, go up 2 levels)
    QString exePath = QCoreApplication::applicationDirPath();
    QString dbPath = exePath + "/../../demodb.db3";

    if (QSqlDatabase::contains(m_connName)) {
        m_db = QSqlDatabase::database(m_connName);
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE", m_connName);
        m_db.setDatabaseName(dbPath);
    }

    if (!m_db.isOpen() && !m_db.open()) {
        qDebug() << "Warning: Could not open DB at" << dbPath
                 << "- will try file dialog later";
        ui->statusBar->showMessage(
            QStringLiteral("数据库未连接，请使用 [打开数据库] 选择 demodb.db3"));
        return;
    }

    QSqlQuery q(m_db);

    // --- Add face columns to studInfo if missing ---
    q.exec("PRAGMA table_info(studInfo)");
    bool hasFeatures = false, hasFaceImage = false;
    while (q.next()) {
        QString col = q.value("name").toString();
        if (col == "face_features") hasFeatures = true;
        if (col == "face_image") hasFaceImage = true;
    }
    if (!hasFeatures) {
        q.exec("ALTER TABLE studInfo ADD COLUMN face_features BLOB");
        qDebug() << "Added face_features column to studInfo";
    }
    if (!hasFaceImage) {
        q.exec("ALTER TABLE studInfo ADD COLUMN face_image TEXT");
        qDebug() << "Added face_image column to studInfo";
    }

    // --- Create attendance table ---
    q.exec("CREATE TABLE IF NOT EXISTS attendance ("
           "attID INTEGER PRIMARY KEY AUTOINCREMENT, "
           "studID INT64 NOT NULL, "
           "attDate DATE NOT NULL, "
           "attTime TIME NOT NULL, "
           "status VARCHAR(10) DEFAULT 'present', "
           "confidence REAL, "
           "face_image TEXT, "
           "FOREIGN KEY (studID) REFERENCES studInfo(studID))");

    // --- Create settings table ---
    q.exec("CREATE TABLE IF NOT EXISTS att_settings ("
           "key VARCHAR(50) PRIMARY KEY, value TEXT)");

    // Insert default settings if not exist
    q.exec("INSERT OR IGNORE INTO att_settings VALUES ('Fr_Threshold', '0.65')");
    q.exec("INSERT OR IGNORE INTO att_settings VALUES ('camera_id', '0')");

    qDebug() << "Database initialized:" << dbPath;
    ui->statusBar->showMessage(
        tr("数据库已连接: %1").arg(dbPath), 3000);

    // Initialize today's attendance for all students
    initTodayAttendance();
}

// =====================================================================
// Face Engine Initialization
// =====================================================================

void MainWindow::initEngine()
{
    // Model directory: first models/ next to the exe, then known SDK locations
    QStringList modelDirCandidates = {
        QCoreApplication::applicationDirPath() + "/models",
        "D:/claude code/program_kaggle/program_from_czw/facial_recognition"
        "/sf3.0_models/sf3.0_models",
        "C:/Users/asus/Desktop/claude code/program_from_czw/facial_recognition"
        "/sf3.0_models/sf3.0_models",
    };
    QString modelDir;
    for (const QString &dir : modelDirCandidates) {
        if (QFile::exists(dir + "/face_detector.csta")) {
            modelDir = dir + "/";
            break;
        }
    }

    m_engine = new FaceRecognitionEngine(this);

    if (modelDir.isEmpty()) {
        QMessageBox::critical(this, tr("错误"),
            tr("人脸识别引擎初始化失败！\n未找到模型目录（含 face_detector.csta），"
               "请将模型放到 %1/models 或设置模型路径")
                .arg(QCoreApplication::applicationDirPath()));
        return;
    }

    if (!m_engine->initModels(modelDir)) {
        QMessageBox::critical(this, tr("错误"),
            tr("人脸识别引擎初始化失败！\n模型路径: %1").arg(modelDir));
        return;
    }

    // Load threshold from settings
    QSqlQuery q(m_db);
    if (q.exec("SELECT value FROM att_settings WHERE key='Fr_Threshold'")) {
        if (q.next()) {
            float th = q.value(0).toFloat();
            if (th > 0 && th <= 1.0f) m_engine->setThreshold(th);
        }
    }

    // Load feature cache from database
    if (m_db.isOpen()) {
        m_engine->loadCache(m_db);
    }

    // Connect frame signal to camera widget
    connect(m_engine, &FaceRecognitionEngine::sigFrameReady,
            this, &MainWindow::onEngineFrame);
}

// =====================================================================
// Tab 0: Check-in
// =====================================================================

void MainWindow::initCheckinTab()
{
    // Embed CameraWidget into the camera container
    m_cameraWidget = new CameraWidget(ui->cameraContainer);
    QVBoxLayout *camLayout = new QVBoxLayout(ui->cameraContainer);
    camLayout->setContentsMargins(0, 0, 0, 0);
    camLayout->addWidget(m_cameraWidget);

    // Connect buttons
    connect(ui->checkinBtn, &QPushButton::clicked,
            this, &MainWindow::on_checkinBtn_clicked);
    connect(ui->autoCheckinBtn, &QPushButton::toggled,
            this, &MainWindow::on_autoCheckinBtn_toggled);

    // Auto check-in timer (every 2 seconds)
    m_autoTimer = new QTimer(this);
    m_autoTimer->setInterval(2000);
    connect(m_autoTimer, &QTimer::timeout, this, [this]() {
        if (m_autoCheckin && !m_checkinBusy) {
            on_checkinBtn_clicked();
        }
    });

    // Style the check-in result label
    ui->checkinStatus->setStyleSheet(
        "QLabel { color: #4ec9b0; font-weight: bold; font-size: 15px; "
        "padding: 4px; }");

    // Style today's table
    ui->todayTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->todayTableView->setAlternatingRowColors(true);

    ui->todayLabel->setStyleSheet(
        "QLabel { color: #e0e0e0; font-size: 14px; font-weight: bold; }");
}

void MainWindow::onEngineFrame(const QImage &frame)
{
    if (m_cameraWidget && ui->tabWidget->currentIndex() == 0) {
        m_cameraWidget->setFrame(frame);
    }
}

void MainWindow::on_checkinBtn_clicked()
{
    if (m_checkinBusy) return;

    // Check camera
    if (!m_engine->cameraIsOpen()) {
        bool ok = m_engine->openCamera(0);
        if (!ok) {
            ui->checkinStatus->setText(
                QStringLiteral("❌ 摄像头打开失败"));
            ui->checkinStatus->setStyleSheet(
                "QLabel { color: #f44747; font-weight: bold; font-size: 15px; }");
            return;
        }
        // Wait a moment for camera to warm up
        QThread::msleep(300);
    }

    // Check registered faces
    if (m_engine->cacheSize() == 0) {
        ui->checkinStatus->setText(
            QStringLiteral("⚠️ 没有已注册的人脸，请先注册"));
        ui->checkinStatus->setStyleSheet(
            "QLabel { color: #cca700; font-weight: bold; font-size: 15px; }");
        return;
    }

    m_checkinBusy = true;
    ui->checkinStatus->setText(QStringLiteral("🔍 正在识别，请正视摄像头..."));
    ui->checkinStatus->setStyleSheet(
        "QLabel { color: #cca700; font-weight: bold; font-size: 15px; }");
    QCoreApplication::processEvents();

    // Detect face and extract features
    float features[1024];
    QString error;
    bool ok = m_engine->detectAndExtract(features, nullptr, &error);

    if (!ok) {
        ui->checkinStatus->setText(
            QStringLiteral("❌ 识别失败: %1").arg(error));
        ui->checkinStatus->setStyleSheet(
            "QLabel { color: #f44747; font-weight: bold; font-size: 15px; }");
        m_checkinBusy = false;
        return;
    }

    // Recognize
    auto result = m_engine->recognize(features);

    if (result.studID > 0) {
        // Check for duplicate today
        if (hasCheckedInToday(result.studID)) {
            ui->checkinStatus->setText(
                QStringLiteral("⚠️ %1 今天已打卡，无需重复").arg(result.name));
            ui->checkinStatus->setStyleSheet(
                "QLabel { color: #cca700; font-weight: bold; font-size: 15px; }");
        } else {
            // Save face snapshot
            QString snapDir = QCoreApplication::applicationDirPath()
                              + "/attendance_snapshots/";
            QDir().mkpath(snapDir);
            QString snapPath = snapDir +
                QString("snap_%1_%2.jpg")
                    .arg(result.studID)
                    .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
            m_engine->currentFrame().save(snapPath, "JPEG", 85);

            // Record attendance
            if (recordAttendance(result.studID, result.score, snapPath)) {
                ui->checkinStatus->setText(
                    QStringLiteral("✅ %1 打卡成功! (%2%)")
                        .arg(result.name)
                        .arg(result.score * 100, 0, 'f', 1));
                ui->checkinStatus->setStyleSheet(
                    "QLabel { color: #4ec9b0; font-weight: bold; font-size: 15px; }");
                refreshTodayList();
                refreshStatistics();
                // Also refresh records tab if model is loaded
                if (m_attModel) m_attModel->select();

                // Play success feedback — keep for 3 seconds
                QString name = result.name;
                QTimer::singleShot(3000, this, [this]() {
                    ui->checkinStatus->setText(
                        QStringLiteral("就绪 - 等待打卡"));
                    ui->checkinStatus->setStyleSheet(
                        "QLabel { color: #9d9d9d; font-weight: bold; font-size: 15px; }");
                });
            } else {
                ui->checkinStatus->setText(
                    QStringLiteral("❌ 记录保存失败"));
                ui->checkinStatus->setStyleSheet(
                    "QLabel { color: #f44747; font-weight: bold; font-size: 15px; }");
            }
        }
    } else {
        ui->checkinStatus->setText(
            QStringLiteral("❌ 未识别到已注册人脸 (最高分: %1)")
                .arg(result.score, 0, 'f', 3));
        ui->checkinStatus->setStyleSheet(
            "QLabel { color: #f44747; font-weight: bold; font-size: 15px; }");
    }

    m_checkinBusy = false;
}

void MainWindow::on_autoCheckinBtn_toggled(bool checked)
{
    m_autoCheckin = checked;
    if (checked) {
        m_autoTimer->start();
        ui->checkinStatus->setText(QStringLiteral("🔄 自动识别模式开启..."));
        ui->checkinStatus->setStyleSheet(
            "QLabel { color: #4ec9b0; font-weight: bold; font-size: 15px; }");

        // Open camera if not open
        if (!m_engine->cameraIsOpen()) {
            m_engine->openCamera(0);
        }
    } else {
        m_autoTimer->stop();
        ui->checkinStatus->setText(QStringLiteral("就绪 - 等待打卡"));
        ui->checkinStatus->setStyleSheet(
            "QLabel { color: #9d9d9d; font-weight: bold; font-size: 15px; }");
    }
}

void MainWindow::initTodayAttendance()
{
    if (!m_db.isOpen()) return;

    QSqlQuery q(m_db);

    // Get all students
    q.exec("SELECT studID FROM studInfo ORDER BY studID");
    QList<int64_t> allStudents;
    while (q.next())
        allStudents.append(q.value(0).toLongLong());

    // For each student, ensure they have a record for today
    for (int64_t sid : allStudents) {
        QSqlQuery check(m_db);
        check.prepare("SELECT COUNT(*) FROM attendance "
                      "WHERE studID=? AND attDate=date('now','localtime')");
        check.addBindValue(sid);
        if (check.exec() && check.next() && check.value(0).toInt() > 0)
            continue; // already has today's record

        QSqlQuery ins(m_db);
        ins.prepare("INSERT INTO attendance (studID, attDate, attTime, status, confidence) "
                    "VALUES (?, date('now','localtime'), time('now','localtime'), 'absent', 0)");
        ins.addBindValue(sid);
        if (!ins.exec())
            qDebug() << "initTodayAttendance failed for" << sid << ins.lastError().text();
    }

    qDebug() << "initTodayAttendance done for" << allStudents.size() << "students";
}

bool MainWindow::hasCheckedInToday(int64_t studID)
{
    QSqlQuery q(m_db);
    q.prepare("SELECT COUNT(*) FROM attendance "
              "WHERE studID=? AND attDate=date('now','localtime') "
              "AND status IN ('present','late')");
    q.addBindValue(studID);
    if (q.exec() && q.next())
        return q.value(0).toInt() > 0;
    return false;
}

bool MainWindow::recordAttendance(int64_t studID, float confidence,
                                   const QString &faceImage)
{
    QSqlQuery q(m_db);
    // Check if student already has a record for today (initTodayAttendance)
    q.prepare("SELECT attID FROM attendance "
              "WHERE studID=? AND attDate=date('now','localtime')");
    q.addBindValue(studID);
    if (q.exec() && q.next()) {
        // Update the existing record (from 'absent' → 'present')
        int attID = q.value(0).toInt();
        QSqlQuery up(m_db);
        up.prepare("UPDATE attendance SET attTime=time('now','localtime'), "
                   "status='present', confidence=?, face_image=? WHERE attID=?");
        up.addBindValue(confidence);
        up.addBindValue(faceImage);
        up.addBindValue(attID);
        if (!up.exec()) {
            qDebug() << "Failed to update attendance:" << up.lastError().text();
            return false;
        }
        qDebug() << "Attendance updated:" << studID << confidence;
    } else {
        // Fallback: insert new record
        QSqlQuery ins(m_db);
        ins.prepare("INSERT INTO attendance (studID, attDate, attTime, status, "
                    "confidence, face_image) "
                    "VALUES (?, date('now','localtime'), time('now','localtime'), "
                    "'present', ?, ?)");
        ins.addBindValue(studID);
        ins.addBindValue(confidence);
        ins.addBindValue(faceImage);
        if (!ins.exec()) {
            qDebug() << "Failed to insert attendance:" << ins.lastError().text();
            return false;
        }
        qDebug() << "Attendance inserted:" << studID << confidence;
    }
    return true;
}

void MainWindow::refreshTodayList()
{
    // Use QSqlQueryModel for read-only today view
    QSqlQueryModel *model = new QSqlQueryModel(this);
    model->setQuery(
        "SELECT s.name AS 姓名, a.attTime AS 时间, a.confidence AS '置信度(%)' "
        "FROM attendance a "
        "JOIN studInfo s ON a.studID = s.studID "
        "WHERE a.attDate = date('now','localtime') "
        "ORDER BY a.attTime DESC",
        m_db);

    model->setHeaderData(0, Qt::Horizontal, QStringLiteral("姓名"));
    model->setHeaderData(1, Qt::Horizontal, QStringLiteral("时间"));
    model->setHeaderData(2, Qt::Horizontal, QStringLiteral("置信度"));

    // Convert confidence to percentage display
    // (QSqlQueryModel is read-only, so we format in the query)

    QAbstractItemModel *old = ui->todayTableView->model();
    ui->todayTableView->setModel(model);
    if (old && old != model) delete old;

    ui->todayTableView->horizontalHeader()->setStretchLastSection(true);
    ui->todayTableView->setColumnWidth(0, 100);
    ui->todayTableView->setColumnWidth(1, 120);
}

// =====================================================================
// Tab 1: Face Registration
// =====================================================================

void MainWindow::on_openRegisterBtn_clicked()
{
    RegisterDialog dlg(m_engine, m_db, this);
    connect(&dlg, &RegisterDialog::registrationComplete, this, [this](int64_t) {
        refreshRegisteredList();
        refreshStatistics();
    });
    dlg.exec();

    // Refresh registered list after dialog closes
    refreshRegisteredList();
}

void MainWindow::refreshRegisteredList()
{
    QSqlQueryModel *model = new QSqlQueryModel(this);
    model->setQuery(
        "SELECT studID AS 学号, name AS 姓名, "
        "CASE WHEN face_features IS NOT NULL THEN '✅已注册' ELSE '❌未注册' END AS '人脸状态' "
        "FROM studInfo ORDER BY studID",
        m_db);

    QAbstractItemModel *old = ui->registeredTableView->model();
    ui->registeredTableView->setModel(model);
    if (old && old != model) delete old;

    ui->registeredTableView->horizontalHeader()->setStretchLastSection(true);
    ui->registeredTableView->setColumnWidth(0, 120);
    ui->registeredTableView->setColumnWidth(1, 100);

    int registered = m_engine->cacheSize();
    ui->registeredInfo->setText(
        QStringLiteral("已注册人脸: %1 人").arg(registered));
}

// =====================================================================
// Tab 2: Attendance Records
// =====================================================================

void MainWindow::initRecordsTab()
{
    // Add CRUD toolbar actions to the records tab area
    // We add them to the main toolbar and they only work when Tab 2 is active

    // --- Records CRUD Actions ---
    m_actRecAppend = new QAction(tr("添加"), this);
    m_actRecAppend->setToolTip(tr("添加一条考勤记录"));
    ui->mainToolBar->addAction(m_actRecAppend);

    m_actRecInsert = new QAction(tr("插入"), this);
    m_actRecInsert->setToolTip(tr("在当前行前插入记录"));
    ui->mainToolBar->addAction(m_actRecInsert);

    m_actRecDelete = new QAction(tr("删除"), this);
    m_actRecDelete->setToolTip(tr("删除当前考勤记录"));
    ui->mainToolBar->addAction(m_actRecDelete);

    ui->mainToolBar->addSeparator();

    m_actRecEdit = new QAction(tr("编辑"), this);
    m_actRecEdit->setToolTip(tr("编辑当前考勤记录"));
    ui->mainToolBar->addAction(m_actRecEdit);

    m_actSubmit = new QAction(tr("保存"), this);
    m_actSubmit->setToolTip(tr("保存所有修改到数据库"));
    ui->mainToolBar->addAction(m_actSubmit);

    m_actRevert = new QAction(tr("取消"), this);
    m_actRevert->setToolTip(tr("撤销所有未保存的修改"));
    ui->mainToolBar->addAction(m_actRevert);

    ui->mainToolBar->addSeparator();

    m_actFields = new QAction(tr("字段列表"), this);
    m_actFields->setToolTip(tr("显示所有字段名"));
    ui->mainToolBar->addAction(m_actFields);

    // All disabled until DB is opened
    m_actRecAppend->setEnabled(false);
    m_actRecInsert->setEnabled(false);
    m_actRecDelete->setEnabled(false);
    m_actRecEdit->setEnabled(false);
    m_actSubmit->setEnabled(false);
    m_actRevert->setEnabled(false);
    m_actFields->setEnabled(false);

    // Connect actions
    connect(m_actRecAppend, &QAction::triggered,
            this, &MainWindow::on_actRecAppend_triggered);
    connect(m_actRecInsert, &QAction::triggered,
            this, &MainWindow::on_actRecInsert_triggered);
    connect(m_actRecDelete, &QAction::triggered,
            this, &MainWindow::on_actRecDelete_triggered);
    connect(m_actRecEdit, &QAction::triggered,
            this, &MainWindow::on_actRecEdit_triggered);
    connect(m_actSubmit, &QAction::triggered,
            this, &MainWindow::on_actSubmit_triggered);
    connect(m_actRevert, &QAction::triggered,
            this, &MainWindow::on_actRevert_triggered);
    connect(m_actFields, &QAction::triggered,
            this, &MainWindow::on_actFields_triggered);

    // --- Search bar (hidden for now, toolbar not shown) ---
    QToolBar *searchBar = new QToolBar(tr("搜索"), this);
    searchBar->setMovable(false);
    searchBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    searchBar->hide();  // not shown, all toolbar hidden

    // StudID filter
    searchBar->addWidget(new QLabel(tr(" 学号: "), this));
    m_searchStudID = new QLineEdit(this);
    m_searchStudID->setMaximumWidth(120);
    m_searchStudID->setPlaceholderText(tr("输入学号"));
    searchBar->addWidget(m_searchStudID);

    // Status filter
    searchBar->addWidget(new QLabel(tr(" 状态: "), this));
    m_searchStatus = new QComboBox(this);
    m_searchStatus->setMaximumWidth(120);
    m_searchStatus->addItem(tr("全部"), QVariant());
    m_searchStatus->addItem(tr("出勤"), "present");
    m_searchStatus->addItem(tr("迟到"), "late");
    m_searchStatus->addItem(tr("缺勤"), "absent");
    m_searchStatus->addItem(tr("请假"), "leave");
    searchBar->addWidget(m_searchStatus);

    // Date range
    searchBar->addWidget(new QLabel(tr(" 从: "), this));
    m_searchDateFrom = new QDateEdit(QDate::currentDate().addMonths(-1), this);
    m_searchDateFrom->setCalendarPopup(true);
    m_searchDateFrom->setDisplayFormat("yyyy-MM-dd");
    m_searchDateFrom->setMaximumWidth(120);
    searchBar->addWidget(m_searchDateFrom);

    searchBar->addWidget(new QLabel(tr(" 到: "), this));
    m_searchDateTo = new QDateEdit(QDate::currentDate(), this);
    m_searchDateTo->setCalendarPopup(true);
    m_searchDateTo->setDisplayFormat("yyyy-MM-dd");
    m_searchDateTo->setMaximumWidth(120);
    searchBar->addWidget(m_searchDateTo);

    // Search button
    m_searchBtn = new QPushButton(tr("🔍 搜索"), this);
    searchBar->addWidget(m_searchBtn);
    connect(m_searchBtn, &QPushButton::clicked,
            this, &MainWindow::on_recordsSearchClicked);

    // Clear button
    QPushButton *clearBtn = new QPushButton(tr("✕ 清除"), this);
    searchBar->addWidget(clearBtn);
    connect(clearBtn, &QPushButton::clicked, this, [this]() {
        m_searchStudID->clear();
        m_searchStatus->setCurrentIndex(0);
        m_searchDateFrom->setDate(QDate::currentDate().addMonths(-1));
        m_searchDateTo->setDate(QDate::currentDate());
        if (m_attModel) {
            m_attModel->setFilter("");
            m_attModel->select();
            ui->statusBar->showMessage(
                tr("共 %1 条记录").arg(m_attModel->rowCount()));
        }
    });

    // Configure records table view
    ui->recordsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->recordsTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->recordsTableView->setAlternatingRowColors(true);
    ui->recordsTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Double-click edit (auto-connected via on_recordsTableView_doubleClicked)
}

void MainWindow::clearModels()
{
    if (ui->recordsTableView->model() == m_attModel) {
        ui->recordsTableView->setModel(nullptr);
    }
    if (m_attModel) {
        delete m_attModel;
        m_attModel = nullptr;
    }
    if (ui->recordsTableView->selectionModel() == m_selModel) {
        ui->recordsTableView->setSelectionModel(nullptr);
    }
    if (m_selModel) {
        delete m_selModel;
        m_selModel = nullptr;
    }
}

void MainWindow::onRecordsTabActivated()
{
    if (!m_attModel && m_db.isOpen()) {
        clearModels();

        m_attModel = new QSqlRelationalTableModel(this, m_db);
        m_attModel->setTable("attendance");
        m_attModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
        m_attModel->setSort(m_attModel->fieldIndex("attDate"), Qt::DescendingOrder);

        // Relation: studID → studInfo.name
        m_attModel->setRelation(
            m_attModel->fieldIndex("studID"),
            QSqlRelation("studInfo", "studID", "name"));

        // Set headers
        m_attModel->setHeaderData(m_attModel->fieldIndex("studID"),
                                   Qt::Horizontal, QStringLiteral("学生"));
        m_attModel->setHeaderData(m_attModel->fieldIndex("attDate"),
                                   Qt::Horizontal, QStringLiteral("日期"));
        m_attModel->setHeaderData(m_attModel->fieldIndex("attTime"),
                                   Qt::Horizontal, QStringLiteral("时间"));
        m_attModel->setHeaderData(m_attModel->fieldIndex("status"),
                                   Qt::Horizontal, QStringLiteral("状态"));
        m_attModel->setHeaderData(m_attModel->fieldIndex("confidence"),
                                   Qt::Horizontal, QStringLiteral("置信度"));

        // Hide attID and face_image columns
        ui->recordsTableView->setItemDelegate(
            new QSqlRelationalDelegate(ui->recordsTableView));

        m_selModel = new QItemSelectionModel(m_attModel, this);
        connect(m_selModel, &QItemSelectionModel::currentChanged,
                this, [this](const QModelIndex &, const QModelIndex &) {
            if (m_attModel) {
                m_actSubmit->setEnabled(m_attModel->isDirty());
                m_actRevert->setEnabled(m_attModel->isDirty());
            }
        });

        ui->recordsTableView->setModel(m_attModel);
        ui->recordsTableView->setSelectionModel(m_selModel);

        // Hide internal columns
        ui->recordsTableView->setColumnHidden(
            m_attModel->fieldIndex("attID"), true);
        ui->recordsTableView->setColumnHidden(
            m_attModel->fieldIndex("face_image"), true);

        if (!m_attModel->select()) {
            qDebug() << "attendance select failed:" << m_attModel->lastError().text();
        }

        // Enable actions
        m_actRecAppend->setEnabled(true);
        m_actRecInsert->setEnabled(true);
        m_actRecDelete->setEnabled(true);
        m_actRecEdit->setEnabled(true);
        m_actFields->setEnabled(true);

        ui->statusBar->showMessage(
            tr("共 %1 条考勤记录").arg(m_attModel->rowCount()));
    }
}

// ---- Records CRUD ----

void MainWindow::on_actRecAppend_triggered()
{
    if (!m_attModel) return;

    Attendancedialog dlg(this);
    dlg.setStudents(m_studentNames);

    if (dlg.exec() == QDialog::Accepted) {
        QSqlRecord rec = dlg.record();
        int row = m_attModel->rowCount();
        if (!m_attModel->insertRecord(row, rec)) {
            QMessageBox::warning(this, tr("错误"),
                tr("插入记录失败: %1").arg(m_attModel->lastError().text()));
        } else {
            m_actSubmit->setEnabled(true);
            m_actRevert->setEnabled(true);
            ui->recordsTableView->selectRow(row);
        }
    }
}

void MainWindow::on_actRecInsert_triggered()
{
    if (!m_attModel) return;

    QModelIndex curIndex = ui->recordsTableView->currentIndex();
    int row = curIndex.isValid() ? curIndex.row() : m_attModel->rowCount();

    Attendancedialog dlg(this);
    dlg.setStudents(m_studentNames);

    if (dlg.exec() == QDialog::Accepted) {
        QSqlRecord rec = dlg.record();
        if (!m_attModel->insertRecord(row, rec)) {
            QMessageBox::warning(this, tr("错误"),
                tr("插入记录失败: %1").arg(m_attModel->lastError().text()));
        } else {
            m_actSubmit->setEnabled(true);
            m_actRevert->setEnabled(true);
            ui->recordsTableView->selectRow(row);
        }
    }
}

void MainWindow::on_actRecDelete_triggered()
{
    if (!m_attModel || !m_selModel) return;
    int row = m_selModel->currentIndex().row();
    if (row < 0) return;

    QString name = m_attModel->index(row,
        m_attModel->fieldIndex("studID")).data(Qt::DisplayRole).toString();
    QString date = m_attModel->index(row,
        m_attModel->fieldIndex("attDate")).data().toString();

    QMessageBox::StandardButton ret = QMessageBox::question(
        this, tr("确认删除"),
        tr("确定要删除这条考勤记录吗？\n\n学生: %1\n日期: %2")
            .arg(name, date),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (ret != QMessageBox::Yes) return;

    m_attModel->removeRow(row);
    m_actSubmit->setEnabled(true);
    m_actRevert->setEnabled(true);
}

void MainWindow::on_actRecEdit_triggered()
{
    if (!m_selModel) return;
    QModelIndex idx = m_selModel->currentIndex();
    if (idx.isValid())
        on_recordsTableView_doubleClicked(idx);
}

void MainWindow::on_actSubmit_triggered()
{
    if (!m_attModel) return;
    if (!m_attModel->submitAll())
        QMessageBox::information(this, tr("消息"),
            tr("数据保存错误\n%1").arg(m_attModel->lastError().text()));
    else {
        m_actSubmit->setEnabled(false);
        m_actRevert->setEnabled(false);
        refreshStatistics();
        ui->statusBar->showMessage(
            tr("共 %1 条考勤记录").arg(m_attModel->rowCount()));
    }
}

void MainWindow::on_actRevert_triggered()
{
    if (!m_attModel) return;
    m_attModel->revertAll();
    m_actSubmit->setEnabled(false);
    m_actRevert->setEnabled(false);
}

void MainWindow::on_actFields_triggered()
{
    if (!m_attModel) return;
    QSqlRecord emptyRec = m_attModel->record();
    QString str;
    for (int i = 0; i < emptyRec.count(); ++i)
        str += emptyRec.fieldName(i) + '\n';
    QMessageBox::information(this, tr("所有字段名"), str);
}

// P1 fix: table-qualified column names in filter
void MainWindow::on_recordsSearchClicked()
{
    if (!m_attModel) return;

    QStringList filters;
    QString id = m_searchStudID->text().trimmed();
    QVariant statusData = m_searchStatus->currentData();
    QString dateFrom = m_searchDateFrom->date().toString("yyyy-MM-dd");
    QString dateTo = m_searchDateTo->date().toString("yyyy-MM-dd");

    if (!id.isEmpty()) {
        QString idEsc = id;
        idEsc.replace("'", "''");
        filters << QString("attendance.studID LIKE '%1'")
                       .arg("%" + idEsc + "%");
    }
    if (statusData.isValid()) {
        filters << QString("attendance.status='%1'")
                       .arg(statusData.toString());
    }
    filters << QString("attendance.attDate BETWEEN '%1' AND '%2'")
                   .arg(dateFrom, dateTo);

    QString filter = filters.join(" AND ");
    m_attModel->setFilter(filter);
    if (!m_attModel->select()) {
        qDebug() << "search select failed:" << m_attModel->lastError().text();
    }
    ui->statusBar->showMessage(
        tr("共 %1 条考勤记录").arg(m_attModel->rowCount()));
}

// P2+P3 fixes: EditRole for FK, setRecord for writing
void MainWindow::on_recordsTableView_doubleClicked(const QModelIndex &index)
{
    if (!index.isValid() || !m_attModel) return;

    int row = index.row();
    QSqlRecord rec = m_attModel->record(row);

    // P2: Use EditRole to get actual FK values (not display text)
    int studCol = m_attModel->fieldIndex("studID");
    rec.setValue("studID",
                 m_attModel->index(row, studCol).data(Qt::EditRole));

    Attendancedialog dlg(this);
    dlg.setStudents(m_studentNames);
    dlg.setRecord(rec);

    if (dlg.exec() == QDialog::Accepted) {
        QSqlRecord newrec = dlg.record();
        // P3: setRecord writes directly, avoids silent FK failure
        m_attModel->setRecord(row, newrec);
        m_actSubmit->setEnabled(true);
        m_actRevert->setEnabled(true);
    }
}

// =====================================================================
// Tab 3: Statistics
// =====================================================================

void MainWindow::initStatisticsTab()
{
    // Style the stat cards
    QString cardStyle = R"(
        QLabel {
            background-color: #2d2d2d;
            color: #ffffff; font-weight: bold; font-size: 16px;
            border: 1px solid #3e3e3e; border-radius: 6px;
            border-left: 3px solid #007acc;
            padding: 12px;
        }
    )";
    ui->statTotalLabel->setStyleSheet(cardStyle);
    ui->statPresentLabel->setStyleSheet(cardStyle);
    ui->statRateLabel->setStyleSheet(cardStyle);

    ui->statsTitle->setStyleSheet(
        "QLabel { color: #e0e0e0; font-size: 14px; font-weight: bold; }");

    ui->statsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->statsTableView->setAlternatingRowColors(true);

    // --- Reset button ---
    QPushButton *resetBtn = new QPushButton(QStringLiteral("🔄 重置所有数据"), this);
    resetBtn->setStyleSheet(
        "QPushButton { background-color: #8b3a3a; color: #ffffff;"
        "font-weight: bold; font-size: 13px; border: none; border-radius: 4px;"
        "padding: 10px 24px; }"
        "QPushButton:hover { background-color: #c0392b; }"
        "QPushButton:pressed { background-color: #a93226; }");
    connect(resetBtn, &QPushButton::clicked, this, &MainWindow::onResetAll);
    // Insert the button into the stats layout (before the table)
    QVBoxLayout *statsLay = qobject_cast<QVBoxLayout*>(
        ui->tabStatistics->layout());
    if (statsLay) {
        statsLay->insertWidget(3, resetBtn, 0, Qt::AlignCenter);
    }
}

void MainWindow::refreshStatistics()
{
    if (!m_db.isOpen()) return;

    // Summary cards
    QSqlQuery q(m_db);

    // Total registered students (with face features)
    q.exec("SELECT COUNT(*) FROM studInfo WHERE face_features IS NOT NULL");
    int total = q.next() ? q.value(0).toInt() : 0;
    ui->statTotalLabel->setText(
        QStringLiteral("注册学生\n%1").arg(total));

    // Today present
    q.exec("SELECT COUNT(DISTINCT studID) FROM attendance "
           "WHERE attDate=date('now','localtime') AND status='present'");
    int present = q.next() ? q.value(0).toInt() : 0;
    ui->statPresentLabel->setText(
        QStringLiteral("今日出勤\n%1").arg(present));

    // Rate
    int rate = total > 0 ? (present * 100 / total) : 0;
    ui->statRateLabel->setText(
        QStringLiteral("出勤率\n%1%").arg(rate));

    // Per-student summary table
    QSqlQueryModel *model = new QSqlQueryModel(this);
    model->setQuery(
        "SELECT s.studID AS '学号', s.name AS '姓名', "
        "  COALESCE(SUM(CASE WHEN a.status='present' THEN 1 ELSE 0 END), 0) AS '出勤', "
        "  COALESCE(SUM(CASE WHEN a.status='late' THEN 1 ELSE 0 END), 0) AS '迟到', "
        "  COALESCE(SUM(CASE WHEN a.status='absent' THEN 1 ELSE 0 END), 0) AS '缺勤', "
        "  COALESCE(SUM(CASE WHEN a.status='leave' THEN 1 ELSE 0 END), 0) AS '请假' "
        "FROM studInfo s "
        "LEFT JOIN attendance a ON s.studID = a.studID "
        "GROUP BY s.studID "
        "ORDER BY s.studID",
        m_db);

    QAbstractItemModel *old = ui->statsTableView->model();
    ui->statsTableView->setModel(model);
    if (old && old != model) delete old;

    ui->statsTableView->horizontalHeader()->setStretchLastSection(true);
}

// =====================================================================
// Tab Change Handler
// =====================================================================

void MainWindow::on_tabWidget_currentChanged(int index)
{
    switch (index) {
    case 0: // Check-in
        // Start camera if not running
        if (!m_engine->cameraIsOpen()) {
            m_engine->openCamera(0);
        }
        refreshTodayList();
        // Disable records-specific actions
        if (m_actRecAppend) m_actRecAppend->setVisible(false);
        if (m_actRecInsert) m_actRecInsert->setVisible(false);
        if (m_actRecDelete) m_actRecDelete->setVisible(false);
        if (m_actRecEdit)   m_actRecEdit->setVisible(false);
        if (m_actSubmit)    m_actSubmit->setVisible(false);
        if (m_actRevert)    m_actRevert->setVisible(false);
        if (m_actFields)    m_actFields->setVisible(false);
        break;
    case 1: // Register
        refreshRegisteredList();
        if (m_actRecAppend) m_actRecAppend->setVisible(false);
        if (m_actRecInsert) m_actRecInsert->setVisible(false);
        if (m_actRecDelete) m_actRecDelete->setVisible(false);
        if (m_actRecEdit)   m_actRecEdit->setVisible(false);
        if (m_actSubmit)    m_actSubmit->setVisible(false);
        if (m_actRevert)    m_actRevert->setVisible(false);
        if (m_actFields)    m_actFields->setVisible(false);
        break;
    case 2: // Records
        initTodayAttendance();
        onRecordsTabActivated();
        // Show records-specific actions
        if (m_actRecAppend) m_actRecAppend->setVisible(true);
        if (m_actRecInsert) m_actRecInsert->setVisible(true);
        if (m_actRecDelete) m_actRecDelete->setVisible(true);
        if (m_actRecEdit)   m_actRecEdit->setVisible(true);
        if (m_actSubmit)    m_actSubmit->setVisible(true);
        if (m_actRevert)    m_actRevert->setVisible(true);
        if (m_actFields)    m_actFields->setVisible(true);
        break;
    case 3: // Statistics
        initTodayAttendance();
        refreshStatistics();
        if (m_actRecAppend) m_actRecAppend->setVisible(false);
        if (m_actRecInsert) m_actRecInsert->setVisible(false);
        if (m_actRecDelete) m_actRecDelete->setVisible(false);
        if (m_actRecEdit)   m_actRecEdit->setVisible(false);
        if (m_actSubmit)    m_actSubmit->setVisible(false);
        if (m_actRevert)    m_actRevert->setVisible(false);
        if (m_actFields)    m_actFields->setVisible(false);
        break;
    }
}

// =====================================================================
// Reset All Data
// =====================================================================

void MainWindow::onResetAll()
{
    QMessageBox::StandardButton btn = QMessageBox::warning(this,
        QStringLiteral("确认重置"),
        QStringLiteral("此操作将清空所有考勤记录和人脸注册数据，不可恢复！\n\n确定继续？"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (btn != QMessageBox::Yes) return;

    QSqlQuery q(m_db);

    // 1. Delete all attendance records
    q.exec("DELETE FROM attendance");
    int attDeleted = q.numRowsAffected();

    // 2. Clear face registration data
    q.exec("UPDATE studInfo SET face_features=NULL, face_image=NULL");
    int faceCleared = q.numRowsAffected();

    // 3. Reload engine cache
    m_engine->reloadCache(m_db);

    // 4. Initialize today's attendance fresh
    initTodayAttendance();

    // 5. Refresh all views
    refreshTodayList();
    refreshRegisteredList();
    refreshStatistics();

    // 6. Reset check-in status
    ui->checkinStatus->setText(QStringLiteral("就绪 - 等待打卡"));
    ui->checkinStatus->setStyleSheet(
        "QLabel { color: #9d9d9d; font-weight: bold; font-size: 15px; }");

    QMessageBox::information(this, QStringLiteral("重置完成"),
        QStringLiteral("已清空 %1 条考勤记录\n已清空 %2 名学生的人脸数据\n"
                       "已重新初始化今日考勤")
            .arg(attDeleted).arg(faceCleared));
}

// =====================================================================
// Student Cache
// =====================================================================

void MainWindow::loadStudentCache()
{
    m_studentNames.clear();

    if (!m_db.isOpen()) return;

    QSqlQuery q(m_db);
    q.exec("SELECT studID, name FROM studInfo ORDER BY studID");
    while (q.next()) {
        int64_t id = q.value(0).toLongLong();
        QString name = q.value(1).toString();
        m_studentNames.insert(id, name);
    }

    qDebug() << "Loaded" << m_studentNames.size() << "students";
}

// =====================================================================
// Toolbar Actions
// =====================================================================

void MainWindow::on_actOpenDB_triggered()
{
    QString aFile = QFileDialog::getOpenFileName(
        this, QStringLiteral("选择文件"), "",
        QStringLiteral("SQLite数据库(*.db3)"));
    if (aFile.isEmpty()) return;

    openDatabase(aFile);
}

void MainWindow::on_actRefresh_triggered()
{
    refreshTodayList();
    refreshRegisteredList();
    refreshStatistics();
    m_engine->reloadCache(m_db);
    ui->statusBar->showMessage(tr("已刷新"), 2000);
}

// =====================================================================
// Close Event
// =====================================================================

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_autoTimer) m_autoTimer->stop();
    if (m_engine) {
        m_engine->closeCamera();
    }
    // Save any pending changes
    if (m_attModel && m_attModel->isDirty()) {
        QMessageBox::StandardButton ret = QMessageBox::question(
            this, tr("未保存"),
            tr("考勤记录有未保存的修改，是否保存？"),
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (ret == QMessageBox::Yes) {
            m_attModel->submitAll();
        } else if (ret == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
    }
    event->accept();
}
