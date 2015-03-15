#include "ui/colorpicker.h"
#include <QColorDialog>
#include <QApplication>
#include <iostream>
ColorPickerButton::ColorPickerButton(QWidget* parent)
	: QPushButton(parent)
{
	connect(this, SIGNAL(released()), 
			this, SLOT(onClicked()));
}

void ColorPickerButton::onClicked()
{
	emit beginUserInteraction();

	QColor col = QColorDialog::getColor(this->getColor(), QApplication::activeWindow()); 
	if (col.isValid())
	{
		setColor(col);
	}

	emit endUserInteraction();
}

QColor ColorPickerButton::getColor() const
{
	return this->palette().button().color();  
}

void ColorPickerButton::setColor(QColor col)
{
	QColor disabledCol(col.red() / 2, col.green() / 2, col.blue() / 2);
	QString qss = QString("QPushButton { background-color: %1 }"\
						  "QPushButton:disabled { background-color: %2 }"
							).arg(col.name(), disabledCol.name());
	this->setStyleSheet(qss);
	emit colorChanged(col);
}
