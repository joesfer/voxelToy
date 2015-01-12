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

    // Render properties widget

    connect(ui->renderProperties, SIGNAL(pathtracerMaxPathLengthChanged(int)),
            ui->glWidget, SLOT(onPathtracerMaxPathLengthChanged(int)));
    connect(ui->renderProperties, SIGNAL(resolutionSettingsChanged(void)),
            this, SLOT(onResolutionSettingsChanged(void)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_reloadShaders_clicked()
{

}

void MainWindow::on_cameraFStop_currentTextChanged(const QString &arg1)
{

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
