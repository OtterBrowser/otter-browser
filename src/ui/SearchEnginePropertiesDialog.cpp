/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "SearchEnginePropertiesDialog.h"
#include "../core/ThemesManager.h"

#include "ui_SearchEnginePropertiesDialog.h"

#include <QtCore/QUrlQuery>
#include <QtGui/QContextMenuEvent>
#include <QtWidgets/QMenu>

namespace Otter
{

SearchEnginePropertiesDialog::SearchEnginePropertiesDialog(const SearchEnginesManager::SearchEngineDefinition &searchEngine, const QStringList &keywords, bool isDefault, QWidget *parent) : Dialog(parent),
	m_currentLineEdit(NULL),
	m_identifier(searchEngine.identifier),
	m_keywords(keywords),
	m_ui(new Ui::SearchEnginePropertiesDialog)
{
	m_ui->setupUi(this);
	m_ui->iconButton->setIcon(searchEngine.icon);
	m_ui->iconButton->setPlaceholderIcon(ThemesManager::getIcon(QLatin1String("edit-find")));
	m_ui->titleLineEdit->setText(searchEngine.title);
	m_ui->descriptionLineEdit->setText(searchEngine.description);
	m_ui->keywordLineEdit->setText(searchEngine.keyword);
	m_ui->keywordLineEdit->setValidator(new QRegularExpressionValidator(QRegularExpression((keywords.isEmpty() ? QString() : QStringLiteral("(?!\\b(%1)\\b)").arg(keywords.join('|'))) + "[a-z0-9]*"), m_ui->keywordLineEdit));
	m_ui->encodingLineEdit->setText(searchEngine.encoding);
	m_ui->formAddressLineEdit->setText(searchEngine.formUrl.toString());
	m_ui->updateAddressLineEdit->setText(searchEngine.selfUrl.toString());
	m_ui->defaultSearchEngineCheckBox->setChecked(isDefault);

	connect(m_ui->resultsPostMethodCheckBox, SIGNAL(toggled(bool)), m_ui->resultsPostWidget, SLOT(setEnabled(bool)));
	connect(m_ui->suggestionsPostMethodCheckBox, SIGNAL(toggled(bool)), m_ui->suggestionsPostWidget, SLOT(setEnabled(bool)));

	m_ui->resultsAddressLineEdit->setText(searchEngine.resultsUrl.url);
	m_ui->resultsAddressLineEdit->installEventFilter(this);
	m_ui->resultsQueryLineEdit->setText(searchEngine.resultsUrl.parameters.toString(QUrl::FullyDecoded));
	m_ui->resultsQueryLineEdit->installEventFilter(this);
	m_ui->resultsPostMethodCheckBox->setChecked(searchEngine.resultsUrl.method == QLatin1String("post"));
	m_ui->resultsEnctypeComboBox->setCurrentText(searchEngine.resultsUrl.enctype);

	m_ui->suggestionsAddressLineEdit->setText(searchEngine.suggestionsUrl.url);
	m_ui->suggestionsAddressLineEdit->installEventFilter(this);
	m_ui->suggestionsQueryLineEdit->setText(searchEngine.suggestionsUrl.parameters.toString(QUrl::FullyDecoded));
	m_ui->suggestionsQueryLineEdit->installEventFilter(this);
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
	if (m_currentLineEdit && !action->data().toString().isEmpty())
	{
		m_currentLineEdit->insert(QStringLiteral("{%1}").arg(action->data().toString()));
	}
}

SearchEnginesManager::SearchEngineDefinition SearchEnginePropertiesDialog::getSearchEngine() const
{
	const QString keyword(m_ui->keywordLineEdit->text().trimmed());
	SearchEnginesManager::SearchEngineDefinition searchEngine;
	searchEngine.identifier = m_identifier;
	searchEngine.title = m_ui->titleLineEdit->text();
	searchEngine.description = m_ui->descriptionLineEdit->text();
	searchEngine.keyword = (m_keywords.contains(keyword) ? QString() : keyword);
	searchEngine.encoding = m_ui->encodingLineEdit->text();
	searchEngine.formUrl = QUrl(m_ui->formAddressLineEdit->text());
	searchEngine.selfUrl = QUrl(m_ui->updateAddressLineEdit->text());
	searchEngine.icon = m_ui->iconButton->icon();
	searchEngine.resultsUrl.url = m_ui->resultsAddressLineEdit->text();
	searchEngine.resultsUrl.enctype = m_ui->resultsEnctypeComboBox->currentText();
	searchEngine.resultsUrl.method = (m_ui->resultsPostMethodCheckBox->isChecked() ? QLatin1String("post") : QLatin1String("get"));
	searchEngine.resultsUrl.parameters = QUrlQuery(m_ui->resultsQueryLineEdit->text());
	searchEngine.suggestionsUrl.url = m_ui->suggestionsAddressLineEdit->text();
	searchEngine.suggestionsUrl.enctype = m_ui->suggestionsEnctypeComboBox->currentText();
	searchEngine.suggestionsUrl.method = (m_ui->suggestionsPostMethodCheckBox->isChecked() ? QLatin1String("post") : QLatin1String("get"));
	searchEngine.suggestionsUrl.parameters = QUrlQuery(m_ui->suggestionsQueryLineEdit->text());

	return searchEngine;
}

bool SearchEnginePropertiesDialog::isDefault() const
{
	return m_ui->defaultSearchEngineCheckBox->isChecked();
}

bool SearchEnginePropertiesDialog::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::ContextMenu)
	{
		QLineEdit *lineEdit(qobject_cast<QLineEdit*>(object));

		if (lineEdit)
		{
			m_currentLineEdit = lineEdit;

			QMenu *contextMenu(lineEdit->createStandardContextMenu());
			contextMenu->addSeparator();

			QMenu *placeholdersMenu(contextMenu->addMenu(tr("Placeholders")));
			placeholdersMenu->addAction(tr("Search Terms"))->setData(QLatin1String("searchTerms"));
			placeholdersMenu->addAction(tr("Language"))->setData(QLatin1String("language"));

			connect(placeholdersMenu, SIGNAL(triggered(QAction*)), this, SLOT(insertPlaceholder(QAction*)));

			contextMenu->exec(static_cast<QContextMenuEvent*>(event)->globalPos());
			contextMenu->deleteLater();

			m_currentLineEdit = NULL;

			return true;
		}
	}

	return QDialog::eventFilter(object, event);
}

}
