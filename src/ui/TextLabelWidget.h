#ifndef OTTER_TEXTLABELWIDGET_H
#define OTTER_TEXTLABELWIDGET_H

#include <QtWidgets/QLineEdit>

namespace Otter
{

class TextLabelWidget : public QLineEdit
{
	Q_OBJECT

public:
	explicit TextLabelWidget(QWidget *parent = NULL);

	void setText(const QString &text);
};

}

#endif
