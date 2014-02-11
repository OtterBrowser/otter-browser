/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "ContentsDialog.h"
#include "../core/Utils.h"

#include <QtGui/QMouseEvent>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QScrollBar>

namespace Otter
{

ContentsDialog::ContentsDialog(const QIcon &icon, const QString &title, const QString &text, const QString &details, QDialogButtonBox::StandardButtons buttons, QWidget *payload, QWidget *parent) : QWidget(parent),
	m_iconLabel(new QLabel(this)),
	m_titleLabel(new QLabel(title, this)),
	m_closeLabel(new QLabel(this)),
	m_scrollArea(NULL),
	m_checkBox(NULL),
	m_buttonBox(NULL),
	m_isAccepted(false)
{
	QFont font = this->font();
	font.setBold(true);

	QPalette palette = this->palette();
	palette.setColor(QPalette::Window, palette.color(QPalette::Window).darker(50));

	m_iconLabel->setToolTip(title);
	m_iconLabel->setPixmap(icon.pixmap(16, 16));
	m_iconLabel->setAutoFillBackground(true);
	m_iconLabel->setPalette(palette);
	m_iconLabel->setMargin(5);
	m_iconLabel->installEventFilter(this);

	m_titleLabel->setToolTip(title);
	m_titleLabel->setFont(font);
	m_titleLabel->setPalette(palette);
	m_titleLabel->setAutoFillBackground(true);
	m_titleLabel->setMargin(5);
	m_titleLabel->installEventFilter(this);

	m_closeLabel->setToolTip(tr("Close"));
	m_closeLabel->setPixmap(Utils::getIcon(QLatin1String("window-close")).pixmap(16, 16));
	m_closeLabel->setAutoFillBackground(true);
	m_closeLabel->setPalette(palette);
	m_closeLabel->setMargin(5);
	m_closeLabel->installEventFilter(this);

	QBoxLayout *titleLayout = new QBoxLayout(QBoxLayout::LeftToRight);
	titleLayout->setContentsMargins(0, 0, 0, 0);
	titleLayout->setSpacing(0);
	titleLayout->addWidget(m_iconLabel);
	titleLayout->addWidget(m_titleLabel, 1);
	titleLayout->addWidget(m_closeLabel);

	QBoxLayout *mainLayout = new QBoxLayout(QBoxLayout::TopToBottom, this);
	mainLayout->setContentsMargins(3, 3, 3, 3);
	mainLayout->addLayout(titleLayout);

	if (!text.isEmpty())
	{
		QLabel *label = new QLabel(text, this);
		label->setTextFormat(Qt::PlainText);

		mainLayout->addWidget(label);
	}

	if (payload || !details.isEmpty())
	{
		m_scrollArea = new QScrollArea(this);

		QWidget *scrollWidget = new QWidget(m_scrollArea);
		scrollWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

		QBoxLayout *scrollLayout = new QBoxLayout(QBoxLayout::TopToBottom, scrollWidget);
		scrollLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
		scrollLayout->setContentsMargins(3, 3, 3, 3);

		m_scrollArea->setWidget(scrollWidget);

		if (!details.isEmpty())
		{
			scrollLayout->addWidget(new QLabel(details, scrollWidget));
		}

		if (payload)
		{
			scrollLayout->addWidget(payload);

			payload->installEventFilter(this);
		}

		scrollWidget->setLayout(scrollLayout);
		scrollWidget->adjustSize();

		mainLayout->addWidget(m_scrollArea);
	}

	if (buttons != QDialogButtonBox::NoButton)
	{
		m_buttonBox = new QDialogButtonBox(buttons, Qt::Horizontal, this);

		mainLayout->addWidget(m_buttonBox);

		connect(m_buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(clicked(QAbstractButton*)));
	}

	adjustSize();
	setAutoFillBackground(true);
	setFocusPolicy(Qt::StrongFocus);
	setWindowFlags(Qt::Widget);
	setWindowTitle(title);
	setLayout(mainLayout);
	installEventFilter(this);
}

void ContentsDialog::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);

	if (m_scrollArea)
	{
		m_scrollArea->setFrameShape(((m_scrollArea->horizontalScrollBar() && m_scrollArea->horizontalScrollBar()->isVisible()) || (m_scrollArea->verticalScrollBar() && m_scrollArea->verticalScrollBar()->isVisible())) ? QFrame::StyledPanel : QFrame::NoFrame);
	}
}

void ContentsDialog::adjustSize()
{
	QWidget::adjustSize();

	if (parentWidget())
	{
		if (height() >= parentWidget()->height())
		{
			resize(width(), qMax(100, parentWidget()->height() - 100));
		}

		if (width() >= parentWidget()->width())
		{
			resize(qMax(100, parentWidget()->width() - 100), height());
		}
	}
}

void ContentsDialog::clicked(QAbstractButton *button)
{
	if (m_buttonBox)
	{
		close(m_buttonBox->standardButton(button));
	}
	else
	{
		close();
	}
}

void ContentsDialog::close(QDialogButtonBox::StandardButton button)
{
	m_isAccepted = (button != QDialogButtonBox::NoButton && button != QDialogButtonBox::Cancel && button != QDialogButtonBox::No && button != QDialogButtonBox::Close && button != QDialogButtonBox::Abort && button != QDialogButtonBox::Discard);

	emit closed(m_isAccepted, button);

	if (m_isAccepted)
	{
		emit accepted();
	}
	else
	{
		emit rejected();
	}
}

void ContentsDialog::setCheckBox(const QString &text, bool state)
{
	if (!m_checkBox)
	{
		QBoxLayout *mainLayout = dynamic_cast<QBoxLayout*>(layout());

		if (!mainLayout)
		{
			return;
		}

		m_checkBox = new QCheckBox(this);

		mainLayout->insertWidget((mainLayout->count() - 1), m_checkBox);
	}

	m_checkBox->setText(text);
	m_checkBox->setChecked(state);

	adjustSize();
}

bool ContentsDialog::getCheckBoxState() const
{
	return (m_checkBox ? m_checkBox->isChecked() : false);
}

bool ContentsDialog::isAccepted() const
{
	return m_isAccepted;
}

bool ContentsDialog::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_titleLabel || object == m_iconLabel)
	{
		if (event->type() == QEvent::MouseButtonPress)
		{
			QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

			m_offset = mouseEvent->pos();
		}
		else if (event->type() == QEvent::MouseMove)
		{
			QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

			move(mapToParent(mouseEvent->pos()) - m_offset);
		}
	}
	else if (object == m_closeLabel && event->type() == QEvent::MouseButtonPress)
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

		if (mouseEvent && mouseEvent->button() == Qt::LeftButton)
		{
			close();
		}
	}
	else if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

		if ((keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return))
		{
			close(QDialogButtonBox::Ok);
		}
		else if (keyEvent->key() == Qt::Key_Escape)
		{
			close(QDialogButtonBox::Cancel);
		}
	}

	return QWidget::eventFilter(object, event);
}

}
