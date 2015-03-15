#include "ui/mainwindow.h"
#include "ui_mainwindow.h"
#include "ui/camerapropertiesui.h"
#include "ui/colorpicker.h"

#include <assert.h>

#include <QFileDialog>
#include <QSlider>
#include <QVariant>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
	qRegisterMetaType<Material::SerializedData>("SerializedMaterialData");

	// GLWidget
	connect(this, SIGNAL(beginUserInteraction(void)), 
			ui->glWidget, SLOT(onBeginUserInteraction(void)));
	connect(this, SIGNAL(endUserInteraction(void)), 
			ui->glWidget, SLOT(onEndUserInteraction(void)));
	connect(ui->glWidget, SIGNAL(materialCreated(Material::SerializedData&)),
			this, SLOT(onMaterialCreated(Material::SerializedData&)));
	connect(this, SIGNAL(materialColorChanged(unsigned int, QColor)),
			ui->glWidget, SLOT(onMaterialColorChanged(unsigned int, QColor)));
	connect(this, SIGNAL(materialValueChanged(unsigned int, float)),
			ui->glWidget, SLOT(onMaterialValueChanged(unsigned int, float)));

    // Camera properties widget

    connect(ui->cameraProperties, SIGNAL(lensModelChanged(int)),
            ui->glWidget, SLOT(cameraLensModelChanged(int)));
    connect(ui->cameraProperties, SIGNAL(lensRadiusChanged(QString)),
            ui->glWidget, SLOT(cameraFStopChanged(QString)));
    connect(ui->cameraProperties, SIGNAL(focalLengthChanged(QString)),
            ui->glWidget, SLOT(cameraFocalLengthChanged(QString)));
    connect(ui->cameraProperties, SIGNAL(cameraControllerChanged(QString)),
            ui->glWidget, SLOT(cameraControllerChanged(QString)));

    // Render properties widget

    connect(ui->renderProperties, SIGNAL(pathtracerMaxSamplesChanged(int)),
            ui->glWidget, SLOT(onPathtracerMaxSamplesChanged(int)));
    connect(ui->renderProperties, SIGNAL(pathtracerMaxPathBouncesChanged(int)),
            ui->glWidget, SLOT(onPathtracerMaxPathBouncesChanged(int)));
    connect(ui->renderProperties, SIGNAL(resolutionSettingsChanged(void)),
            this, SLOT(onResolutionSettingsChanged(void)));
    connect(ui->renderProperties, SIGNAL(wireframeOpacityChanged(int)),
            ui->glWidget, SLOT(onWireframeOpacityChanged(int)));
    connect(ui->renderProperties, SIGNAL(wireframeThicknessChanged(int)),
            ui->glWidget, SLOT(onWireframeThicknessChanged(int)));
    connect(ui->renderProperties, SIGNAL(backgroundColorChangedConstant(QColor)),
            ui->glWidget, SLOT(onBackgroundColorChangedConstant(QColor)));
    connect(ui->renderProperties, SIGNAL(backgroundColorChangedGradientFrom(QColor)),
            ui->glWidget, SLOT(onBackgroundColorChangedGradientFrom(QColor)));
    connect(ui->renderProperties, SIGNAL(backgroundColorChangedGradientTo(QColor)),
            ui->glWidget, SLOT(onBackgroundColorChangedGradientTo(QColor)));
    connect(ui->renderProperties, SIGNAL(backgroundColorChangedImage(QString)),
            ui->glWidget, SLOT(onBackgroundColorChangedImage(QString)));
    connect(ui->renderProperties, SIGNAL(backgroundImageRotationChanged(int)),
            ui->glWidget, SLOT(onBackgroundImageRotationChanged(int)));
	connect(ui->renderProperties, SIGNAL(beginUserInteraction(void)), 
			ui->glWidget, SLOT(onBeginUserInteraction(void)));
	connect(ui->renderProperties, SIGNAL(endUserInteraction(void)), 
			ui->glWidget, SLOT(onEndUserInteraction(void)));


	// log window
    connect(ui->glWidget, SIGNAL(logMessage(QString)),
            ui->logWindow, SLOT(onLogMessage(QString)));

	ui->renderProperties->setBackground(QColor(192, 192, 192));
	ui->renderProperties->setBackground(QColor(153, 187, 201), QColor(77, 64, 50));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionReload_Shaders_triggered()
{
    ui->glWidget->reloadShaders();
}

void MainWindow::on_actionLoad_Mesh_triggered()
{
	emit beginUserInteraction();

    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("OBJ files (*.obj)"));
    dialog.setViewMode(QFileDialog::Detail);
    if(dialog.exec())
    {
        QString file = dialog.selectedFiles()[0];
        ui->glWidget->loadMesh(file);
    }

	emit endUserInteraction();
}

void MainWindow::onResolutionSettingsChanged()
{
    RenderPropertiesUI::ResolutionMode mode;
    int axis1, axis2;
   ui->renderProperties->getResolutionSettings(mode, axis1, axis2);
   ui->glWidget->onResolutionSettingsChanged(mode, axis1, axis2);
}

void MainWindow::on_actionLoad_VOX_file_triggered()
{
	emit beginUserInteraction();

    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("VOX files (*.vox)"));
    dialog.setViewMode(QFileDialog::Detail);
    if(dialog.exec())
    {
        QString file = dialog.selectedFiles()[0];
		ui->materialsTreeWidgets->clear();
        ui->glWidget->loadVoxFile(file);
    }

	emit endUserInteraction();
}

void MainWindow::on_actionSelect_Focal_Point_toggled(bool triggered)
{
   emit actionTriggered(GLWidget::ACTION_SELECT_FOCAL_POINT, triggered);
}

void MainWindow::on_actionAdd_Voxel_triggered(bool checked)
{
   emit actionTriggered(GLWidget::ACTION_EDIT_VOXELS, checked);
}

void MainWindow::on_actionSave_Image_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save File"),
                                                     "untitled.png",
                                                     tr("Images (*.png *.jpg)"));
    if(fileName.size() > 0)
    {
        ui->glWidget->saveImage(fileName);
    }

}

QWidget* MainWindow::appendMaterialProperty(const Material::SerializedData& data, QTreeWidgetItem* parent)
{
	QTreeWidgetItem* child = new QTreeWidgetItem(parent);
	parent->addChild(child);

	if ( data.m_propertyType == Material::SerializedData::PROPERTY_TYPE_COLOR )
	{
		assert(data.m_childProperties.size() == 3);
		ColorPickerButton* colorPicker = new ColorPickerButton(child->treeWidget());
		colorPicker->setColor(QColor((int)(data.m_childProperties[0].m_value * 255),
									 (int)(data.m_childProperties[1].m_value * 255),
									 (int)(data.m_childProperties[2].m_value * 255)));

		child->treeWidget()->setItemWidget(child, 0, colorPicker);

		connect(colorPicker, SIGNAL(beginUserInteraction(void)), 
				ui->glWidget, SLOT(onBeginUserInteraction(void)));
		connect(colorPicker, SIGNAL(endUserInteraction(void)), 
				ui->glWidget, SLOT(onEndUserInteraction(void)));
		connect(colorPicker, SIGNAL(colorChanged(QColor)), 
				this, SLOT(onMaterialColorChanged(QColor)));

		return colorPicker;
	}
	else if( data.m_propertyType == Material::SerializedData::PROPERTY_TYPE_FLOAT )
	{
		assert(data.m_childProperties.size() == 0);

		QSlider* slider = new QSlider(child->treeWidget());
		// FIXME: there's a loss of data precission here. Might not matter for
		// large values, but will do if we're dealing with [0,1] floats
		slider->setMinimum( (int)(data.m_dataRangeFrom) );
		slider->setMaximum( (int)(data.m_dataRangeTo) );
		slider->setValue( (int)(data.m_value) );
		slider->setOrientation(Qt::Horizontal);

		connect(slider, SIGNAL(valueChanged(int)), 
				this, SLOT(onMaterialValueChanged(int)));


		child->treeWidget()->setItemWidget(child, 0, slider);

		return slider;
	}

	return NULL;
}

void MainWindow::onMaterialCreated(Material::SerializedData& data)
{
	QTreeWidgetItem* item = new QTreeWidgetItem(ui->materialsTreeWidgets);
	item->setText(0, data.m_propertyName.c_str());
	for(size_t i = 0; i < data.m_childProperties.size(); ++i)
	{
		const Material::SerializedData& childProperty = data.m_childProperties[i];
		QWidget* widget = appendMaterialProperty(childProperty, item);
		if ( widget == NULL ) continue;

		widget->setToolTip(childProperty.m_propertyName.c_str());
		widget->setProperty("dataOffset", QVariant((uint)childProperty.m_dataOffset));

	}
	this->ui->materialsTreeWidgets->addTopLevelItem(item);
}

void MainWindow::onMaterialColorChanged(QColor col)
{
	QWidget* sender = (QWidget*)this->sender();
	if (sender == NULL) return;
	
	QVariant dataOffsetVariant = sender->property("dataOffset");
	if (!dataOffsetVariant.isValid()) return;

	bool valid = false;
	unsigned int dataOffset = dataOffsetVariant.toUInt(&valid);
	if (!valid) return;

	// re-emit signal with unpacked information
	emit materialColorChanged(dataOffset, col);
}

void MainWindow::onMaterialValueChanged(int value)
{
	QWidget* sender = (QWidget*)this->sender();
	if (sender == NULL) return;
	
	QVariant dataOffsetVariant = sender->property("dataOffset");
	if (!dataOffsetVariant.isValid()) return;

	bool valid = false;
	unsigned int dataOffset = dataOffsetVariant.toUInt(&valid);
	if (!valid) return;

	// re-emit signal with unpacked information
	emit materialValueChanged(dataOffset, (float)value);
}



