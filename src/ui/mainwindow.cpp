#include "ui/mainwindow.h"
#include "ui_mainwindow.h"
#include "ui/camerapropertiesui.h"

#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Camera properties widget

    connect(ui->cameraProperties, SIGNAL(lensModelChanged(bool)),
            ui->glWidget, SLOT(cameraLensModelChanged(bool)));
    connect(ui->cameraProperties, SIGNAL(lensRadiusChanged(QString)),
            ui->glWidget, SLOT(cameraFStopChanged(QString)));
    connect(ui->cameraProperties, SIGNAL(focalLengthChanged(QString)),
            ui->glWidget, SLOT(cameraFocalLengthChanged(QString)));
    connect(ui->cameraProperties, SIGNAL(cameraControllerChanged(QString)),
            ui->glWidget, SLOT(cameraControllerChanged(QString)));

    // Render properties widget

    connect(ui->renderProperties, SIGNAL(pathtracerMaxSamplesChanged(int)),
            ui->glWidget, SLOT(onPathtracerMaxSamplesChanged(int)));
    connect(ui->renderProperties, SIGNAL(pathtracerMaxPathLengthChanged(int)),
            ui->glWidget, SLOT(onPathtracerMaxPathLengthChanged(int)));
    connect(ui->renderProperties, SIGNAL(resolutionSettingsChanged(void)),
            this, SLOT(onResolutionSettingsChanged(void)));
    connect(ui->renderProperties, SIGNAL(wireframeOpacityChanged(int)),
            ui->glWidget, SLOT(onWireframeOpacityChanged(int)));
    connect(ui->renderProperties, SIGNAL(wireframeThicknessChanged(int)),
            ui->glWidget, SLOT(onWireframeThicknessChanged(int)));
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
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("OBJ files (*.obj)"));
    dialog.setViewMode(QFileDialog::Detail);
    if(dialog.exec())
    {
        QString file = dialog.selectedFiles()[0];
        ui->glWidget->loadMesh(file);
    }
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
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("VOX files (*.vox)"));
    dialog.setViewMode(QFileDialog::Detail);
    if(dialog.exec())
    {
        QString file = dialog.selectedFiles()[0];
        ui->glWidget->loadVoxFile(file);
    }
}

void MainWindow::on_actionSelect_Focal_Point_toggled(bool triggered)
{
   emit actionTriggered(GLWidget::ACTION_SELECT_FOCAL_POINT, triggered);
}

void MainWindow::on_actionAdd_Voxel_triggered(bool checked)
{
   emit actionTriggered(GLWidget::ACTION_EDIT_VOXELS, checked);
}
