#pragma once
#include <camera.h>

#include <GL/gl.h>
#include <QGLWidget>
#include <QOpenGLFunctions>

#include <OpenEXR/ImathMatrix.h>
#include <OpenEXR/ImathBox.h>

class GLWidget : public QGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT

public:
     GLWidget(QWidget *parent = 0);
     ~GLWidget();

     QSize minimumSizeHint() const;
     QSize sizeHint() const;

private slots:
	void reloadShaders();
	Imath::V3f lightDirection() const;

protected:
	void initializeGL();
	void paintGL();
	void resizeGL(int width, int height);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void keyPressEvent(QKeyEvent *);
	void updateCameraMatrices();
	void createDistanceTexture();

private:
	QPoint lastPos;

	Imath::Box3f m_volumeBounds;

	Camera m_camera;
    GLuint m_densityTexture;
	GLuint m_shader;

	GLuint m_uniformDistanceTexture;
	GLuint m_uniformVolumeBoundsMin;
	GLuint m_uniformVolumeBoundsMax;
	GLuint m_uniformViewport;
	GLuint m_uniformCameraInverseProj;
	GLuint m_uniformCameraInverseModelView;
	GLuint m_uniformLightDir;
};
