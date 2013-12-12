#include "ContentsDialog.h"

#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

namespace Otter
{

ContentsDialog::ContentsDialog(QWidget *payload, QWidget *parent) : QWidget(parent),
	m_payload(payload)
{
	QFont font = payload->font();
	font.setBold(true);

	QPalette palette = payload->palette();
	palette.setColor(QPalette::Window, palette.color(QPalette::Window).darker(50));

	QLabel *titleLabel = new QLabel(payload->windowTitle());
	titleLabel->setFont(font);
	titleLabel->setPalette(palette);
	titleLabel->setAutoFillBackground(true);
	titleLabel->setMargin(5);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(titleLabel);
	layout->addWidget(payload);

	setLayout(layout);

	payload->setWindowFlags(Qt::Widget);
	payload->setFocusPolicy(Qt::StrongFocus);

	adjustSize();
	setAutoFillBackground(true);

	connect(payload, SIGNAL(destroyed()), this, SLOT(deleteLater()));
}

}
