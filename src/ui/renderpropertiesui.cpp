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

void RenderPropertiesUI::onResolutionSettingsChanged()
{
    emit resolutionSettingsChanged();
}

void RenderPropertiesUI::getResolutionSettings(ResolutionMode &mode, int &axis1, int &axis2)
{
    if (!ui) return;
    axis1 = 0;
    axis2 = 0;

    if (ui->resolutionFixedRadioButton->isChecked())
    {
        mode = RM_FIXED;
        axis1 = ui->resolutionFixedWidthSpinBox->value();
        axis2 = ui->resolutionFixedHeightSpinBox->value();
    }
    else if(ui->resolutionLongestAxisRadioButton->isChecked())
    {
        mode = RM_LONGEST_AXIS;
        axis1 = ui->resolutionLongestAxisSpinBox->value();
    }
    else
    {
        mode = RM_MATCH_WINDOW;
    }
}
