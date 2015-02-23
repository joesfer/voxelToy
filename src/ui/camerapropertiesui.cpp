#include "camerapropertiesui.h"
#include "ui_camerapropertiesui.h"

CameraPropertiesUI::CameraPropertiesUI(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CameraPropertiesUI)
{
    ui->setupUi(this);
}

CameraPropertiesUI::~CameraPropertiesUI()
{
    delete ui;
}

void CameraPropertiesUI::onLensModelChanged(int model)
{
    emit lensModelChanged(model);
}
void CameraPropertiesUI::onLensRadiusChanged(QString value)
{
    emit lensRadiusChanged(value);
}

void CameraPropertiesUI::onFocalLengthChanged(QString value)
{
    emit focalLengthChanged(value);
}

void CameraPropertiesUI::onCameraControllerChanged(QString value)
{
    emit cameraControllerChanged(value);
}

void CameraPropertiesUI::on_cameraLensModel_activated(int index)
{
   this->ui->cameraFStop->setEnabled(index == 1);  // thin lens
   this->ui->cameraFocalLength->setEnabled(index != 2);  // != orthographic
}
