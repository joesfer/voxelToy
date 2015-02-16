#define GL_GLEXT_PROTOTYPES

#ifdef QT5
#include <QtOpenGLExtensions/qopenglextensions.h>
#else
#include <GL/glew.h>
#endif
#include <QtGui>
#include <QtOpenGL>

#include "ui/glwidget.h"

#include "tools/toolFocalDistance.h"
#include "tools/toolAddRemoveVoxel.h"

#include <math.h>

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

GLWidget::GLWidget(QWidget *parent)
     : QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
{
    m_resolutionMode = RenderPropertiesUI::RM_MATCH_WINDOW;
	m_resolutionLongestAxis = 1024;
	m_activeTool = NULL;
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
#ifdef QT5
	initializeOpenGLFunctions();
#else
	glewInit();
#endif

	std::string shaderPath(STRINGIFY(SHADER_DIR));
	shaderPath += QDir::separator().toLatin1();

	m_renderer.initialize(shaderPath);
	m_renderer.updateRenderSettings();
}

void GLWidget::resizeRender(int renderW, int renderH, 
							int windowW, int windowH)
{
    int resW, resH;
    const float windowAR = (float)windowW / windowH;
	int viewportX, viewportY, viewportW, viewportH;
    const float renderAR = (float)renderW / renderH;

    switch(m_resolutionMode)
    {
    case RenderPropertiesUI::RM_FIXED:
		resW = renderW; 
		resH = renderH;
		if (windowAR >= renderAR)
		{
			/*  _____________________
			 * |###|             |###|
			 * |###|             |###|
			 * |###|             |###|
			 * |###|             |###|
			 * |###|             |###|
			 *  ---------------------
			 */
			viewportY = 0;
			viewportH = windowH;
			viewportW = windowH * renderAR;
			viewportX = (windowW - viewportW) / 2;
		}
		else
		{
			/*
			 *  ---------
			 * |#########|
			 * |#########|
			 * |#########|
			 * |---------|
			 * |         |
			 * |_________|
			 * |#########|
			 * |#########|
			 * |#########|
			 *  ---------
			 */
			viewportX = 0;
            viewportW = windowW;
			viewportH = windowW / renderAR;
			viewportY = (windowH - viewportH) / 2;
		}
        break;
    case RenderPropertiesUI::RM_LONGEST_AXIS:
        if (windowW >= windowH)
        {
            resW = m_resolutionLongestAxis;
            resH = resW / windowAR;
        }
        else
        {
            resH = m_resolutionLongestAxis;
            resW = resH * windowAR;
        }
        viewportX = viewportY = 0;
        viewportW = windowW;
        viewportH = windowH;
        break;
    case RenderPropertiesUI::RM_MATCH_WINDOW:
        resW = windowW;
        resH = windowH;
		viewportX = viewportY = 0;
		viewportW = windowW;
		viewportH = windowH;
        break;

    default: break;
    }

	m_renderer.resizeFrame(resW, resH,
						   viewportX, viewportY,
						   viewportW, viewportH);

	update();
}
void GLWidget::resizeGL(int width, int height)
{
	resizeRender(m_renderer.renderSettings().m_imageResolution.x,
				 m_renderer.renderSettings().m_imageResolution.y,
				 width, 
				 height);
}

void GLWidget::paintGL()
{
	if (m_renderer.render() == Renderer::RR_SAMPLES_PENDING)
	{
		update();
	}
	emit statusChanged(QString(m_renderer.getStatus().c_str()));
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
	if ( m_activeTool != NULL &&
		 m_activeTool->mousePressEvent(event, this->size()))
    {
		emit currentToolActioned();
		update();
    }
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
	if ( m_activeTool != NULL &&
		 m_activeTool->mouseMoveEvent(event, this->size()))
	{
		emit currentToolActioned();
		update();
		return;
	}

	// if event is not processed by active tool, hand it off to the renderer

    const int dx = event->x() - m_lastPos.x();
    const int dy = event->y() - m_lastPos.y();
	if (m_renderer.onMouseMove(dx, dy, (int)event->buttons()))
	{
		update();
	}

    m_lastPos = event->pos();
	m_lastMouseButtons = event->buttons();
}

void GLWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if ( m_activeTool != NULL &&
		 m_activeTool->mouseReleaseEvent(event, this->size()))
	{
		emit currentToolActioned();
		update();
		return;
	}

    m_lastPos = event->pos();
	m_lastMouseButtons = event->buttons();
}

void GLWidget::keyPressEvent(QKeyEvent* event)
{
	if ( m_activeTool != NULL &&
		 m_activeTool->keyPressEvent(event))
	{
		emit currentToolActioned();
		update();
		return;
	}

	// if event is not processed by active tool, hand it off to the renderer
	if (m_renderer.onKeyPress(event->key()))
	{
		update();
	}
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
	m_renderer.camera().enableDOF( dof );
    m_renderer.resetRender();
	update();
}
void GLWidget::cameraControllerChanged(QString mode)
{
	if ( mode == "orbit" )
	{
		m_renderer.camera().setCameraController(Camera::CCM_ORBIT);
	}
	else
	{
		m_renderer.camera().setCameraController(Camera::CCM_FLY);
	}
}

void GLWidget::onPathtracerMaxSamplesChanged(int value)
{
    m_renderer.renderSettings().m_pathtracerMaxSamples = value;
    m_renderer.updateRenderSettings();
	update();
}

void GLWidget::onPathtracerMaxPathLengthChanged(int value)
{
	m_renderer.renderSettings().m_pathtracerMaxPathLength = value;
	m_renderer.updateRenderSettings();
	update();
}

void GLWidget::onWireframeOpacityChanged(int value)
{
	m_renderer.renderSettings().m_wireframeOpacity = (float)value / 100;
	m_renderer.updateRenderSettings();
	update();
}

void GLWidget::onWireframeThicknessChanged(int value)
{
	m_renderer.renderSettings().m_wireframeThickness = (float)value / 1000;
	m_renderer.updateRenderSettings();
	update();
}

void GLWidget::loadMesh(QString file)
{
    m_renderer.loadMesh(file.toStdString());
}

void GLWidget::loadVoxFile(QString file)
{
    m_renderer.loadVoxFile(file.toStdString());
}

void GLWidget::reloadShaders()
{
	std::string shaderPath(STRINGIFY(SHADER_DIR));
	shaderPath += QDir::separator().toLatin1();

	m_renderer.reloadShaders(shaderPath);
}

void GLWidget::onResolutionSettingsChanged(RenderPropertiesUI::ResolutionMode mode,
                                           int axis1,
                                           int axis2)
{
    m_resolutionMode = mode;
    m_resolutionLongestAxis = axis1;
	resizeRender(axis1, axis2, width(), height());
}

void GLWidget::onActionTriggered(int action, bool triggered)
{
	if (!triggered) 
	{
		delete m_activeTool;
		m_activeTool = NULL;
		return;
	}

	switch(action)
	{
		case ACTION_SELECT_FOCAL_POINT:
		{
			m_activeTool = new ToolFocalDistance(m_renderer);
			break;
		}
		case ACTION_EDIT_VOXELS: 
		{
			m_activeTool = new ToolAddRemoveVoxel(m_renderer);
			break;
		}
		default: return;
	}
}

void GLWidget::onBackgroundColorChangedConstant(QColor color)
{
	m_renderer.renderSettings().m_backgroundImage = "";
	m_renderer.renderSettings().m_backgroundColor[0] = Imath::V3f(color.redF(), color.greenF(), color.blueF());
	m_renderer.renderSettings().m_backgroundColor[1] = Imath::V3f(color.redF(), color.greenF(), color.blueF());
	m_renderer.updateRenderSettings();
	update();
}
void GLWidget::onBackgroundColorChangedGradientFrom(QColor color)
{
	m_renderer.renderSettings().m_backgroundImage = "";
	m_renderer.renderSettings().m_backgroundColor[0] = Imath::V3f(color.redF(), color.greenF(), color.blueF());
	m_renderer.updateRenderSettings();
	update();
}
void GLWidget::onBackgroundColorChangedGradientTo(QColor color)
{
	m_renderer.renderSettings().m_backgroundImage = "";
	m_renderer.renderSettings().m_backgroundColor[1] = Imath::V3f(color.redF(), color.greenF(), color.blueF());
	m_renderer.updateRenderSettings();
	update();
}
void GLWidget::onBackgroundColorChangedImage(QString path)
{
	m_renderer.renderSettings().m_backgroundImage = path.toStdString();
	m_renderer.updateRenderSettings();
	update();
}
void GLWidget::onBackgroundImageRotationChanged(int rotation)
{
	m_renderer.renderSettings().m_backgroundRotationDegrees = rotation;
	m_renderer.updateRenderSettings();
	update();
}
