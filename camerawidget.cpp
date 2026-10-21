#include "camerawidget.h"
#include <QVBoxLayout>

CameraWidget::CameraWidget(QWidget *parent)
    : QWidget(parent)
{
    m_view = new QLabel(this);
    m_view->setAlignment(Qt::AlignCenter);
    m_view->setMinimumSize(320, 240);
    m_view->setStyleSheet(
        "QLabel {"
        "  background-color: #0a0a1a;"
        "  border: 2px solid #0f3460;"
        "  border-radius: 6px;"
        "  color: #555;"
        "  font-size: 14px;"
        "}"
    );
    m_view->setText(QStringLiteral("摄像头未开启"));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);
}

void CameraWidget::setFrame(const QImage &frame)
{
    if (frame.isNull()) return;
    m_currentFrame = frame;

    QPixmap pix = QPixmap::fromImage(
        frame.scaled(m_view->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_view->setPixmap(pix);
}
