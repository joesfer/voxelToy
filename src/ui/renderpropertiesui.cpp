#include "renderpropertiesui.h"
#include "ui_renderpropertiesui.h"

RenderPropertiesUI::RenderPropertiesUI(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RenderPropertiesUI)
{
    ui->setupUi(this);
}

RenderPropertiesUI::~RenderPropertiesUI()
{
    delete ui;
}
void RenderPropertiesUI::onAmbientOcclusionEnabled(bool value)
{
    emit ambientOcclusionEnabled(value);
}

void RenderPropertiesUI::onAmbientOcclusionReachChanged(int value)
{
    emit ambientOcclusionReachChanged(value);
}

void RenderPropertiesUI::onAmbientOcclusionSpreadChanged(int value)
{
   emit ambientOcclusionSpreadChanged(value);
}
