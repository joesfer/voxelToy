#include <GL/glut.h>
#include <GL/glu.h>

#include "glwidget.h"
#include "shader.h"
#include "content.h"

#include <QtGui>
#include <QtOpenGL>
#include <QtOpenGLExtensions/qopenglextensions.h>

#include <math.h>

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

#define FOCAL_DISTANCE_TEXTURE_RESOLUTION 1 

GLWidget::GLWidget(QWidget *parent)
     : QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
{
    m_camera.centerAt(Imath::V3f(0,0,0));
    m_camera.setDistanceToTarget(100);
    m_camera.setFStop(16);
	m_activeSampleTexture = 0;
	m_numberSamples = 0;
	m_screenFocalPoint = Imath::V2f(0.5f, 0.5f);

	m_renderSettings.m_ambientOcclusionEnabled = true;
	m_renderSettings.m_ambientOcclusionReach = 0.5f;
	m_renderSettings.m_ambientOcclusionSpread = 0.33f;
}

GLWidget::~GLWidget()
{
}

QSize GLWidget::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize GLWidget::sizeHint() const
{
	return QSize(400, 400);
}

void GLWidget::initializeGL()
{
	initializeOpenGLFunctions();

	glClearColor(0.1f, 0.1f, 0.1f, 0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

    glEnable(GL_TEXTURE_3D);
    if (glIsTexture(m_occupancyTexture)) glDeleteTextures(1, &m_occupancyTexture);
    glGenTextures(1, &m_occupancyTexture);
    if (glIsTexture(m_voxelColorTexture)) glDeleteTextures(1, &m_voxelColorTexture);
    glGenTextures(1, &m_voxelColorTexture);

	createVoxelDataTexture();
    createFramebuffer();
	reloadShaders();
	updateCamera();
	updateRenderSettings();
}

Imath::V3f GLWidget::lightDirection() const
{
	Imath::V3f d(1, -1, 1);
	return d.normalized();
}

bool GLWidget::reloadFocalDistanceShader()
{
	std::string shaderPath(STRINGIFY(SHADER_DIR));

	QString vsPath = QString(STRINGIFY(SHADER_DIR)) + QDir::separator() + QString("screenSpace.vs");
	QString fsPath = QString(STRINGIFY(SHADER_DIR)) + QDir::separator() + QString("focalDistance.fs");

	std::string vs(vsPath.toUtf8().constData());
	std::string fs(fsPath.toUtf8().constData());

    if ( !Shader::compileProgramFromFile("FocalDistance",
                                        vs, "",
                                        fs, "#define PINHOLE\n",
                                        m_settingsFocalDistance.m_shader) )
	{
		return false;
	}
    
	glUseProgram(m_settingsFocalDistance.m_shader);

	m_settingsFocalDistance.m_uniformVoxelOccupancyTexture  = glGetUniformLocation(m_settingsFocalDistance.m_shader, "occupancyTexture");
	m_settingsFocalDistance.m_uniformVoxelDataResolution    = glGetUniformLocation(m_settingsFocalDistance.m_shader, "voxelResolution");
	m_settingsFocalDistance.m_uniformVolumeBoundsMin        = glGetUniformLocation(m_settingsFocalDistance.m_shader, "volumeBoundsMin");
	m_settingsFocalDistance.m_uniformVolumeBoundsMax        = glGetUniformLocation(m_settingsFocalDistance.m_shader, "volumeBoundsMax");
	m_settingsFocalDistance.m_uniformViewport               = glGetUniformLocation(m_settingsFocalDistance.m_shader, "viewport");
	m_settingsFocalDistance.m_uniformCameraNear             = glGetUniformLocation(m_settingsFocalDistance.m_shader, "cameraNear");
	m_settingsFocalDistance.m_uniformCameraFar              = glGetUniformLocation(m_settingsFocalDistance.m_shader, "cameraFar");
	m_settingsFocalDistance.m_uniformCameraProj             = glGetUniformLocation(m_settingsFocalDistance.m_shader, "cameraProj");
	m_settingsFocalDistance.m_uniformCameraInverseProj      = glGetUniformLocation(m_settingsFocalDistance.m_shader, "cameraInverseProj");
	m_settingsFocalDistance.m_uniformCameraInverseModelView = glGetUniformLocation(m_settingsFocalDistance.m_shader, "cameraInverseModelView");
	m_settingsFocalDistance.m_uniformCameraFocalLength      = glGetUniformLocation(m_settingsFocalDistance.m_shader, "cameraFocalLength");             
	m_settingsFocalDistance.m_uniformSampledFragment        = glGetUniformLocation(m_settingsFocalDistance.m_shader, "sampledFragment");             

	glViewport(0,0,width(), height());
	glUniform4f(m_settingsFocalDistance.m_uniformViewport, 0, 0, (float)width(), (float)height());

	glUniform1i(m_settingsFocalDistance.m_uniformVoxelOccupancyTexture, TEXTURE_UNIT_OCCUPANCY);
	
	glUniform3i(m_settingsFocalDistance.m_uniformVoxelDataResolution, 
				m_volumeResolution.x, 
				m_volumeResolution.y, 
				m_volumeResolution.z);

	glUniform3f(m_settingsFocalDistance.m_uniformVolumeBoundsMin,
				m_volumeBounds.min.x,
				m_volumeBounds.min.y,
				m_volumeBounds.min.z);

	glUniform3f(m_settingsFocalDistance.m_uniformVolumeBoundsMax,
				m_volumeBounds.max.x,
				m_volumeBounds.max.y,
				m_volumeBounds.max.z);

	return true;
}

bool GLWidget::reloadTexturedShader()
{
	std::string shaderPath(STRINGIFY(SHADER_DIR));

	QString vsPath = QString(STRINGIFY(SHADER_DIR)) + QDir::separator() + QString("screenSpace.vs");
	QString fsPath = QString(STRINGIFY(SHADER_DIR)) + QDir::separator() + QString("textureMap.fs");

	std::string vs(vsPath.toUtf8().constData());
	std::string fs(fsPath.toUtf8().constData());

    if ( !Shader::compileProgramFromFile("textured",
                                        vs, "",
                                        fs, "",
                                        m_settingsTextured.m_shader) )
	{
		return false;
	}
    
	glUseProgram(m_settingsTextured.m_shader);

	m_settingsTextured.m_uniformTexture  = glGetUniformLocation(m_settingsTextured.m_shader, "texture");
	m_settingsTextured.m_uniformViewport = glGetUniformLocation(m_settingsTextured.m_shader, "viewport");

    glUniform4f(m_settingsTextured.m_uniformViewport, 0, 0, (float)width(), (float)height());

	return true;
}

bool GLWidget::reloadAverageShader()
{
	std::string shaderPath(STRINGIFY(SHADER_DIR));

	QString vsPath = QString(STRINGIFY(SHADER_DIR)) + QDir::separator() + QString("screenSpace.vs");
	QString fsPath = QString(STRINGIFY(SHADER_DIR)) + QDir::separator() + QString("accumulation.fs");

	std::string vs(vsPath.toUtf8().constData());
	std::string fs(fsPath.toUtf8().constData());

    if ( !Shader::compileProgramFromFile("accumulation",
                                        vs, "",
                                        fs, "",
                                        m_settingsAverage.m_shader) )
	{
		return false;
	}
    
	glUseProgram(m_settingsAverage.m_shader);

	m_settingsAverage.m_uniformSampleTexture  = glGetUniformLocation(m_settingsAverage.m_shader, "sampleTexture");
	m_settingsAverage.m_uniformAverageTexture = glGetUniformLocation(m_settingsAverage.m_shader, "averageTexture");
	m_settingsAverage.m_uniformSampleCount    = glGetUniformLocation(m_settingsAverage.m_shader, "sampleCount");
	m_settingsAverage.m_uniformViewport       = glGetUniformLocation(m_settingsAverage.m_shader, "viewport");

    glUniform4f(m_settingsAverage.m_uniformViewport, 0, 0, (float)width(), (float)height());

	return true;
}

bool GLWidget::reloadDDAShader()
{
	std::string shaderPath(STRINGIFY(SHADER_DIR));

	QString vsPath = QString(STRINGIFY(SHADER_DIR)) + QDir::separator() + QString("screenSpace.vs");
	QString fsPath = QString(STRINGIFY(SHADER_DIR)) + QDir::separator() + QString("dda.fs");

	std::string vs(vsPath.toUtf8().constData());
	std::string fs(fsPath.toUtf8().constData());

    if ( !Shader::compileProgramFromFile("dda",
                                        vs, "",
                                        fs, "#define PINHOLE\n#define THINLENS\n",
                                        m_settingsDDA.m_shader) )
	{
		return false;
	}
    
	glUseProgram(m_settingsDDA.m_shader);

	m_settingsDDA.m_uniformVoxelOccupancyTexture  = glGetUniformLocation(m_settingsDDA.m_shader, "occupancyTexture");
	m_settingsDDA.m_uniformVoxelColorTexture      = glGetUniformLocation(m_settingsDDA.m_shader, "voxelColorTexture");
	m_settingsDDA.m_uniformNoiseTexture           = glGetUniformLocation(m_settingsDDA.m_shader, "noiseTexture");
	m_settingsDDA.m_uniformFocalDistanceTexture   = glGetUniformLocation(m_settingsDDA.m_shader, "focalDistanceTexture");
	m_settingsDDA.m_uniformVoxelDataResolution    = glGetUniformLocation(m_settingsDDA.m_shader, "voxelResolution");
	m_settingsDDA.m_uniformVolumeBoundsMin        = glGetUniformLocation(m_settingsDDA.m_shader, "volumeBoundsMin");
	m_settingsDDA.m_uniformVolumeBoundsMax        = glGetUniformLocation(m_settingsDDA.m_shader, "volumeBoundsMax");
	m_settingsDDA.m_uniformViewport               = glGetUniformLocation(m_settingsDDA.m_shader, "viewport");
	m_settingsDDA.m_uniformCameraNear             = glGetUniformLocation(m_settingsDDA.m_shader, "cameraNear");
	m_settingsDDA.m_uniformCameraFar              = glGetUniformLocation(m_settingsDDA.m_shader, "cameraFar");
	m_settingsDDA.m_uniformCameraProj             = glGetUniformLocation(m_settingsDDA.m_shader, "cameraProj");
	m_settingsDDA.m_uniformCameraInverseProj      = glGetUniformLocation(m_settingsDDA.m_shader, "cameraInverseProj");
	m_settingsDDA.m_uniformCameraInverseModelView = glGetUniformLocation(m_settingsDDA.m_shader, "cameraInverseModelView");
	m_settingsDDA.m_uniformCameraFocalLength      = glGetUniformLocation(m_settingsDDA.m_shader, "cameraFocalLength");             
	m_settingsDDA.m_uniformCameraLensRadius       = glGetUniformLocation(m_settingsDDA.m_shader, "cameraLensRadius");
	m_settingsDDA.m_uniformCameraFilmSize         = glGetUniformLocation(m_settingsDDA.m_shader, "cameraFilmSize");
	m_settingsDDA.m_uniformLightDir               = glGetUniformLocation(m_settingsDDA.m_shader, "wsLightDir");
	m_settingsDDA.m_uniformSampleCount            = glGetUniformLocation(m_settingsDDA.m_shader, "sampleCount");
	m_settingsDDA.m_uniformEnableDOF              = glGetUniformLocation(m_settingsDDA.m_shader, "enableDOF");
	m_settingsDDA.m_uniformAmbientOcclusionEnable = glGetUniformLocation(m_settingsDDA.m_shader, "ambientOcclusionEnable");
	m_settingsDDA.m_uniformAmbientOcclusionReach  = glGetUniformLocation(m_settingsDDA.m_shader, "ambientOcclusionReach");
	m_settingsDDA.m_uniformAmbientOcclusionSpread = glGetUniformLocation(m_settingsDDA.m_shader, "ambientOcclusionSpread");

	glViewport(0,0,width(), height());
	glUniform4f(m_settingsDDA.m_uniformViewport, 0, 0, (float)width(), (float)height());

	Imath::V3f lightDir = lightDirection();
	glUniform3f(m_settingsDDA.m_uniformLightDir, lightDir.x, lightDir.y, -lightDir.z);

	glUniform1i(m_settingsDDA.m_uniformVoxelOccupancyTexture, TEXTURE_UNIT_OCCUPANCY);
	glUniform1i(m_settingsDDA.m_uniformVoxelColorTexture, TEXTURE_UNIT_COLOR);
	glUniform1i(m_settingsDDA.m_uniformNoiseTexture, TEXTURE_UNIT_NOISE);
	glUniform1i(m_settingsDDA.m_uniformFocalDistanceTexture, TEXTURE_UNIT_FOCAL_DISTANCE);
	
	glUniform3i(m_settingsDDA.m_uniformVoxelDataResolution, 
				m_volumeResolution.x, 
				m_volumeResolution.y, 
				m_volumeResolution.z);

	glUniform3f(m_settingsDDA.m_uniformVolumeBoundsMin,
				m_volumeBounds.min.x,
				m_volumeBounds.min.y,
				m_volumeBounds.min.z);

	glUniform3f(m_settingsDDA.m_uniformVolumeBoundsMax,
				m_volumeBounds.max.x,
				m_volumeBounds.max.y,
				m_volumeBounds.max.z);

    updateCamera();

	return true;
}

void GLWidget::reloadShaders()
{
	if (!reloadDDAShader() || 
		!reloadAverageShader() || 
		!reloadTexturedShader() ||
		!reloadFocalDistanceShader())
	{
		std::cout << "Shader loading failed" << std::endl;
    }
	updateCamera();
	updateRenderSettings();
}

void GLWidget::updateCamera()
{
	using namespace Imath;
	V3f eye     = m_camera.eye();
	V3f right   = m_camera.rightUnitVector();
	V3f up      = m_camera.upUnitVector();
	V3f forward = m_camera.forwardUnitVector();

	M44f mvm; // modelViewMatrix
	M44f pm; // projectionMatrix

	{ // gluLookAt
		mvm.makeIdentity();
        mvm.x[0][0] = right[0]    ; mvm.x[0][1] = right[1]    ; mvm.x[0][2] = right[2]    ; mvm.x[0][3] = -eye.dot(right) ;
        mvm.x[1][0] = up[0]       ; mvm.x[1][1] = up[1]       ; mvm.x[1][2] = up[2]       ; mvm.x[1][3] = -eye.dot(up);
        mvm.x[2][0] = -forward[0] ; mvm.x[2][1] = -forward[1] ; mvm.x[2][2] = -forward[2] ; mvm.x[2][3] = eye.dot(forward) ;
        mvm.x[3][0] = 0           ; mvm.x[3][1] = 0           ; mvm.x[3][2] = 0           ; mvm.x[3][3] = 1 ;
	}

	{ // gluPerspective
		const float a = (float)width() / height();
		const float n = m_camera.nearDistance();
		const float f = m_camera.farDistance();
		const float e = 1.0f / tan(m_camera.fovY()/2);

		pm.x[0][0] = e/a ; pm.x[0][1] = 0 ; pm.x[0][2] = 0           ; pm.x[0][3] = 0              ;
		pm.x[1][0] = 0   ; pm.x[1][1] = e ; pm.x[1][2] = 0           ; pm.x[1][3] = 0              ;
		pm.x[2][0] = 0   ; pm.x[2][1] = 0 ; pm.x[2][2] = (f+n)/(n-f) ; pm.x[2][3] = 2.0f*f*n/(n-f) ;
		pm.x[3][0] = 0   ; pm.x[3][1] = 0 ; pm.x[3][2] = -1          ; pm.x[3][3] = 0              ;
	}

    M44f invModelView = mvm.inverse();
    M44f invProj = pm.inverse();
	
	// FocalDistance shader

    glUseProgram(m_settingsFocalDistance.m_shader);

    glUniform1f(m_settingsFocalDistance.m_uniformCameraNear, m_camera.nearDistance());
	glUniform1f(m_settingsFocalDistance.m_uniformCameraFar, m_camera.farDistance());

    glUniformMatrix4fv(m_settingsFocalDistance.m_uniformCameraInverseModelView,
                       1,
                       GL_TRUE,
                       &invModelView.x[0][0]);

    glUniformMatrix4fv(m_settingsFocalDistance.m_uniformCameraProj,
					   1,
                       GL_TRUE,
					   &pm.x[0][0]);

    glUniformMatrix4fv(m_settingsFocalDistance.m_uniformCameraInverseProj,
					   1,
                       GL_TRUE,
					   &invProj.x[0][0]);
	
	// DDA shader 
	
    glUseProgram(m_settingsDDA.m_shader);

    glUniform1f(m_settingsDDA.m_uniformCameraNear, m_camera.nearDistance());
	glUniform1f(m_settingsDDA.m_uniformCameraFar, m_camera.farDistance());

    glUniformMatrix4fv(m_settingsDDA.m_uniformCameraInverseModelView,
                       1,
                       GL_TRUE,
                       &invModelView.x[0][0]);

    glUniformMatrix4fv(m_settingsDDA.m_uniformCameraProj,
					   1,
                       GL_TRUE,
					   &pm.x[0][0]);

    glUniformMatrix4fv(m_settingsDDA.m_uniformCameraInverseProj,
					   1,
                       GL_TRUE,
					   &invProj.x[0][0]);
	bool enableDOF = m_camera.lensModel() == Camera::CLM_THIN_LENS; 
	enableDOF &= !(m_lastMouseButtons & Qt::RightButton);

	// TODO the focal length can be extracted from the perspective matrix (FOV)
	// so we should choose either method, but not both.
    glUseProgram(m_settingsDDA.m_shader);
    glUniform1f(m_settingsDDA.m_uniformCameraFocalLength  , m_camera.focalLength());
    glUniform1f(m_settingsDDA.m_uniformCameraLensRadius   , m_camera.lensRadius());
    glUniform1i(m_settingsDDA.m_uniformEnableDOF          , enableDOF ? 1 : 0 );
    glUniform2f(m_settingsDDA.m_uniformCameraFilmSize     , m_camera.filmSize().x, 
															m_camera.filmSize().y);
    glUseProgram(m_settingsFocalDistance.m_shader);
    glUniform1f(m_settingsFocalDistance.m_uniformCameraFocalLength  , m_camera.focalLength());
	glUniform2f(m_settingsFocalDistance.m_uniformSampledFragment,
				m_screenFocalPoint[0] * width(), 
				(1.0f - m_screenFocalPoint[1]) * height()); // m_screenFocal point origin is at top-left, whilst glsl is bottom-left
		
}

void GLWidget::resizeGL(int width, int height)
{
	const float viewportAspectRatio = (float)width / height;
	if (viewportAspectRatio >= 1.0f) 
	{
		m_camera.setFilmSize(Camera::FILM_SIZE_35MM, Camera::FILM_SIZE_35MM / viewportAspectRatio);
	}
	else 
	{
		m_camera.setFilmSize(Camera::FILM_SIZE_35MM * viewportAspectRatio, Camera::FILM_SIZE_35MM);
	}
	updateCamera();

	glViewport(0,0,width, height);

    glUseProgram(m_settingsDDA.m_shader);
	glUniform4f(m_settingsDDA.m_uniformViewport, 0, 0, (float)width, (float)height);

    glUseProgram(m_settingsAverage.m_shader);
    glUniform4f(m_settingsAverage.m_uniformViewport, 0, 0, (float)width, (float)height);

    glUseProgram(m_settingsTextured.m_shader);
    glUniform4f(m_settingsTextured.m_uniformViewport, 0, 0, (float)width, (float)height);

    glUseProgram(m_settingsFocalDistance.m_shader);
    glUniform4f(m_settingsFocalDistance.m_uniformViewport, 0, 0, (float)width, (float)height);

    glUseProgram(0);
	
	// create texture
	for( int i = 0; i < 3; ++i )
	{
		switch(i)
		{
			case 0: 
                glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_SAMPLE);
				glBindTexture(GL_TEXTURE_2D, m_sampleTexture);
				break;
			case 1: 
                glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_AVERAGE0);
				glBindTexture(GL_TEXTURE_2D, m_averageTexture[0]);
				break;
			case 2: 
                glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_AVERAGE1);
				glBindTexture(GL_TEXTURE_2D, m_averageTexture[1]);
				break;
			default: break;
		}

		glTexImage2D(GL_TEXTURE_2D,
					 0,
					 GL_RGBA,
					 width,
					 height,
					 0,
					 GL_RGBA,
					 GL_FLOAT,
                     NULL);

	}

	// these should match the viewport resolution, but maybe some older hardware
	// still performs some power-of-two rounding; so we'll store the actual
	// values to match the viewport when rendering to texture.
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &m_textureDimensions[0]);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &m_textureDimensions[1]);
	
    glBindRenderbuffer(GL_RENDERBUFFER, m_mainRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, 
						  GL_DEPTH_COMPONENT,
						  width,
						  height);

    m_numberSamples = 0;
    update();
}

void GLWidget::drawFullscreenQuad()
{
	// draw quad
	glBegin(GL_QUADS);
		glVertex3f(  1.0f,  1.0f, m_camera.nearDistance());
		glVertex3f( -1.0f,  1.0f, m_camera.nearDistance());
		glVertex3f( -1.0f, -1.0f, m_camera.nearDistance());
		glVertex3f(  1.0f, -1.0f, m_camera.nearDistance());
	glEnd();
}

void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
	
	// calculate focal distance
	if (m_numberSamples == 0)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_focalDistanceFBO);

		glViewport(0,0,FOCAL_DISTANCE_TEXTURE_RESOLUTION,FOCAL_DISTANCE_TEXTURE_RESOLUTION);

		glUseProgram(m_settingsFocalDistance.m_shader);

		drawFullscreenQuad();
	}
const float viewportAspectRatio = (float)width() / height();
if (viewportAspectRatio >= 1.0f) 
{
	m_camera.setFilmSize(Camera::FILM_SIZE_35MM, Camera::FILM_SIZE_35MM / viewportAspectRatio);
}
else 
{
	m_camera.setFilmSize(Camera::FILM_SIZE_35MM * viewportAspectRatio, Camera::FILM_SIZE_35MM);
}
updateCamera();
	
    // render frame into m_sampleTexture

    glBindFramebuffer(GL_FRAMEBUFFER, m_mainFBO);

    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D,
                           m_sampleTexture, // where we'll write to
                           0);

    glUseProgram(m_settingsDDA.m_shader);
    glUniform1i(m_settingsDDA.m_uniformSampleCount, std::min(m_numberSamples, (int)MAX_FRAME_SAMPLES - 1));

	glViewport(0,0,m_textureDimensions[0], m_textureDimensions[1]);
    drawFullscreenQuad();

    // m_sampleTexture and m_average[active] ---> m_average[active^1]

    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D,
                           m_averageTexture[m_activeSampleTexture ^ 1], // where we'll write to
                           0);

    glUseProgram(m_settingsAverage.m_shader);
    glUniform1i(m_settingsAverage.m_uniformSampleTexture, TEXTURE_UNIT_SAMPLE);
    glUniform1i(m_settingsAverage.m_uniformAverageTexture, TEXTURE_UNIT_AVERAGE0 + m_activeSampleTexture);
    glUniform1i(m_settingsAverage.m_uniformSampleCount, m_numberSamples);
    drawFullscreenQuad();
	
    // finally draw to screen

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(m_settingsTextured.m_shader);
    glUniform1i(m_settingsTextured.m_uniformTexture, TEXTURE_UNIT_AVERAGE0 + (m_activeSampleTexture^1));
	glViewport(0,0,m_textureDimensions[0], m_textureDimensions[1]);
    drawFullscreenQuad();
	
	// run continuously?
    if ( m_numberSamples++ < (int)MAX_FRAME_SAMPLES)
    {
        m_activeSampleTexture ^= 1;
        update();
    }
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    m_lastPos = event->pos();
    if (event->buttons() & Qt::LeftButton)
    {
        m_screenFocalPoint.x = (float)event->pos().x() / width();
        m_screenFocalPoint.y = (float)event->pos().y() / height();
        updateCamera();
        m_numberSamples = 0;
        update();
    }

}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - m_lastPos.x();
    int dy = event->y() - m_lastPos.y();

    bool change = false;
    if (event->buttons() & Qt::RightButton)
    {
        const float speed = 1.f;
        const float theta = -(float)dy / this->height() * M_PI * speed + m_camera.rotationTheta();
        const float phi = -(float)dx / this->width() * 2.0f * M_PI * speed + m_camera.rotationPhi();

        m_camera.setOrbitRotation(theta, phi);
		change = true;
    }
    else if( event->buttons() & Qt::MiddleButton)
    {
        const float speed = 0.99f;
        m_camera.setDistanceToTarget( dy > 0 ? m_camera.distanceToTarget() * speed :
                                               m_camera.distanceToTarget() / speed );
		change = true;
    }

    m_lastPos = event->pos();
	m_lastMouseButtons = event->buttons();

	if ( change )
	{
		updateCamera();
		m_numberSamples = 0;
		update();
	}
}

void GLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    m_lastPos = event->pos();
	m_lastMouseButtons = event->buttons();

	updateCamera();
	m_numberSamples = 0;
	update();

}

void GLWidget::keyPressEvent(QKeyEvent* /*event*/)
{
    m_numberSamples = 0;
	update();
}

inline float remap(float uniform, float min, float max)
{
    return min + (max-min)*uniform;
}

void GLWidget::createFramebuffer()
{
    glEnable(GL_TEXTURE_2D);

    if (glIsTexture(m_sampleTexture)) glDeleteTextures(1, &m_sampleTexture);
    glGenTextures(1, &m_sampleTexture);

    if (glIsTexture(m_noiseTexture)) glDeleteTextures(1, &m_noiseTexture);
    glGenTextures(1, &m_noiseTexture);

    if (glIsTexture(m_averageTexture[0])) glDeleteTextures(2, m_averageTexture);
    glGenTextures(2, m_averageTexture);

    if (glIsTexture(m_focalDistanceTexture)) glDeleteTextures(1, &m_focalDistanceTexture);
    glGenTextures(1, &m_focalDistanceTexture);

	// create focal distance texture
	{
		// this is a 1x1 px texture
		glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_FOCAL_DISTANCE);
		glBindTexture(GL_TEXTURE_2D, m_focalDistanceTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexImage2D(GL_TEXTURE_2D,
					 0,
					 GL_R16F,
					 FOCAL_DISTANCE_TEXTURE_RESOLUTION, 
					 FOCAL_DISTANCE_TEXTURE_RESOLUTION, 
					 0,
					 GL_RED,
					 GL_FLOAT,
                     NULL);
	}
	
	// create noise texture
	{
		float* noise = (float*)malloc(MAX_FRAME_SAMPLES * 4 * sizeof(float));
        for( int i = 0; i < 4 * (int)MAX_FRAME_SAMPLES; ++i ) noise[i] = (float)rand()/RAND_MAX;
		glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_NOISE);
		glBindTexture(GL_TEXTURE_2D, m_noiseTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexImage2D(GL_TEXTURE_2D,
					 0,
					 GL_RGBA,
					 MAX_FRAME_SAMPLES, 
					 1,
					 0,
					 GL_RGBA,
					 GL_FLOAT,
                     noise);
		free(noise);
	}

	// create sample texture
	for( int i = 0; i < 3; ++i )
	{
		switch(i)
		{
			case 0: 
                glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_SAMPLE);
				glBindTexture(GL_TEXTURE_2D, m_sampleTexture);
				break;
			case 1: 
                glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_AVERAGE0);
				glBindTexture(GL_TEXTURE_2D, m_averageTexture[0]);
				break;
			case 2: 
                glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_AVERAGE1);
				glBindTexture(GL_TEXTURE_2D, m_averageTexture[1]);
				break;
			default: break;
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexImage2D(GL_TEXTURE_2D,
					 0,
					 GL_RGBA,
					 width(),
					 height(),
					 0,
					 GL_RGBA,
					 GL_FLOAT,
                     NULL);
	}

	// generate focal distance shader's fbo/rbo
	
    if (glIsRenderbuffer(m_focalDistanceRBO)) glDeleteRenderbuffers(1, &m_focalDistanceRBO);
    glGenRenderbuffers(1, &m_focalDistanceRBO);

    glBindRenderbuffer(GL_RENDERBUFFER, m_focalDistanceRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 
						  FOCAL_DISTANCE_TEXTURE_RESOLUTION, 
						  FOCAL_DISTANCE_TEXTURE_RESOLUTION);

    //
    if (glIsFramebuffer(m_focalDistanceFBO)) glDeleteFramebuffers(1, &m_focalDistanceFBO);
    glGenFramebuffers(1, &m_focalDistanceFBO);

    glBindFramebuffer(GL_FRAMEBUFFER, m_focalDistanceFBO);
	
	// attach texture to color0
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D,
                           m_focalDistanceTexture, // where we'll write to
                           0);
	
	// attach a texture to depth attachment point
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
							  GL_DEPTH_ATTACHMENT, 
							  GL_RENDERBUFFER,
                              m_focalDistanceRBO);
	
	// generate main fbo/rbo

    if (glIsRenderbuffer(m_mainRBO)) glDeleteRenderbuffers(1, &m_mainRBO);
    glGenRenderbuffers(1, &m_mainRBO);

    glBindRenderbuffer(GL_RENDERBUFFER, m_mainRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, 
						  GL_DEPTH_COMPONENT,
						  width(),
						  height());

    //
    if (glIsFramebuffer(m_mainFBO)) glDeleteFramebuffers(1, &m_mainFBO);
    glGenFramebuffers(1, &m_mainFBO);

    glBindFramebuffer(GL_FRAMEBUFFER, m_mainFBO);
	
	// attach a texture to depth attachment point

    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
							  GL_DEPTH_ATTACHMENT, 
							  GL_RENDERBUFFER,
                              m_mainRBO);

	// note the main fbo is not complete yet
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLWidget::createVoxelDataTexture()
{
    using namespace Imath;
	
	// Texture resolution 
    m_volumeResolution = V3i(256);

    const size_t numVoxels = (size_t)(m_volumeResolution.x * m_volumeResolution.y * m_volumeResolution.z);

    GLubyte* occupancyTexels = (GLubyte*)malloc(numVoxels * sizeof(GLubyte));
    RGB* colorTexels = (RGB*)malloc(numVoxels * sizeof(RGB));

    float sizeMultiplier = 100;
    m_volumeBounds = Box3f( V3f(-0.5f, -0.5f, -0.5f) * sizeMultiplier, V3f(0.5f, 0.5f, 0.5f) * sizeMultiplier );

    memset(occupancyTexels, 0, numVoxels * sizeof(GLubyte));

    VoxelTools::addPlane( V3f(0.5f,1,0.5f).normalize(), V3f(0,0,0),
                          m_volumeResolution, m_volumeBounds,
                          occupancyTexels, colorTexels );


	for( int i = 0; i < 30; ++i)
	{
		const V3f sphereCenter = m_volumeBounds.min + (m_volumeBounds.max - m_volumeBounds.min) * V3f((float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX);
        const float sphereRadius = remap((float)rand() / RAND_MAX, 0.01f, 0.1f) * m_volumeBounds.size()[m_volumeBounds.majorAxis()];

		VoxelTools::addVoxelSphere( sphereCenter, sphereRadius, 
									m_volumeResolution, m_volumeBounds,
                        			occupancyTexels, colorTexels );
	}


	// Upload texture data to card

	glActiveTexture( GL_TEXTURE0 + TEXTURE_UNIT_OCCUPANCY);
    glBindTexture(GL_TEXTURE_3D, m_occupancyTexture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage3D(GL_TEXTURE_3D,
				 0,
               	 GL_R8,
                 m_volumeResolution.x,
                 m_volumeResolution.y,
                 m_volumeResolution.z,
               	 0,
               	 GL_RED,
               	 GL_UNSIGNED_BYTE,
                 occupancyTexels);

	glActiveTexture( GL_TEXTURE0 + TEXTURE_UNIT_COLOR);
    glBindTexture(GL_TEXTURE_3D, m_voxelColorTexture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage3D(GL_TEXTURE_3D,
				 0,
               	 GL_RGB8,
                 m_volumeResolution.x,
                 m_volumeResolution.y,
                 m_volumeResolution.z,
               	 0,
               	 GL_RGB,
               	 GL_UNSIGNED_BYTE,
                 colorTexels);


    free(occupancyTexels);
    free(colorTexels);
}
void GLWidget::resetRender()
{
    updateCamera();
    m_numberSamples = 0;
    update();
}

void GLWidget::updateRenderSettings()
{
	glUseProgram(m_settingsDDA.m_shader);
    glUniform1i(m_settingsDDA.m_uniformAmbientOcclusionEnable, m_renderSettings.m_ambientOcclusionEnabled ? 1 : 0);
	glUniform1f(m_settingsDDA.m_uniformAmbientOcclusionReach, m_renderSettings.m_ambientOcclusionReach);
	glUniform1f(m_settingsDDA.m_uniformAmbientOcclusionSpread, m_renderSettings.m_ambientOcclusionSpread);

	glUseProgram(0);
    resetRender();
}

void GLWidget::cameraFStopChanged(QString fstop)
{
    m_camera.setFStop(fstop.toFloat());
    resetRender();
}

void GLWidget::cameraFocalLengthChanged(QString length)
{
    m_camera.setFocalLength(length.toFloat());
    resetRender();
}

void GLWidget::cameraLensModelChanged(bool dof)
{
	m_camera.setLensModel( dof ? Camera::CLM_THIN_LENS : Camera::CLM_PINHOLE );
    resetRender();
}

void GLWidget::onAmbientOcclusionEnabled(bool value)
{
	m_renderSettings.m_ambientOcclusionEnabled = value;
	updateRenderSettings();
}

void GLWidget::onAmbientOcclusionReachChanged(int value)
{
	m_renderSettings.m_ambientOcclusionReach = (float)value / 100;
	updateRenderSettings();
}

void GLWidget::onAmbientOcclusionSpreadChanged(int value)
{
	m_renderSettings.m_ambientOcclusionSpread = (float)value / 90;
	updateRenderSettings();
}
