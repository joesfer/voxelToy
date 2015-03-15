#pragma once
#include <QPushButton>

class ColorPickerButton : public QPushButton
{
	Q_OBJECT
public:
	ColorPickerButton(QWidget* parent=0);
	QColor getColor() const;
	void setColor(QColor);
signals:
	void colorChanged(QColor);
	void beginUserInteraction();
	void endUserInteraction();
public slots:
	void onClicked();
};
