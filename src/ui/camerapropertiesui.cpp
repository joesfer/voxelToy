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

void CameraPropertiesUI::onLensModelChanged(bool changed)
{
    emit lensModelChanged(changed);
}
void CameraPropertiesUI::onLensRadiusChanged(QString value)
{
    emit lensRadiusChanged(value);
}

void CameraPropertiesUI::onFocalLengthChanged(QString value)
{
    emit focalLengthChanged(value);
}
