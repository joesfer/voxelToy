#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
signals:
	void actionTriggered(int, bool);
	void beginUserInteraction();
	void endUserInteraction();

private slots:
    void on_actionReload_Shaders_triggered();

    void on_actionLoad_Mesh_triggered();

    void onResolutionSettingsChanged();

    void on_actionLoad_VOX_file_triggered();

    void on_actionSelect_Focal_Point_toggled(bool arg1);

    void on_actionAdd_Voxel_triggered(bool checked);

    void on_actionSave_Image_triggered();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
