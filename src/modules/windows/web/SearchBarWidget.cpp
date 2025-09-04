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
	m_ui->nextButton->setIcon(ThemesManager::createIcon(QLatin1String("go-down")));
	m_ui->previousButton->setIcon(ThemesManager::createIcon(QLatin1String("go-up")));
	m_ui->highlightButton->setChecked(SettingsManager::getOption(SettingsManager::Search_EnableFindInPageHighlightAllOption).toBool());
	m_ui->closeButton->setIcon(ThemesManager::createIcon(QLatin1String("window-close")));

	connect(m_ui->queryLineEditWidget, &LineEditWidget::textEdited, this, &SearchBarWidget::queryChanged);
	connect(m_ui->queryLineEditWidget, &LineEditWidget::returnPressed, this, &SearchBarWidget::notifyRequestedSearch);
	connect(m_ui->caseSensitiveButton, &QPushButton::clicked, this, &SearchBarWidget::notifyFlagsChanged);
	connect(m_ui->highlightButton, &QPushButton::clicked, this, &SearchBarWidget::notifyFlagsChanged);
	connect(m_ui->nextButton, &QPushButton::clicked, this, &SearchBarWidget::notifyRequestedSearch);
	connect(m_ui->previousButton, &QPushButton::clicked, this, [&]()
	{
		emit requestedSearch(getFlags() | WebWidget::BackwardFind);
	});
	connect(m_ui->closeButton, &QPushButton::clicked, this, &SearchBarWidget::hide);
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

		m_ui->queryLineEditWidget->clear();

		emit requestedSearch(getFlags());
	}
}

void SearchBarWidget::notifyRequestedSearch()
{
	emit requestedSearch(getFlags());
}

void SearchBarWidget::notifyFlagsChanged()
{
	const WebWidget::FindFlags flags(getFlags());

	if (flags.testFlag(WebWidget::HighlightAllFind) != SettingsManager::getOption(SettingsManager::Search_EnableFindInPageHighlightAllOption).toBool())
	{
		SettingsManager::setOption(SettingsManager::Search_EnableFindInPageHighlightAllOption, flags.testFlag(WebWidget::HighlightAllFind));
	}

	emit flagsChanged(flags);
}

void SearchBarWidget::selectAll()
{
	m_ui->queryLineEditWidget->setFocus();
	m_ui->queryLineEditWidget->selectAll();
}

void SearchBarWidget::updateResults(const QString &query, int matchesAmount, int activeResult)
{
	QString resultsText;
	QPalette palette(this->palette());
	const bool hasMatches(matchesAmount != 0);

	if (!query.isEmpty() && m_ui->queryLineEditWidget->text() == query)
	{
//TODO Ensure that text is readable
		if (hasMatches)
		{
			palette.setColor(QPalette::Base, QColor(0xCE, 0xF6, 0xDF));
		}
		else
		{
			palette.setColor(QPalette::Base, QColor(0xF1, 0xE7, 0xE4));
		}

		if (matchesAmount > 0 && activeResult > 0)
		{
			resultsText = tr("%1 of %n result(s)", "", matchesAmount).arg(activeResult);
		}
		else if (matchesAmount == 0)
		{
			resultsText = tr("Phrase not found");
		}
	}

	m_ui->resultsLabel->setText(resultsText);
	m_ui->queryLineEditWidget->setPalette(palette);
	m_ui->nextButton->setEnabled(hasMatches);
	m_ui->previousButton->setEnabled(hasMatches);
}

void SearchBarWidget::setQuery(const QString &query)
{
	m_ui->queryLineEditWidget->setText(query);
}

void SearchBarWidget::setVisible(bool visible)
{
	QWidget::setVisible(visible);

	if (!visible && parentWidget())
	{
		parentWidget()->setFocus();
	}
}

QString SearchBarWidget::getQuery() const
{
	return m_ui->queryLineEditWidget->text();
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

bool SearchBarWidget::hasQuery() const
{
	return !m_ui->queryLineEditWidget->text().isEmpty();
}

}
