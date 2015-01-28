#pragma once

#include "tools/tool.h"
#include "renderer/renderer.h"

class ToolFocalDistance : public Tool
{
public:
	ToolFocalDistance(Renderer& renderer) : m_renderer(renderer) {}
    virtual bool mouseReleaseEvent(QMouseEvent *event, QSize widgetDimensions);

private:
	Renderer& m_renderer;
};
