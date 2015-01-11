#define GL_GLEXT_PROTOTYPES

#include "glwidget.h"

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

	std::string shaderPath(STRINGIFY(SHADER_DIR));
	shaderPath += QDir::separator().toLatin1();

	m_renderer.initialize(shaderPath);
}


void GLWidget::resizeGL(int width, int height)
{
	m_renderer.resizeFrame(width, height);
	update();
}

void GLWidget::paintGL()
{
	if (m_renderer.render() == Renderer::RR_SAMPLES_PENDING)
	{
		update();
	}
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    m_lastPos = event->pos();
    if (event->buttons() & Qt::LeftButton)
    {
		m_renderer.setScreenFocalPoint((float)event->pos().x() / width(), 
									   (float)event->pos().y() / height());
		update();
    }

}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - m_lastPos.x();
    int dy = event->y() - m_lastPos.y();

	m_renderer.onMouseMove(dx, dy, (int)event->buttons());

    m_lastPos = event->pos();
	m_lastMouseButtons = event->buttons();

	if ( !(event->buttons() & Qt::NoButton) ) update();

}

void GLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    m_lastPos = event->pos();
	m_lastMouseButtons = event->buttons();

	m_renderer.resetRender();
	update();
}

void GLWidget::keyPressEvent(QKeyEvent* /*event*/)
{
	update();
}


void GLWidget::cameraFStopChanged(QString fstop)
{
    m_renderer.camera().setFStop(fstop.toFloat());
    m_renderer.resetRender();
	update();
}

void GLWidget::cameraFocalLengthChanged(QString length)
{
    m_renderer.camera().setFocalLength(length.toFloat());
    m_renderer.resetRender();
	update();
}

void GLWidget::cameraLensModelChanged(bool dof)
{
	m_renderer.camera().setLensModel( dof ? Camera::CLM_THIN_LENS : Camera::CLM_PINHOLE );
    m_renderer.resetRender();
	update();
}

void GLWidget::onPathtracerMaxPathLengthChanged(int value)
{
	m_renderer.renderSettings().m_pathtracerMaxPathLength = value;
	m_renderer.updateRenderSettings();
}

void GLWidget::loadMesh(QString file)
{
	m_renderer.loadMesh(file.toStdString());
}

void GLWidget::reloadShaders()
{
	std::string shaderPath(STRINGIFY(SHADER_DIR));
	shaderPath += QDir::separator().toLatin1();

	m_renderer.reloadShaders(shaderPath);
}
