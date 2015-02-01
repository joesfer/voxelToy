#include "tools/toolFocalDistance.h"

bool ToolFocalDistance::mousePressEvent(QMouseEvent *event, QSize widgetDimensions) 
{
	if ( !(event->buttons() & Qt::LeftButton) ) return false;

	m_renderer.pickingAction((float)event->pos().x() / widgetDimensions.width(), 
						     (float)event->pos().y() / widgetDimensions.height(),
							 Renderer::PA_SELECT_FOCAL_POINT);
	m_renderer.resetRender();
	return true;
}
