#include <GL/glut.h>
#include <GL/glu.h>

#include <shader.h>
#include <glwidget.h>
#include <noise.h>

#include <QtGui>
#include <QtOpenGL>
#include <QtOpenGLExtensions/qopenglextensions.h>

#include <math.h>

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

GLWidget::GLWidget(QWidget *parent)
     : QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
{
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

	createDistanceTexture();
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

	QString vsPath = QString(STRINGIFY(SHADER_DIR)) + QDir::separator() + QString("raymarch.vs");
	QString fsPath = QString(STRINGIFY(SHADER_DIR)) + QDir::separator() + QString("raymarch.fs");

	std::string vs(vsPath.toUtf8().constData());
	std::string fs(fsPath.toUtf8().constData());

    if ( Shader::compileProgramFromFile("shader",
                                        vs, "",
                                        fs, "",
                                        m_shader) )
    {
        glUseProgram(m_shader);

		m_uniformDistanceTexture = glGetUniformLocation(m_shader, "distanceTexture");
		m_uniformVolumeBoundsMin = glGetUniformLocation(m_shader, "volumeBoundsMin");
		m_uniformVolumeBoundsMax = glGetUniformLocation(m_shader, "volumeBoundsMax");
		m_uniformViewport = glGetUniformLocation(m_shader, "viewport");
		m_uniformCameraInverseProj = glGetUniformLocation(m_shader, "cameraInverseProj");
		m_uniformCameraInverseModelView = glGetUniformLocation(m_shader, "cameraInverseModelView");
		m_uniformLightDir = glGetUniformLocation(m_shader, "wsLightDir");

        glViewport(0,0,width(), height());
        glUniform4f(m_uniformViewport, 0, 0, (float)width(), (float)height());

		Imath::V3f lightDir = lightDirection();
        glUniform3f(m_uniformLightDir, lightDir.x, lightDir.y, -lightDir.z);

        updateCameraMatrices();

		glUniform3f(m_uniformVolumeBoundsMin, 
                    m_volumeBounds.min.x,
                    m_volumeBounds.min.y,
                    m_volumeBounds.min.z);
		glUniform3f(m_uniformVolumeBoundsMax,
                    m_volumeBounds.max.x,
                    m_volumeBounds.max.y,
                    m_volumeBounds.max.z);

		const int textureUnit = 0;
		glUniform1i(m_uniformDistanceTexture, textureUnit);
		glActiveTexture( GL_TEXTURE0 + textureUnit);
		glBindTexture(GL_TEXTURE_2D, m_densityTexture);
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
        mvm.x[0][0] = right[0] ; mvm.x[0][1] = up[0]  ; mvm.x[0][2] = -forward[0] ; mvm.x[0][3] = 0 ;
        mvm.x[1][0] = right[1] ; mvm.x[1][1] = up[1]  ; mvm.x[1][2] = -forward[1] ; mvm.x[1][3] = 0 ;
        mvm.x[2][0] = right[2] ; mvm.x[2][1] = up[2]  ; mvm.x[2][2] = -forward[2] ; mvm.x[2][3] = 0 ;
        mvm.x[3][0] = -eye.x   ; mvm.x[3][1] = -eye.y ; mvm.x[3][2] = -eye.z      ; mvm.x[3][3] = 1 ;
	}

	{ // gluPerspective
		const float a = (float)width() / height();
		float n = 0.1f;
		float f = 100000.0f;
		const float e = 1.0f / tan(m_camera.fovY()/2);

		pm.x[0][0] = e ; pm.x[0][1] = 0     ; pm.x[0][2] = 0            ; pm.x[0][3] = 0               ;
		pm.x[1][0] = 0 ; pm.x[1][1] = e / a ; pm.x[1][2] = 0            ; pm.x[1][3] = 0               ;
		pm.x[2][0] = 0 ; pm.x[2][1] = 0     ; pm.x[2][2] = -(f+n)/(f-n) ; pm.x[2][3] = -2.0f*f*n/(f-n) ;
		pm.x[3][0] = 0 ; pm.x[3][1] = 0     ; pm.x[3][2] = -1           ; pm.x[3][3] = 0               ;
		pm.transpose();
	}

    M44f invModelView = mvm.inverse();
    glUniformMatrix4fv(m_uniformCameraInverseModelView,
                       1,
                       GL_FALSE,
                       &invModelView.x[0][0]);

    M44f invProj = pm.inverse();
    glUniformMatrix4fv(m_uniformCameraInverseProj,
					   1,
                       GL_FALSE,
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
		glTexCoord2f( 1.0f, 1.0f ); glVertex3f( 1.0f, 1.0f, 0.1f); 
		glTexCoord2f( 0.0f, 1.0f ); glVertex3f(-1.0f, 1.0f, 0.1f);  
		glTexCoord2f( 0.0f, 0.0f ); glVertex3f(-1.0f,-1.0f, 0.1f);  
		glTexCoord2f( 1.0f, 0.0f ); glVertex3f( 1.0f,-1.0f, 0.1f);  
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
        const float theta = (float)dy / this->height() * M_PI * speed + m_camera.rotationTheta();
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

void GLWidget::createDistanceTexture()
{
    using namespace Imath;
	// Texture resolution 
    size_t width  = 256;
    size_t height = 256;
    size_t depth  = 256;

    float* texels = (float*)malloc(width * height * depth * sizeof(float));

    m_volumeBounds = Box3f( V3f(-0.5f, -0.5f, -0.5f), V3f(0.5f, 0.5f, 0.5f) );

    V3f voxelSize(m_volumeBounds.size().x / width,
                  m_volumeBounds.size().y / height,
                  m_volumeBounds.size().z / depth);

	// Seed the distance values with a sphere centered in the volume bounds,
	// which surface is perturbed by Simplex noise. 
    const V3f sphereCenter = (m_volumeBounds.max + m_volumeBounds.min) * 0.5f;
    const float sphereRadius = 0.125f * (sphereCenter - m_volumeBounds.min).length();

    float octaves = 8;
    float persistence = 0.5f;
    float scale = 10.0f;
	float noiseScale = 0.025f;

    for( size_t k = 0; k < depth; ++k )
    {
        for( size_t j = 0; j < height; ++j )
        {
            for( size_t i = 0; i < width; ++i )
            {
                size_t offset = k * width * height + j * width + i;
                V3f voxelCenter = V3f(i + 0.5f, j + 0.5f, k + 0.5f) * voxelSize + m_volumeBounds.min;

                float distance = (voxelCenter - sphereCenter).length() - sphereRadius;
				float distOffset = octave_noise_3d( octaves,
													persistence,
													scale,
													(float)i/width,
													(float)j/width,
													(float)k/width) * noiseScale; 

                texels[offset] = distance + distOffset;
            }
        }
    }

	// Upload texture data to card

	if (glIsTexture(m_densityTexture)) glDeleteTextures(1, &m_densityTexture);
    glGenTextures(1, &m_densityTexture);
    glEnable(GL_TEXTURE_3D);
    glBindTexture(GL_TEXTURE_3D, m_densityTexture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage3D(GL_TEXTURE_3D,
				 0,
               	 GL_R32F,
               	 width,
               	 height,
               	 depth,
               	 0,
               	 GL_RED,
               	 GL_FLOAT,
                 texels);

    free(texels);
}
