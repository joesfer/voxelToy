#pragma once

#include "tools/tool.h"
#include "renderer/renderer.h"

class ToolAddRemoveVoxel : public Tool
{
public:
	ToolAddRemoveVoxel(Renderer& renderer) : m_renderer(renderer) {}
	virtual bool mousePressEvent(QMouseEvent* /*event*/, QSize /*widgetDimensions*/);
	virtual bool mouseMoveEvent(QMouseEvent *event, QSize widgetDimensions);

private:
	Renderer& m_renderer;
};

