#ifndef ATTENDANCEDIALOG_H
#define ATTENDANCEDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QDateEdit>
#include <QTimeEdit>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QtSql>
#include <QMap>

class Attendancedialog : public QDialog
{
    Q_OBJECT
public:
    explicit Attendancedialog(QWidget *parent = nullptr);

    void setRecord(const QSqlRecord &rec);
    QSqlRecord record() const;

    void setStudents(const QMap<int64_t, QString> &students);

private slots:
    void validateAndAccept();

private:
    QComboBox       *cbStudent;     // studID → name
    QDateEdit       *deDate;
    QTimeEdit       *teTime;
    QComboBox       *cbStatus;      // present/late/absent/leave
    QDoubleSpinBox  *sbConfidence;
    QDialogButtonBox *buttonBox;

    QMap<int64_t, QString> m_students;
};

#endif // ATTENDANCEDIALOG_H
