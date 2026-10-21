#ifndef CAMERAWIDGET_H
#define CAMERAWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QImage>

class CameraWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CameraWidget(QWidget *parent = nullptr);

    void setFrame(const QImage &frame);
    QImage currentFrame() const { return m_currentFrame; }

private:
    QLabel *m_view;
    QImage  m_currentFrame;
};

#endif // CAMERAWIDGET_H
