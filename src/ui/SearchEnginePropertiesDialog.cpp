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

#include "SearchEnginePropertiesDialog.h"

#include "ui_SearchEnginePropertiesDialog.h"

#include <QtCore/QUrlQuery>
#include <QtGui/QContextMenuEvent>
#include <QtWidgets/QMenu>

namespace Otter
{

SearchEnginePropertiesDialog::SearchEnginePropertiesDialog(const SearchEnginesManager::SearchEngineDefinition &searchEngine, const QStringList &keywords, QWidget *parent) : Dialog(parent),
	m_currentLineEditWidget(nullptr),
	m_identifier(searchEngine.identifier),
	m_keywords(keywords),
	m_ui(new Ui::SearchEnginePropertiesDialog)
{
	m_ui->setupUi(this);
	m_ui->iconButton->setIcon(searchEngine.icon);
	m_ui->iconButton->setDefaultIcon(QLatin1String("edit-find"));
	m_ui->titleLineEditWidget->setText(searchEngine.title);
	m_ui->descriptionLineEditWidget->setText(searchEngine.description);
	m_ui->keywordLineEditWidget->setText(searchEngine.keyword);
	m_ui->keywordLineEditWidget->setValidator(new QRegularExpressionValidator(QRegularExpression((keywords.isEmpty() ? QString() : QStringLiteral("(?!\\b(%1)\\b)").arg(keywords.join('|'))) + "[a-z0-9]*"), m_ui->keywordLineEditWidget));
	m_ui->encodingLineEditWidget->setText(searchEngine.encoding);
	m_ui->formAddressLineEditWidget->setText(searchEngine.formUrl.toString());
	m_ui->updateAddressLineEditWidget->setText(searchEngine.selfUrl.toString());

	connect(m_ui->resultsPostMethodCheckBox, &QCheckBox::toggled, m_ui->resultsPostWidget, &QWidget::setEnabled);
	connect(m_ui->suggestionsPostMethodCheckBox, &QCheckBox::toggled, m_ui->suggestionsPostWidget, &QWidget::setEnabled);

	m_ui->resultsAddressLineEditWidget->setText(searchEngine.resultsUrl.url);
	m_ui->resultsAddressLineEditWidget->installEventFilter(this);
	m_ui->resultsQueryLineEditWidget->setText(searchEngine.resultsUrl.parameters.toString(QUrl::FullyDecoded));
	m_ui->resultsQueryLineEditWidget->installEventFilter(this);
	m_ui->resultsPostMethodCheckBox->setChecked(searchEngine.resultsUrl.method == QLatin1String("post"));
	m_ui->resultsEnctypeComboBox->setCurrentText(searchEngine.resultsUrl.enctype);

	m_ui->suggestionsAddressLineEditWidget->setText(searchEngine.suggestionsUrl.url);
	m_ui->suggestionsAddressLineEditWidget->installEventFilter(this);
	m_ui->suggestionsQueryLineEditWidget->setText(searchEngine.suggestionsUrl.parameters.toString(QUrl::FullyDecoded));
	m_ui->suggestionsQueryLineEditWidget->installEventFilter(this);
	m_ui->suggestionsPostMethodCheckBox->setChecked(searchEngine.suggestionsUrl.method == QLatin1String("post"));
	m_ui->suggestionsEnctypeComboBox->setCurrentText(searchEngine.suggestionsUrl.enctype);
}

SearchEnginePropertiesDialog::~SearchEnginePropertiesDialog()
{
	delete m_ui;
}

void SearchEnginePropertiesDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void SearchEnginePropertiesDialog::insertPlaceholder(QAction *action)
{
	if (m_currentLineEditWidget && !action->data().toString().isEmpty())
	{
		m_currentLineEditWidget->insert(QLatin1Char('{') + action->data().toString() + QLatin1Char('}'));
	}
}

SearchEnginesManager::SearchEngineDefinition SearchEnginePropertiesDialog::getSearchEngine() const
{
	const QString keyword(m_ui->keywordLineEditWidget->text().trimmed());
	SearchEnginesManager::SearchEngineDefinition searchEngine;
	searchEngine.identifier = m_identifier;
	searchEngine.title = m_ui->titleLineEditWidget->text();
	searchEngine.description = m_ui->descriptionLineEditWidget->text();
	searchEngine.keyword = (m_keywords.contains(keyword) ? QString() : keyword);
	searchEngine.encoding = m_ui->encodingLineEditWidget->text();
	searchEngine.formUrl = QUrl(m_ui->formAddressLineEditWidget->text());
	searchEngine.selfUrl = QUrl(m_ui->updateAddressLineEditWidget->text());
	searchEngine.icon = m_ui->iconButton->icon();
	searchEngine.resultsUrl.url = m_ui->resultsAddressLineEditWidget->text();
	searchEngine.resultsUrl.enctype = m_ui->resultsEnctypeComboBox->currentText();
	searchEngine.resultsUrl.method = (m_ui->resultsPostMethodCheckBox->isChecked() ? QLatin1String("post") : QLatin1String("get"));
	searchEngine.resultsUrl.parameters = QUrlQuery(m_ui->resultsQueryLineEditWidget->text());
	searchEngine.suggestionsUrl.url = m_ui->suggestionsAddressLineEditWidget->text();
	searchEngine.suggestionsUrl.enctype = m_ui->suggestionsEnctypeComboBox->currentText();
	searchEngine.suggestionsUrl.method = (m_ui->suggestionsPostMethodCheckBox->isChecked() ? QLatin1String("post") : QLatin1String("get"));
	searchEngine.suggestionsUrl.parameters = QUrlQuery(m_ui->suggestionsQueryLineEditWidget->text());

	return searchEngine;
}

bool SearchEnginePropertiesDialog::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::ContextMenu)
	{
		LineEditWidget *lineEditWidget(qobject_cast<LineEditWidget*>(object));

		if (!lineEditWidget)
		{
			return QDialog::eventFilter(object, event);
		}

		m_currentLineEditWidget = lineEditWidget;

		QMenu *contextMenu(lineEditWidget->createStandardContextMenu());
		contextMenu->addSeparator();

		QMenu *placeholdersMenu(contextMenu->addMenu(tr("Placeholders")));
		placeholdersMenu->addAction(tr("Search Terms"))->setData(QLatin1String("searchTerms"));
		placeholdersMenu->addAction(tr("Language"))->setData(QLatin1String("language"));

		connect(placeholdersMenu, &QMenu::triggered, this, &SearchEnginePropertiesDialog::insertPlaceholder);

		contextMenu->exec(static_cast<QContextMenuEvent*>(event)->globalPos());
		contextMenu->deleteLater();

		m_currentLineEditWidget = nullptr;

		return true;
	}

	return QDialog::eventFilter(object, event);
}

}
