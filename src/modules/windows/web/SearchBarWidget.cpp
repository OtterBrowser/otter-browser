/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "SearchBarWidget.h"
#include "../../../core/ThemesManager.h"

#include "ui_SearchBarWidget.h"

#include <QtGui/QKeyEvent>

namespace Otter
{

SearchBarWidget::SearchBarWidget(QWidget *parent) : QWidget(parent),
	m_ui(new Ui::SearchBarWidget)
{
	m_ui->setupUi(this);
	m_ui->nextButton->setIcon(ThemesManager::getIcon(QLatin1String("go-down")));
	m_ui->previousButton->setIcon(ThemesManager::getIcon(QLatin1String("go-up")));
	m_ui->closeButton->setIcon(ThemesManager::getIcon(QLatin1String("window-close")));

	connect(m_ui->queryLineEdit, SIGNAL(textEdited(QString)), this, SIGNAL(queryChanged()));
	connect(m_ui->queryLineEdit, SIGNAL(returnPressed()), this, SLOT(notifyRequestedSearch()));
	connect(m_ui->caseSensitiveButton, SIGNAL(clicked()), this, SLOT(notifyFlagsChanged()));
	connect(m_ui->highlightButton, SIGNAL(clicked()), this, SLOT(notifyFlagsChanged()));
	connect(m_ui->nextButton, SIGNAL(clicked()), this, SLOT(notifyRequestedSearch()));
	connect(m_ui->previousButton, SIGNAL(clicked()), this, SLOT(notifyRequestedSearch()));
	connect(m_ui->closeButton, SIGNAL(clicked()), this, SLOT(hide()));
}

SearchBarWidget::~SearchBarWidget()
{
	delete m_ui;
}

void SearchBarWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void SearchBarWidget::keyPressEvent(QKeyEvent *event)
{
	QWidget::keyPressEvent(event);

	if (event->key() == Qt::Key_Escape)
	{
		hide();

		m_ui->queryLineEdit->clear();

		emit requestedSearch(getFlags());
	}
}

void SearchBarWidget::notifyRequestedSearch()
{
	WebWidget::FindFlags flags(getFlags());

	if (sender() && sender()->objectName() == QLatin1String("previousButton"))
	{
		flags |= WebWidget::BackwardFind;
	}

	emit requestedSearch(flags);
}

void SearchBarWidget::notifyFlagsChanged()
{
	emit flagsChanged(getFlags());
}

void SearchBarWidget::selectAll()
{
	m_ui->queryLineEdit->setFocus();
	m_ui->queryLineEdit->selectAll();
}

void SearchBarWidget::setQuery(const QString &query)
{
	m_ui->queryLineEdit->setText(query);
}

void SearchBarWidget::setVisible(bool visible)
{
	QWidget::setVisible(visible);

	if (!visible && parentWidget())
	{
		parentWidget()->setFocus();
	}
}

void SearchBarWidget::setResultsFound(bool found)
{
	QPalette palette(this->palette());

	if (!m_ui->queryLineEdit->text().isEmpty())
	{
//TODO Ensure that text is readable
		if (found)
		{
			palette.setColor(QPalette::Base, QColor(QLatin1String("#CEF6DF")));
		}
		else
		{
			palette.setColor(QPalette::Base, QColor(QLatin1String("#F1E7E4")));
		}
	}

	m_ui->queryLineEdit->setPalette(palette);
	m_ui->nextButton->setEnabled(found);
	m_ui->previousButton->setEnabled(found);
}

QString SearchBarWidget::getQuery() const
{
	return m_ui->queryLineEdit->text();
}

WebWidget::FindFlags SearchBarWidget::getFlags() const
{
	WebWidget::FindFlags flags(WebWidget::NoFlagsFind);

	if (m_ui->highlightButton->isChecked())
	{
		flags |= WebWidget::HighlightAllFind;
	}

	if (m_ui->caseSensitiveButton->isChecked())
	{
		flags |= WebWidget::CaseSensitiveFind;
	}

	return flags;
}

}
