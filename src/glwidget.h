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

public slots:
	void reloadShaders();
    void cameraFStopChanged(QString fstop);
	void cameraFocalLengthChanged(QString length);
	void cameraLensModelChanged(bool dof);
	void onPathtracerMaxPathLengthChanged(int);

protected:
	void initializeGL();
	void paintGL();
	void resizeGL(int width, int height);
	void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void keyPressEvent(QKeyEvent *);
	void updateCamera();
	void createVoxelDataTexture();
	bool reloadFocalDistanceShader();
	bool reloadTexturedShader();
	bool reloadAverageShader();
	bool reloadDDAShader();
	void drawFullscreenQuad();
	void createFramebuffer();

private:
    Imath::V3f lightDirection() const;
    void resetRender();
	void updateRenderSettings();

    QPoint           m_lastPos;
	Qt::MouseButtons m_lastMouseButtons;

	Imath::Box3f m_volumeBounds;
	Imath::V3i	 m_volumeResolution;

	int m_activeSampleTexture;
	int m_numberSamples;

	Camera m_camera;

	GLuint m_mainFBO;
	GLuint m_mainRBO;
    GLuint m_sampleTexture;
    GLuint m_averageTexture[2];
    GLuint m_noiseTexture;
	GLint m_textureDimensions[2];

	// Normalized coordinates in screen space from where the focal distance is
	// retrieved (the actual pixel coordinates are calculated as
	// m_screenFocalPoint * viewportSize;
	Imath::V2f m_screenFocalPoint;
	GLuint m_focalDistanceFBO;
	GLuint m_focalDistanceRBO;
    GLuint m_focalDistanceTexture;

    GLuint m_occupancyTexture;
    GLuint m_voxelColorTexture;

	DDAShaderSettings            m_settingsDDA;
	AccumulationShaderSettings   m_settingsAverage;
	TexturedShaderSettings       m_settingsTextured;
	FocalDistanceShaderSettings  m_settingsFocalDistance;

	enum TextureUnits
	{
		TEXTURE_UNIT_OCCUPANCY = 0,
		TEXTURE_UNIT_COLOR,
		TEXTURE_UNIT_SAMPLE,
		TEXTURE_UNIT_AVERAGE0,
		TEXTURE_UNIT_AVERAGE1,
		TEXTURE_UNIT_NOISE,
		TEXTURE_UNIT_FOCAL_DISTANCE
    };
	
	struct RenderSettings
	{
		// maximum path length allowed in the path tracer (1 = direct
		// illumination only).
		int m_pathtracerMaxPathLength;
	};

	RenderSettings m_renderSettings;

    static const unsigned int MAX_FRAME_SAMPLES = 256;
};
