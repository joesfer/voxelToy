#include "tools/toolAddRemoveVoxel.h"
#include <QtGui>

bool ToolAddRemoveVoxel::mousePressEvent(QMouseEvent* event, QSize widgetDimensions) 
{
	bool res = false;
	const int dx = event->pos().x() - m_lastPos[0];
	const int dy = event->pos().y() - m_lastPos[1];
	const float fx = (float)event->pos().x() / widgetDimensions.width(); 
	const float fy = (float)event->pos().y() / widgetDimensions.height();
	const float fdx = (float)dx / widgetDimensions.width();
 	const float fdy = (float)dy / widgetDimensions.height();
	if ( event->buttons() & Qt::LeftButton ) 
	{
		if (event->modifiers() & Qt::ControlModifier)
		{
			m_renderer.requestAction(fx, fy,
									 fdx, fdy,
									 Action::PA_REMOVE_VOXEL,
									 true);
		}
		else
		{
			m_renderer.requestAction(fx, fy,
									 fdx, fdy,
									 Action::PA_ADD_VOXEL,
									 true);
		}
		res = true;	
	}

	m_lastPos[0] = event->pos().x();
	m_lastPos[1] = event->pos().y();

	return res;
}

bool ToolAddRemoveVoxel::mouseMoveEvent(QMouseEvent *event, QSize widgetDimensions)
{
	const int dx = event->pos().x() - m_lastPos[0];
	const int dy = event->pos().y() - m_lastPos[1];
	const float fx = (float)event->pos().x() / widgetDimensions.width(); 
	const float fy = (float)event->pos().y() / widgetDimensions.height();
	const float fdx = (float)dx / widgetDimensions.width();
 	const float fdy = (float)dy / widgetDimensions.height();

	m_renderer.requestAction(fx, fy,
							 fdx, fdy,
							 Action::PA_SELECT_ACTIVE_VOXEL, 
							 true);

	if ( event->buttons() & Qt::LeftButton ) 
	{
		if (event->modifiers() & Qt::ControlModifier)
		{
			m_renderer.requestAction(fx, fy,
									 fdx, fdy,
									 Action::PA_REMOVE_VOXEL,
									 true);
		}
		else
		{
			m_renderer.requestAction(fx, fy,
									 fdx, fdy,
									 Action::PA_ADD_VOXEL,
									 true);
		}
	}

	m_lastPos[0] = event->pos().x();
	m_lastPos[1] = event->pos().y();

    return false; // don't kill the event, let the camera process it as well
}

