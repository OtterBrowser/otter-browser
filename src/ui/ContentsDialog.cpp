/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "ContentsDialog.h"
#include "../core/ThemesManager.h"

#include <QtGui/QMouseEvent>
#include <QtWidgets/QDialog>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QStyle>

namespace Otter
{

ContentsDialog::ContentsDialog(const QIcon &icon, const QString &title, const QString &text, const QString &details, QDialogButtonBox::StandardButtons buttons, QWidget *payload, QWidget *parent) : QFrame(parent),
	m_contentsLayout(new QBoxLayout(QBoxLayout::TopToBottom)),
	m_headerWidget(new QWidget(this)),
	m_payloadWidget(payload),
	m_closeLabel(new QLabel(m_headerWidget)),
	m_detailsLabel(nullptr),
	m_scrollArea(nullptr),
	m_checkBox(nullptr),
	m_buttonBox(nullptr),
	m_isAccepted(false)
{
	QBoxLayout *mainLayout(new QBoxLayout(QBoxLayout::TopToBottom, this));
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);
	mainLayout->addWidget(m_headerWidget);

	setObjectName(QLatin1String("contentsDialog"));
	setStyleSheet(QStringLiteral("#contentsDialog {border:1px solid #CCC;border-radius:4px;background:%1;}").arg(palette().color(QPalette::Window).name()));

	m_headerWidget->setAutoFillBackground(true);
	m_headerWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_headerWidget->setObjectName(QLatin1String("headerWidget"));
	m_headerWidget->setStyleSheet(QStringLiteral("#headerWidget {border-bottom:1px solid #CCC;border-top-left-radius:4px;border-top-right-radius:4px;background:%1;}").arg(palette().color(QPalette::Window).darker(50).name()));

	QBoxLayout *headerLayout(new QBoxLayout(QBoxLayout::LeftToRight, m_headerWidget));
	headerLayout->setContentsMargins(0, 0, 0, 0);
	headerLayout->setSpacing(0);

	m_headerWidget->setLayout(headerLayout);
	m_headerWidget->installEventFilter(this);

	QLabel *iconLabel(new QLabel(m_headerWidget));
	iconLabel->setToolTip(title);
	iconLabel->setPixmap(icon.pixmap(16, 16));
	iconLabel->setContentsMargins(5, 5, 5, 5);

	QFont font(this->font());
	font.setBold(true);

	QLabel *titleLabel(new QLabel(title, m_headerWidget));
	titleLabel->setToolTip(title);
	titleLabel->setFont(font);

	m_closeLabel->setToolTip(tr("Close"));
	m_closeLabel->setPixmap(ThemesManager::createIcon(QLatin1String("window-close")).pixmap(16, 16));
	m_closeLabel->setContentsMargins(5, 5, 5, 5);
	m_closeLabel->installEventFilter(this);

	headerLayout->addWidget(iconLabel);
	headerLayout->addWidget(titleLabel, 1);
	headerLayout->addWidget(m_closeLabel);

	mainLayout->addWidget(m_headerWidget);
	mainLayout->addLayout(m_contentsLayout);

	if (!text.isEmpty())
	{
		QLabel *textLabel(new QLabel(text, this));
		textLabel->setTextFormat(Qt::PlainText);
		textLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
		textLabel->setWordWrap(true);

		if (m_payloadWidget)
		{
			m_contentsLayout->addWidget(textLabel);
		}
		else
		{
			m_payloadWidget = textLabel;
		}
	}

	if (!m_payloadWidget || buttons != QDialogButtonBox::NoButton)
	{
		m_contentsLayout->setContentsMargins(9, 9, 9, 9);
		m_contentsLayout->setSpacing(6);
	}

	if (m_payloadWidget || !details.isEmpty())
	{
		m_scrollArea = new QScrollArea(this);
		m_scrollArea->setAlignment(Qt::AlignTop);
		m_scrollArea->setFrameStyle(QFrame::NoFrame);

		QWidget *scrollWidget(new QWidget(m_scrollArea));
		scrollWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

		QBoxLayout *scrollLayout(new QBoxLayout(QBoxLayout::TopToBottom, scrollWidget));
		scrollLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
		scrollLayout->setContentsMargins(3, 3, 3, 3);
		scrollLayout->setSpacing(3);

		m_scrollArea->setWidget(scrollWidget);

		if (!details.isEmpty())
		{
			m_detailsLabel = new QLabel(details, scrollWidget);
			m_detailsLabel->setTextFormat(Qt::PlainText);
			m_detailsLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

			scrollLayout->addWidget(m_detailsLabel);
		}

		if (m_payloadWidget)
		{
			m_payloadWidget->setParent(this);
			m_payloadWidget->setObjectName(QLatin1String("payloadWidget"));
			m_payloadWidget->setStyleSheet(QLatin1String("#payloadWidget {border-radius:4px;}"));
			m_payloadWidget->installEventFilter(this);

			if (m_payloadWidget->sizeHint().width() < 250)
			{
				m_payloadWidget->setMinimumWidth(250);
			}

			scrollLayout->addWidget(m_payloadWidget);

			const QDialog *dialog(qobject_cast<QDialog*>(m_payloadWidget));

			if (dialog)
			{
				connect(this, &ContentsDialog::accepted, dialog, &QDialog::accept);
				connect(this, &ContentsDialog::rejected, dialog, &QDialog::reject);
				connect(dialog, &QDialog::finished, dialog, [&](int result)
				{
					m_isAccepted = (result == QDialog::Accepted);

					close();
				});
			}
		}

		scrollWidget->setLayout(scrollLayout);
		scrollWidget->adjustSize();

		m_contentsLayout->addWidget(m_scrollArea);
	}

	if (buttons != QDialogButtonBox::NoButton)
	{
		m_buttonBox = new QDialogButtonBox(buttons, Qt::Horizontal, this);

		m_contentsLayout->addWidget(m_buttonBox);

		connect(m_buttonBox, &QDialogButtonBox::clicked, this, [&](QAbstractButton *button)
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
		});
	}

	setContextMenuPolicy(Qt::PreventContextMenu);
	setFocusPolicy(Qt::StrongFocus);
	setWindowFlags(Qt::Widget);
	setWindowTitle(title);
	setLayout(mainLayout);
	installEventFilter(this);
}

void ContentsDialog::showEvent(QShowEvent *event)
{
	if (m_scrollArea)
	{
		m_scrollArea->setMaximumHeight(6 + style()->pixelMetric(QStyle::PM_ScrollBarExtent) + (m_payloadWidget ? (m_payloadWidget->sizeHint().height() + 3) : 0) + (m_detailsLabel ? (m_detailsLabel->sizeHint().height() + 3) : 0));
	}

	updateSize();

	QFrame::showEvent(event);
}

void ContentsDialog::closeEvent(QCloseEvent *event)
{
	const bool isChecked(getCheckBoxState());

	if (m_isAccepted)
	{
		emit accepted(isChecked);
	}
	else
	{
		emit rejected(isChecked);
	}

	emit finished((m_isAccepted ? QDialog::Accepted : QDialog::Rejected), isChecked);

	event->accept();
}

void ContentsDialog::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Escape)
	{
		m_isAccepted = false;

		close();
	}
	else if (!event->modifiers())
	{
		switch (event->key())
		{
			case Qt::Key_Enter:
			case Qt::Key_Return:
				{
					const QList<QPushButton*> buttons(findChildren<QPushButton*>());

					for (int i = 0; i < buttons.count(); ++i)
					{
						QPushButton *button(buttons.at(i));

						if (button->isDefault())
						{
							button->click();

							return;
						}
					}
				}

				break;
			default:
				break;
		}
	}

	event->ignore();
}

void ContentsDialog::updateSize()
{
	m_headerWidget->setMaximumWidth(0);

	adjustSize();
	resize(sizeHint());

	m_headerWidget->setMaximumWidth(QWIDGETSIZE_MAX);

	if (parentWidget())
	{
		if (height() >= parentWidget()->height())
		{
			resize(width(), qMax(100, parentWidget()->height() - 20));
		}

		if (width() >= parentWidget()->width())
		{
			resize(qMax(100, parentWidget()->width() - 20), height());
		}
	}
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

	m_headerWidget->setMaximumWidth(0);

	adjustSize();

	m_headerWidget->setMaximumWidth(QWIDGETSIZE_MAX);
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
			m_offset = static_cast<QMouseEvent*>(event)->pos();
		}
		else if (event->type() == QEvent::MouseMove)
		{
			move(mapToParent(static_cast<QMouseEvent*>(event)->pos()) - m_offset);
		}
	}
	else if (object == m_closeLabel && event->type() == QEvent::MouseButtonPress && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton)
	{
		close();
	}
	else if (event->type() == QEvent::KeyPress)
	{
		switch (static_cast<QKeyEvent*>(event)->key())
		{
			case Qt::Key_Enter:
			case Qt::Key_Return:
				m_isAccepted = true;

				close();

				break;
			case Qt::Key_Escape:
				m_isAccepted = false;

				close();

				break;
			default:
				break;
		}
	}

	return QWidget::eventFilter(object, event);
}

}
