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

#include "SearchPropertiesDialog.h"
#include "../core/SearchesManager.h"

#include "ui_SearchPropertiesDialog.h"

#include <QtCore/QUrlQuery>
#include <QtGui/QContextMenuEvent>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMenu>

namespace Otter
{

SearchPropertiesDialog::SearchPropertiesDialog(const SearchInformation &engine, const QStringList &keywords, bool isDefault, QWidget *parent) : QDialog(parent),
	m_currentLineEdit(NULL),
	m_identifier(engine.identifier),
	m_keywords(keywords),
	m_ui(new Ui::SearchPropertiesDialog)
{
	m_ui->setupUi(this);
	m_ui->iconButton->setIcon(engine.icon);
	m_ui->titleLineEdit->setText(engine.title);
	m_ui->descriptionLineEdit->setText(engine.description);
	m_ui->keywordLineEdit->setText(engine.keyword);
	m_ui->keywordLineEdit->setValidator(new QRegularExpressionValidator(QRegularExpression((keywords.isEmpty() ? QString() : QStringLiteral("(?!\\b(%1)\\b)").arg(keywords.join('|'))) + "[a-z0-9]*"), m_ui->keywordLineEdit));
	m_ui->encodingLineEdit->setText(engine.encoding);
	m_ui->defaultSearchCheckBox->setChecked(isDefault);

	connect(m_ui->resultsPostMethodCheckBox, SIGNAL(toggled(bool)), m_ui->resultsPostWidget, SLOT(setEnabled(bool)));
	connect(m_ui->suggestionsPostMethodCheckBox, SIGNAL(toggled(bool)), m_ui->suggestionsPostWidget, SLOT(setEnabled(bool)));

	m_ui->resultsAddressLineEdit->setText(engine.resultsUrl.url);
	m_ui->resultsAddressLineEdit->installEventFilter(this);
	m_ui->resultsQueryLineEdit->setText(engine.resultsUrl.parameters.toString());
	m_ui->resultsQueryLineEdit->installEventFilter(this);
	m_ui->resultsPostMethodCheckBox->setChecked(engine.resultsUrl.method == QLatin1String("post"));
	m_ui->resultsEnctypeComboBox->setCurrentText(engine.resultsUrl.enctype);

	m_ui->suggestionsAddressLineEdit->setText(engine.suggestionsUrl.url);
	m_ui->suggestionsAddressLineEdit->installEventFilter(this);
	m_ui->suggestionsQueryLineEdit->setText(engine.suggestionsUrl.parameters.toString());
	m_ui->suggestionsQueryLineEdit->installEventFilter(this);
	m_ui->suggestionsPostMethodCheckBox->setChecked(engine.suggestionsUrl.method == QLatin1String("post"));
	m_ui->suggestionsEnctypeComboBox->setCurrentText(engine.suggestionsUrl.enctype);

	connect(m_ui->iconButton, SIGNAL(clicked()), this, SLOT(selectIcon()));
}

SearchPropertiesDialog::~SearchPropertiesDialog()
{
	delete m_ui;
}

void SearchPropertiesDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			m_ui->retranslateUi(this);

			break;
		default:
			break;
	}
}

void SearchPropertiesDialog::insertPlaceholder(QAction *action)
{
	if (m_currentLineEdit && !action->data().toString().isEmpty())
	{
		m_currentLineEdit->insert(QStringLiteral("{%1}").arg(action->data().toString()));
	}
}

void SearchPropertiesDialog::selectIcon()
{
	const QString path = QFileDialog::getOpenFileName(this, tr("Select Icon"), QString(), tr("Images (*.png *.jpg *.bmp *.gif *.ico)"));

	if (!path.isEmpty())
	{
		m_ui->iconButton->setIcon(QIcon(QPixmap(path)));
	}
}

SearchInformation SearchPropertiesDialog::getSearchEngine() const
{
	const QString keyword = m_ui->keywordLineEdit->text().trimmed();
	SearchInformation engine;
	engine.identifier = m_identifier;
	engine.title = m_ui->titleLineEdit->text();
	engine.description = m_ui->descriptionLineEdit->text();
	engine.keyword = (m_keywords.contains(keyword) ? keyword : QString());
	engine.encoding = m_ui->encodingLineEdit->text();
	engine.icon = m_ui->iconButton->icon();
	engine.resultsUrl.url = m_ui->resultsAddressLineEdit->text();
	engine.resultsUrl.enctype = m_ui->resultsEnctypeComboBox->currentText();
	engine.resultsUrl.method = (m_ui->resultsPostMethodCheckBox->isChecked() ? QLatin1String("post") : QLatin1String("get"));
	engine.resultsUrl.parameters = QUrlQuery(m_ui->resultsQueryLineEdit->text());
	engine.suggestionsUrl.url = m_ui->suggestionsAddressLineEdit->text();
	engine.suggestionsUrl.enctype = m_ui->suggestionsEnctypeComboBox->currentText();
	engine.suggestionsUrl.method = (m_ui->suggestionsPostMethodCheckBox->isChecked() ? QLatin1String("post") : QLatin1String("get"));
	engine.suggestionsUrl.parameters = QUrlQuery(m_ui->suggestionsQueryLineEdit->text());

	return engine;
}

bool SearchPropertiesDialog::isDefault() const
{
	return m_ui->defaultSearchCheckBox->isChecked();
}

bool SearchPropertiesDialog::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::ContextMenu)
	{
		QLineEdit *lineEdit = qobject_cast<QLineEdit*>(object);

		if (lineEdit)
		{
			m_currentLineEdit = lineEdit;

			QMenu *contextMenu = lineEdit->createStandardContextMenu();
			contextMenu->addSeparator();

			QMenu *placeholdersMenu = contextMenu->addMenu(tr("Placeholders"));
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
