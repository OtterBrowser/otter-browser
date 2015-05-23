/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include <QtWidgets/QScrollBar>

namespace Otter
{

ContentsDialog::ContentsDialog(const QIcon &icon, const QString &title, const QString &text, const QString &details, QDialogButtonBox::StandardButtons buttons, QWidget *payload, QWidget *parent) : QFrame(parent),
	m_contentsLayout(new QBoxLayout(QBoxLayout::TopToBottom)),
	m_headerWidget(new QWidget(this)),
	m_closeLabel(new QLabel(m_headerWidget)),
	m_scrollArea(NULL),
	m_checkBox(NULL),
	m_buttonBox(NULL),
	m_isAccepted(false)
{
	QBoxLayout *mainLayout = new QBoxLayout(QBoxLayout::TopToBottom, this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);
	mainLayout->addWidget(m_headerWidget);

	setObjectName(QLatin1String("contentsDialog"));
	setStyleSheet(QStringLiteral("#contentsDialog {border:1px solid #CCC;border-radius:4px;background:%1;}").arg(palette().color(QPalette::Window).name()));

	m_headerWidget->setAutoFillBackground(true);
	m_headerWidget->setObjectName(QLatin1String("headerWidget"));
	m_headerWidget->setStyleSheet(QStringLiteral("#headerWidget {border-bottom:1px solid #CCC;border-top-left-radius:4px;border-top-right-radius:4px;background:%1;}").arg(palette().color(QPalette::Window).darker(50).name()));

	QBoxLayout *headerLayout = new QBoxLayout(QBoxLayout::LeftToRight, m_headerWidget);
	headerLayout->setContentsMargins(0, 0, 0, 0);
	headerLayout->setSpacing(0);

	m_headerWidget->setLayout(headerLayout);
	m_headerWidget->installEventFilter(this);

	QLabel *iconLabel = new QLabel(m_headerWidget);
	iconLabel->setToolTip(title);
	iconLabel->setPixmap(icon.pixmap(16, 16));
	iconLabel->setMargin(5);

	QFont font = this->font();
	font.setBold(true);

	QLabel *titleLabel = new QLabel(title, m_headerWidget);
	titleLabel->setToolTip(title);
	titleLabel->setFont(font);

	m_closeLabel->setToolTip(tr("Close"));
	m_closeLabel->setPixmap(Utils::getIcon(QLatin1String("window-close")).pixmap(16, 16));
	m_closeLabel->setMargin(5);
	m_closeLabel->installEventFilter(this);

	headerLayout->addWidget(iconLabel);
	headerLayout->addWidget(titleLabel, 1);
	headerLayout->addWidget(m_closeLabel);

	if (!payload || buttons != QDialogButtonBox::NoButton)
	{
		m_contentsLayout->setContentsMargins(9, 9, 9, 9);
		m_contentsLayout->setSpacing(6);
	}

	mainLayout->addWidget(m_headerWidget);
	mainLayout->addLayout(m_contentsLayout);

	if (!text.isEmpty())
	{
		QLabel *textLabel = new QLabel(text, this);
		textLabel->setTextFormat(Qt::PlainText);
		textLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

		m_contentsLayout->addWidget(textLabel);
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
			QLabel *label = new QLabel(details, scrollWidget);
			label->setTextFormat(Qt::PlainText);
			label->setTextInteractionFlags(Qt::TextBrowserInteraction);

			scrollLayout->addWidget(label);
		}

		if (payload)
		{
			payload->setParent(this);
			payload->setObjectName(QLatin1String("payloadWidget"));
			payload->setStyleSheet(QLatin1String("#payloadWidget {border-radius:4px;}"));
			payload->installEventFilter(this);

			scrollLayout->addWidget(payload);
		}

		scrollWidget->setLayout(scrollLayout);
		scrollWidget->adjustSize();

		m_contentsLayout->addWidget(m_scrollArea);
	}

	if (buttons != QDialogButtonBox::NoButton)
	{
		m_buttonBox = new QDialogButtonBox(buttons, Qt::Horizontal, this);

		m_contentsLayout->addWidget(m_buttonBox);

		connect(m_buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(clicked(QAbstractButton*)));
	}

	adjustSize();
	setContextMenuPolicy(Qt::PreventContextMenu);
	setFocusPolicy(Qt::StrongFocus);
	setWindowFlags(Qt::Widget);
	setWindowTitle(title);
	setLayout(mainLayout);
	installEventFilter(this);
}

void ContentsDialog::closeEvent(QCloseEvent *event)
{
	if (m_isAccepted)
	{
		emit accepted();
	}
	else
	{
		emit rejected();
	}

	emit finished();

	event->accept();
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
		switch (m_buttonBox->standardButton(button))
		{
			case QDialogButtonBox::Ok:
			case QDialogButtonBox::Apply:
			case QDialogButtonBox::Yes:
			case QDialogButtonBox::YesToAll:
				m_isAccepted = true;

				break;
			default:
				break;
		}
	}

	close();
}

void ContentsDialog::setCheckBox(const QString &text, bool state)
{
	if (!m_checkBox)
	{
		m_checkBox = new QCheckBox(this);

		m_contentsLayout->insertWidget((m_contentsLayout->count() - 1), m_checkBox);
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
	if (object == m_headerWidget)
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
			m_isAccepted = true;

			close();
		}
		else if (keyEvent->key() == Qt::Key_Escape)
		{
			m_isAccepted = false;

			close();
		}
	}

	return QWidget::eventFilter(object, event);
}

}
