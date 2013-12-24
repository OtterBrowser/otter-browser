#include "TextLabelWidget.h"

namespace Otter
{

TextLabelWidget::TextLabelWidget(QWidget *parent) : QLineEdit(parent)
{
	setFrame(false);
	setStyleSheet(QStringLiteral("QLineEdit {background:transparent;}"));
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	setReadOnly(true);
}

void TextLabelWidget::setText(const QString &text)
{
	QLineEdit::setText(text);
	setCursorPosition(0);
	setReadOnly(true);
}

}
