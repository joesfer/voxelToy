#pragma once

#include <QtGui>

class Tool
{
public:
	virtual bool mousePressEvent(QMouseEvent *event, QSize widgetDimensions)   { return false; }
    virtual bool mouseReleaseEvent(QMouseEvent *event, QSize widgetDimensions) { return false; }
	virtual bool mouseMoveEvent(QMouseEvent *event, QSize widgetDimensions)    { return false; }
	virtual bool keyPressEvent(QKeyEvent *)                                    { return false; }
};
