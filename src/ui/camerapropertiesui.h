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
    void lensModelChanged(bool);
    void lensRadiusChanged(QString);
    void focalLengthChanged(QString);

public slots:
    void onLensModelChanged(bool );
    void onLensRadiusChanged(QString);
    void onFocalLengthChanged(QString);

private:
    Ui::CameraPropertiesUI *ui;
};

#endif // CAMERAPROPERTIESUI_H
