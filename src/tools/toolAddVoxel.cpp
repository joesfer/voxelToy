#include "tools/toolAddVoxel.h"
#include <QtGui>

bool ToolAddVoxel::mouseReleaseEvent(QMouseEvent* /*event*/, QSize /*widgetDimensions*/) 
{
	return false;
}

bool ToolAddVoxel::mouseMoveEvent(QMouseEvent *event, QSize widgetDimensions)
{
	m_renderer.requestAction((float)event->pos().x() / widgetDimensions.width(), 
						     (float)event->pos().y() / widgetDimensions.height(),
							 Renderer::PA_SELECT_ACTIVE_VOXEL, 
							 true);
    return false; // don't kill the event, let the camera process it as well
}

