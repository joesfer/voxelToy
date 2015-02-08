#include "tools/toolFocalDistance.h"

bool ToolFocalDistance::mousePressEvent(QMouseEvent *event, QSize widgetDimensions) 
{
	if ( !(event->buttons() & Qt::LeftButton) ) return false;

	m_renderer.requestAction((float)event->pos().x() / widgetDimensions.width(), 
						     (float)event->pos().y() / widgetDimensions.height(),
							 0, 0,
							 Renderer::PA_SELECT_FOCAL_POINT, 
							 true);
	return true;
}
