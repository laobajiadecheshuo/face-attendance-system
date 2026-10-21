#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.setWindowTitle(QStringLiteral("学生考勤管理系统"));

    // Optional: open database from command line argument
    if (argc > 1) {
        QString filename = argv[1];
        w.openDatabase(filename);
    }

    w.resize(1100, 700);
    w.show();

    return a.exec();
}
