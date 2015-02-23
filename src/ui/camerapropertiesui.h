#ifndef CAMERAPROPERTIESUI_H
#define CAMERAPROPERTIESUI_H

#include <QWidget>

namespace Ui {
class CameraPropertiesUI;
}

class CameraPropertiesUI : public QWidget
{
    Q_OBJECT

public:
    explicit CameraPropertiesUI(QWidget *parent = 0);
    ~CameraPropertiesUI();

signals:
    void lensModelChanged(int);
    void lensRadiusChanged(QString);
    void focalLengthChanged(QString);
    void cameraControllerChanged(QString);

public slots:
    void onLensModelChanged(int);
    void onLensRadiusChanged(QString);
    void onFocalLengthChanged(QString);
    void onCameraControllerChanged(QString);

private slots:
    void on_cameraLensModel_activated(int index);

private:
    Ui::CameraPropertiesUI *ui;
};

#endif // CAMERAPROPERTIESUI_H
