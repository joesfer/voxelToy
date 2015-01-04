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
void RenderPropertiesUI::onPathtracerMaxPathLengthChanged(int value)
{
    emit pathtracerMaxPathLengthChanged(value);
}

