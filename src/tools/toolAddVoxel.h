#pragma once

#include "tools/tool.h"
#include "renderer/renderer.h"

class ToolAddVoxel : public Tool
{
public:
	ToolAddVoxel(Renderer& renderer) : m_renderer(renderer) {}
    virtual bool mouseReleaseEvent(QMouseEvent *event, QSize widgetDimensions);
	virtual bool mouseMoveEvent(QMouseEvent *event, QSize widgetDimensions);

private:
	Renderer& m_renderer;
};

