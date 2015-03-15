#pragma once
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>

#include <QGLWidget>
#ifdef QT5
#include <QOpenGLFunctions>
#endif

#include <OpenEXR/ImathVec.h>

#include "renderer/renderer.h"
#include "renderer/material/material.h"
#include "ui/renderpropertiesui.h"
#include "tools/tool.h"
#include "log/logger.h"

class GLWidget : public QGLWidget
#ifdef QT5
				 , protected QOpenGLFunctions
#endif
{
	Q_OBJECT

public:
     GLWidget(QWidget *parent = 0);
     ~GLWidget();

     QSize minimumSizeHint() const;
     QSize sizeHint() const;

	 enum Actions
	 {
		 ACTION_SELECT_FOCAL_POINT,
		 ACTION_EDIT_VOXELS
	 };
signals:
	void statusChanged(QString);
	void currentToolActioned();
	void logMessage(QString);
	void materialCreated(Material::SerializedData&);

public slots:
	void onActionTriggered(int, bool);
	void reloadShaders();
    void loadMesh(QString file);
    void loadVoxFile(QString file);
    void saveImage(QString file);

    void cameraFStopChanged(QString fstop);
	void cameraFocalLengthChanged(QString length);
	void cameraLensModelChanged(int model);
    void cameraControllerChanged(QString mode);
    void onPathtracerMaxSamplesChanged(int);
    void onPathtracerMaxPathBouncesChanged(int);
    void onResolutionSettingsChanged(RenderPropertiesUI::ResolutionMode mode, int axis1, int axis2);
    void onWireframeOpacityChanged(int);
    void onWireframeThicknessChanged(int);
    void onBackgroundColorChangedConstant(QColor);
    void onBackgroundColorChangedGradientFrom(QColor);
    void onBackgroundColorChangedGradientTo(QColor);
	void onBackgroundColorChangedImage(QString);
	void onBackgroundImageRotationChanged(int);
	void onLogMessage(QString);
	void onMaterialColorChanged(unsigned int dataOffset, QColor col);
	void onMaterialValueChanged(unsigned int dataOffset, float value);

	void onBeginUserInteraction();
	void onEndUserInteraction();

protected:
	void initializeGL();
	void paintGL();
	void resizeGL(int width, int height);
	void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void keyPressEvent(QKeyEvent *);
	void resizeRender(int renderW, int renderH, int windowW, int windowH);
private:
	unsigned int                       m_activeUserDialogs;
    QPoint                             m_lastPos;
    Qt::MouseButtons                   m_lastMouseButtons;
    Renderer                           m_renderer;
    RenderPropertiesUI::ResolutionMode m_resolutionMode;
    unsigned int                       m_resolutionLongestAxis;
	Tool*                              m_activeTool;
	QtLogger						   m_logger;

};
