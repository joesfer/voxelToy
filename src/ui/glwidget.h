#pragma once

#include <GL/gl.h>
#include <QGLWidget>
#include <QOpenGLFunctions>

#include <OpenEXR/ImathVec.h>

#include "../renderer.h"
#include "renderpropertiesui.h"

class GLWidget : public QGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT

public:
     GLWidget(QWidget *parent = 0);
     ~GLWidget();

     QSize minimumSizeHint() const;
     QSize sizeHint() const;

public slots:
	void reloadShaders();
    void loadMesh(QString file);

    void cameraFStopChanged(QString fstop);
	void cameraFocalLengthChanged(QString length);
	void cameraLensModelChanged(bool dof);
	void onPathtracerMaxPathLengthChanged(int);
    void onResolutionSettingsChanged(RenderPropertiesUI::ResolutionMode mode, int axis1, int axis2);

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
    QPoint                             m_lastPos;
    Qt::MouseButtons                   m_lastMouseButtons;
    Renderer                           m_renderer;
    RenderPropertiesUI::ResolutionMode m_resolutionMode;
    unsigned int                       m_resolutionLongestAxis;
};
