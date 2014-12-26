#pragma once

#include "camera.h"
#include "shader.h"

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
    void cameraFStopChanged(QString fstop);
	void cameraFocalLengthChanged(QString length);
	void cameraFocalDistanceChanged(QString distance);
	void cameraLensModelChanged(bool dof);

protected:
	void initializeGL();
	void paintGL();
	void resizeGL(int width, int height);
	void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void keyPressEvent(QKeyEvent *);
	void updateCameraMatrices();
	void updateCameraLens();
	void createVoxelDataTexture();
	bool reloadTexturedShader();
	bool reloadAverageShader();
	bool reloadDDAShader();
	void drawFullscreenQuad();
	void createFramebuffer();

private:
    Imath::V3f lightDirection() const;

    QPoint           m_lastPos;
	Qt::MouseButtons m_lastMouseButtons;

	Imath::Box3f m_volumeBounds;
	Imath::V3i	 m_volumeResolution;

	int m_activeSampleTexture;
	int m_numberSamples;

	Camera m_camera;

	GLuint m_fbo;
	GLuint m_rbo;
    GLuint m_sampleTexture;
    GLuint m_averageTexture[2];
    GLuint m_noiseTexture;
	GLint m_textureDimensions[2];

    GLuint m_occupancyTexture;
    GLuint m_voxelColorTexture;

	DDAShaderSettings            m_settingsDDA;
	AccumulationShaderSettings   m_settingsAverage;
	TexturedShaderSettings       m_settingsTextured;

	enum TextureUnits
	{
		TEXTURE_UNIT_OCCUPANCY = 0,
		TEXTURE_UNIT_COLOR,
		TEXTURE_UNIT_SAMPLE,
		TEXTURE_UNIT_AVERAGE0,
		TEXTURE_UNIT_AVERAGE1,
		TEXTURE_UNIT_NOISE,
    };
	
    static const unsigned int MAX_FRAME_SAMPLES = 256;
};
