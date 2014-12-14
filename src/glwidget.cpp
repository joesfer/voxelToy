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

GLWidget::GLWidget(QWidget *parent)
     : QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
{
	m_camera.setDistanceToTarget(1.1f);
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
	reloadShaders();
}

Imath::V3f GLWidget::lightDirection() const
{
	Imath::V3f d(1, -1, 1);
	return d.normalized();
}

void GLWidget::reloadShaders()
{
	std::string shaderPath(STRINGIFY(SHADER_DIR));

	QString vsPath = QString(STRINGIFY(SHADER_DIR)) + QDir::separator() + QString("dda.vs");
	QString fsPath = QString(STRINGIFY(SHADER_DIR)) + QDir::separator() + QString("dda.fs");

	std::string vs(vsPath.toUtf8().constData());
	std::string fs(fsPath.toUtf8().constData());

    if ( Shader::compileProgramFromFile("shader",
                                        vs, "",
                                        fs, "",
                                        m_shader) )
    {
        glUseProgram(m_shader);

		m_uniformVoxelOccupancyTexture  = glGetUniformLocation(m_shader, "occupancyTexture");
		m_uniformVoxelColorTexture      = glGetUniformLocation(m_shader, "voxelColorTexture");
		m_uniformVoxelDataResolution    = glGetUniformLocation(m_shader, "voxelResolution");
		m_uniformVolumeBoundsMin        = glGetUniformLocation(m_shader, "volumeBoundsMin");
		m_uniformVolumeBoundsMax        = glGetUniformLocation(m_shader, "volumeBoundsMax");
		m_uniformViewport               = glGetUniformLocation(m_shader, "viewport");
		m_uniformCameraNear             = glGetUniformLocation(m_shader, "cameraNear");
		m_uniformCameraFar              = glGetUniformLocation(m_shader, "cameraFar");
		m_uniformCameraProj             = glGetUniformLocation(m_shader, "cameraProj");
		m_uniformCameraInverseProj      = glGetUniformLocation(m_shader, "cameraInverseProj");
		m_uniformCameraInverseModelView = glGetUniformLocation(m_shader, "cameraInverseModelView");
		m_uniformLightDir               = glGetUniformLocation(m_shader, "wsLightDir");

        glViewport(0,0,width(), height());
        glUniform4f(m_uniformViewport, 0, 0, (float)width(), (float)height());

		Imath::V3f lightDir = lightDirection();
        glUniform3f(m_uniformLightDir, lightDir.x, lightDir.y, -lightDir.z);

        updateCameraMatrices();

        glUniform1i(m_uniformVoxelOccupancyTexture, 0);
        glUniform1i(m_uniformVoxelColorTexture, 1);

		glUniform3i(m_uniformVoxelDataResolution, 
					m_volumeResolution.x, 
					m_volumeResolution.y, 
					m_volumeResolution.z);

		glUniform3f(m_uniformVolumeBoundsMin,
					m_volumeBounds.min.x,
					m_volumeBounds.min.y,
					m_volumeBounds.min.z);

		glUniform3f(m_uniformVolumeBoundsMax,
					m_volumeBounds.max.x,
					m_volumeBounds.max.y,
					m_volumeBounds.max.z);

    }
    else
    {
        std::cout << "Shader loading failed" << std::endl;
    }
}

void GLWidget::updateCameraMatrices()
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

	glUniform1f(m_uniformCameraNear, m_camera.nearDistance());
	glUniform1f(m_uniformCameraFar, m_camera.farDistance());

    M44f invModelView = mvm.inverse();
    glUniformMatrix4fv(m_uniformCameraInverseModelView,
                       1,
                       GL_TRUE,
                       &invModelView.x[0][0]);

    glUniformMatrix4fv(m_uniformCameraProj,
					   1,
                       GL_TRUE,
					   &pm.x[0][0]);

    M44f invProj = pm.inverse();
    glUniformMatrix4fv(m_uniformCameraInverseProj,
					   1,
                       GL_TRUE,
					   &invProj.x[0][0]);
	
}

void GLWidget::resizeGL(int width, int height)
{
	glViewport(0,0,width, height);
	glUniform4f(m_uniformViewport, 0, 0, (float)width, (float)height);
}

void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	
	// draw quad
	glBegin(GL_QUADS);
		glVertex3f( 1.0f, 1.0f, m_camera.nearDistance()) ;
		glVertex3f(-1.0f, 1.0f, m_camera.nearDistance()) ;
		glVertex3f(-1.0f,-1.0f, m_camera.nearDistance()) ;
		glVertex3f( 1.0f,-1.0f, m_camera.nearDistance()) ;
	glEnd();

	// run continuously?
    //update(); 
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    lastPos = event->pos();
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - lastPos.x();
    int dy = event->y() - lastPos.y();

    if (event->buttons() & Qt::LeftButton)
    {
        const float speed = 1.f;
        const float theta = -(float)dy / this->height() * M_PI * speed + m_camera.rotationTheta();
        const float phi = -(float)dx / this->width() * 2.0f * M_PI * speed + m_camera.rotationPhi();

        m_camera.setOrbitRotation(theta, phi);
    }
    else if( event->buttons() & Qt::RightButton)
    {
        const float speed = 0.99f;
        m_camera.setDistanceToTarget( dy > 0 ? m_camera.distanceToTarget() * speed :
                                               m_camera.distanceToTarget() / speed );
    }

    lastPos = event->pos();
	updateCameraMatrices();

	update();
}

void GLWidget::keyPressEvent(QKeyEvent* event)
{
	update();
}

inline float remap(float uniform, float min, float max)
{
    return min + (max-min)*uniform;
}

void GLWidget::createVoxelDataTexture()
{
    using namespace Imath;
	
	// Texture resolution 
    m_volumeResolution = V3i(256);

    const size_t numVoxels = (size_t)(m_volumeResolution.x * m_volumeResolution.y * m_volumeResolution.z);

    GLubyte* occupancyTexels = (GLubyte*)malloc(numVoxels * sizeof(GLubyte));
    RGB* colorTexels = (RGB*)malloc(numVoxels * sizeof(RGB));

    m_volumeBounds = Box3f( V3f(-0.5f, -0.5f, -0.5f), V3f(0.5f, 0.5f, 0.5f) );

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

	glActiveTexture( GL_TEXTURE0);
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

	glActiveTexture( GL_TEXTURE1);
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
