#include "tools/toolFocalDistance.h"

bool ToolFocalDistance::mouseReleaseEvent(QMouseEvent *event, QSize widgetDimensions) 
{
	m_renderer.pickingAction((float)event->pos().x() / widgetDimensions.width(), 
						     (float)event->pos().y() / widgetDimensions.height(),
							 Renderer::PA_SELECT_FOCAL_POINT);
	m_renderer.resetRender();
	return true;
}
