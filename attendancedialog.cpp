#include "attendancedialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QSqlField>
#include <QSqlRecord>
#include <QMetaType>
#include <QDebug>

Attendancedialog::Attendancedialog(QWidget *parent)
    : QDialog(parent),
      cbStudent(new QComboBox(this)),
      deDate(new QDateEdit(QDate::currentDate(), this)),
      teTime(new QTimeEdit(QTime::currentTime(), this)),
      cbStatus(new QComboBox(this)),
      sbConfidence(new QDoubleSpinBox(this)),
      buttonBox(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this))
{
    setWindowTitle(QStringLiteral("考勤记录编辑"));
    setMinimumWidth(420);
    setStyleSheet(R"(
        QDialog { background-color: #1e1e1e; }
        QLabel { color: #e0e0e0; font-weight: bold; font-size: 13px; }
        QComboBox, QDateEdit, QTimeEdit, QDoubleSpinBox {
            background-color: #2d2d2d; color: #d4d4d4;
            border: 1px solid #3e3e3e; border-radius: 4px;
            padding: 6px 10px; font-size: 13px;
        }
        QComboBox:hover, QDateEdit:hover, QTimeEdit:hover, QDoubleSpinBox:hover {
            border-color: #555555;
        }
        QComboBox:focus, QDateEdit:focus, QTimeEdit:focus, QDoubleSpinBox:focus {
            border-color: #007acc;
        }
        QComboBox QAbstractItemView {
            background-color: #252526; color: #d4d4d4;
            selection-background-color: #094771; border: 1px solid #3e3e3e;
        }
        QPushButton {
            background-color: #0e639c; color: #ffffff;
            font-weight: bold; font-size: 13px;
            border: none; border-radius: 4px;
            padding: 6px 20px; min-width: 80px;
        }
        QPushButton:hover { background-color: #1177bb; }
        QPushButton:pressed { background-color: #0d5685; }
    )");

    // Status options
    cbStatus->addItem(QStringLiteral("出勤"),   "present");
    cbStatus->addItem(QStringLiteral("迟到"),   "late");
    cbStatus->addItem(QStringLiteral("缺勤"),   "absent");
    cbStatus->addItem(QStringLiteral("请假"),   "leave");

    // Confidence spinbox
    sbConfidence->setRange(0.0, 1.0);
    sbConfidence->setDecimals(4);
    sbConfidence->setSingleStep(0.01);
    sbConfidence->setValue(0.0);

    // Date/Time display format
    deDate->setCalendarPopup(true);
    deDate->setDisplayFormat("yyyy-MM-dd");
    teTime->setDisplayFormat("HH:mm:ss");

    QFormLayout *form = new QFormLayout;
    form->addRow(QStringLiteral("学生:"), cbStudent);
    form->addRow(QStringLiteral("日期:"), deDate);
    form->addRow(QStringLiteral("时间:"), teTime);
    form->addRow(QStringLiteral("状态:"), cbStatus);
    form->addRow(QStringLiteral("置信度:"), sbConfidence);

    QVBoxLayout *mainLay = new QVBoxLayout;
    mainLay->addLayout(form);
    mainLay->addWidget(buttonBox);
    setLayout(mainLay);

    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &Attendancedialog::validateAndAccept);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &Attendancedialog::reject);
}

void Attendancedialog::setStudents(const QMap<int64_t, QString> &students)
{
    m_students = students;
    cbStudent->clear();
    cbStudent->addItem(QString(), QVariant()); // empty first
    for (auto it = m_students.cbegin(); it != m_students.cend(); ++it) {
        cbStudent->addItem(
            QString("%1 - %2").arg(it.key()).arg(it.value()),
            QVariant::fromValue<qint64>(it.key()));
    }
}

void Attendancedialog::setRecord(const QSqlRecord &rec)
{
    if (rec.isEmpty()) return;

    // studID
    QVariant sv = rec.value("studID");
    if (sv.isValid() && !sv.isNull()) {
        int idx = cbStudent->findData(sv);
        if (idx >= 0) cbStudent->setCurrentIndex(idx);
    }

    // attDate
    QVariant dv = rec.value("attDate");
    if (dv.isValid() && !dv.isNull())
        deDate->setDate(QDate::fromString(dv.toString(), "yyyy-MM-dd"));

    // attTime
    QVariant tv = rec.value("attTime");
    if (tv.isValid() && !tv.isNull())
        teTime->setTime(QTime::fromString(tv.toString(), "HH:mm:ss"));

    // status
    QString st = rec.value("status").toString();
    int sidx = cbStatus->findData(st);
    if (sidx >= 0) cbStatus->setCurrentIndex(sidx);

    // confidence
    QVariant cv = rec.value("confidence");
    if (cv.isValid() && !cv.isNull())
        sbConfidence->setValue(cv.toDouble());
    else
        sbConfidence->setValue(0.0);
}

QSqlRecord Attendancedialog::record() const
{
    QSqlRecord rec;
    rec.append(QSqlField("studID",     QMetaType::fromType<qint64>()));
    rec.append(QSqlField("attDate",    QMetaType::fromType<QString>()));
    rec.append(QSqlField("attTime",    QMetaType::fromType<QString>()));
    rec.append(QSqlField("status",     QMetaType::fromType<QString>()));
    rec.append(QSqlField("confidence", QMetaType::fromType<double>()));

    QVariant sd = cbStudent->currentData();
    if (sd.isValid())
        rec.setValue("studID", sd.toLongLong());
    else
        rec.setNull("studID");

    rec.setValue("attDate", deDate->date().toString("yyyy-MM-dd"));
    rec.setValue("attTime", teTime->time().toString("HH:mm:ss"));
    rec.setValue("status", cbStatus->currentData().toString());
    rec.setValue("confidence", sbConfidence->value());

    return rec;
}

void Attendancedialog::validateAndAccept()
{
    if (!cbStudent->currentData().isValid()) {
        QMessageBox::warning(this, QStringLiteral("校验失败"),
                             QStringLiteral("请选择学生"));
        return;
    }
    accept();
}
