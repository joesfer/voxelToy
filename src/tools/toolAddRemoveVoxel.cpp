#include "tools/toolAddRemoveVoxel.h"
#include <QtGui>

bool ToolAddRemoveVoxel::mousePressEvent(QMouseEvent* event, QSize widgetDimensions) 
{
	if ( event->buttons() & Qt::LeftButton ) 
	{
		if (event->modifiers() & Qt::ControlModifier)
		{
			m_renderer.requestAction((float)event->pos().x() / widgetDimensions.width(), 
									 (float)event->pos().y() / widgetDimensions.height(),
									 Renderer::PA_REMOVE_VOXEL,
									 true);
		}
		else
		{
			m_renderer.requestAction((float)event->pos().x() / widgetDimensions.width(), 
									 (float)event->pos().y() / widgetDimensions.height(),
									 Renderer::PA_ADD_VOXEL,
									 true);
		}
		return true;
	}
	return false;
}

bool ToolAddRemoveVoxel::mouseMoveEvent(QMouseEvent *event, QSize widgetDimensions)
{
	m_renderer.requestAction((float)event->pos().x() / widgetDimensions.width(), 
						     (float)event->pos().y() / widgetDimensions.height(),
							 Renderer::PA_SELECT_ACTIVE_VOXEL, 
							 true);
	if ( event->buttons() & Qt::LeftButton ) 
	{
		if (event->modifiers() & Qt::ControlModifier)
		{
			m_renderer.requestAction((float)event->pos().x() / widgetDimensions.width(), 
									 (float)event->pos().y() / widgetDimensions.height(),
									 Renderer::PA_REMOVE_VOXEL,
									 true);
		}
		else
		{
			m_renderer.requestAction((float)event->pos().x() / widgetDimensions.width(), 
									 (float)event->pos().y() / widgetDimensions.height(),
									 Renderer::PA_ADD_VOXEL,
									 true);
		}
	}
    return false; // don't kill the event, let the camera process it as well
}

