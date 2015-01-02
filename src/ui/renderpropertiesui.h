#ifndef RENDERPROPERTIESUI_H
#define RENDERPROPERTIESUI_H

#include <QWidget>

namespace Ui {
class RenderPropertiesUI;
}

class RenderPropertiesUI : public QWidget
{
    Q_OBJECT

public:
    explicit RenderPropertiesUI(QWidget *parent = 0);
    ~RenderPropertiesUI();

signals:
   void ambientOcclusionEnabled(bool);
   void ambientOcclusionReachChanged(int);
   void ambientOcclusionSpreadChanged(int);

public slots:
   void onAmbientOcclusionEnabled(bool);
   void onAmbientOcclusionReachChanged(int);
   void onAmbientOcclusionSpreadChanged(int);

private:
    Ui::RenderPropertiesUI *ui;
};

#endif // RENDERPROPERTIESUI_H
