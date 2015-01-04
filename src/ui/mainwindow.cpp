#include "ui/mainwindow.h"
#include "ui_mainwindow.h"
#include "ui/camerapropertiesui.h"

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
