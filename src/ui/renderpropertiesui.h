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

    enum ResolutionMode
    {
        RM_FIXED,
        RM_LONGEST_AXIS,
        RM_MATCH_WINDOW
    };

    void getResolutionSettings(ResolutionMode& mode,
                               int& axis1,
                               int& axis2);

	void setBackground(QColor constantColor);
	void setBackground(QColor gradientFrom, QColor gradientTo);
	void setBackground(QString image);

signals:
	void pathtracerMaxSamplesChanged(int);
	void pathtracerMaxPathLengthChanged(int);
	void resolutionSettingsChanged();
	void wireframeOpacityChanged(int);
	void wireframeThicknessChanged(int);
    void backgroundColorChangedConstant(QColor);
    void backgroundColorChangedGradientFrom(QColor);
    void backgroundColorChangedGradientTo(QColor);

public slots:
	void onPathtracerMaxSamplesChanged(int);
	void onPathtracerMaxPathLengthChanged(int);
	void onResolutionSettingsChanged();
	void onWireframeOpacityChanged(int value);
	void onWireframeThicknessChanged(int value);
    void onBackgroundColorChangedConstant(QColor);
    void onBackgroundColorChangedGradientFrom(QColor);
    void onBackgroundColorChangedGradientTo(QColor);
    void onBackgroundColorConstant();
    void onBackgroundColorGradient();
    void onBackgroundColorImage();

private:
    Ui::RenderPropertiesUI *ui;
};

#endif // RENDERPROPERTIESUI_H
