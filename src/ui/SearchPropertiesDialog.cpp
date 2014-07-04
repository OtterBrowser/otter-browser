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

#include "SearchPropertiesDialog.h"

#include "ui_SearchPropertiesDialog.h"

#include <QtCore/QUrlQuery>
#include <QtGui/QContextMenuEvent>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMenu>

namespace Otter
{

SearchPropertiesDialog::SearchPropertiesDialog(const QVariantHash &engineData, const QStringList &keywords, QWidget *parent) : QDialog(parent),
	m_currentLineEdit(NULL),
	m_engineData(engineData),
	m_ui(new Ui::SearchPropertiesDialog)
{
	m_ui->setupUi(this);
	m_ui->iconButton->setIcon(engineData[QLatin1String("icon")].value<QIcon>());
	m_ui->titleLineEdit->setText(engineData[QLatin1String("title")].toString());
	m_ui->descriptionLineEdit->setText(engineData[QLatin1String("description")].toString());
	m_ui->keywordLineEdit->setText(engineData[QLatin1String("keyword")].toString());
	m_ui->keywordLineEdit->setValidator(new QRegularExpressionValidator(QRegularExpression((keywords.isEmpty() ? QString() : QStringLiteral("(?!\\b(%1)\\b)").arg(keywords.join('|'))) + "[a-z0-9]*"), m_ui->keywordLineEdit));
	m_ui->defaultSearchCheckBox->setChecked(engineData[QLatin1String("isDefault")].toBool());

	connect(m_ui->resultsPostMethodCheckBox, SIGNAL(toggled(bool)), m_ui->resultsPostWidget, SLOT(setEnabled(bool)));
	connect(m_ui->suggestionsPostMethodCheckBox, SIGNAL(toggled(bool)), m_ui->suggestionsPostWidget, SLOT(setEnabled(bool)));

	m_ui->resultsAddressLineEdit->setText(engineData[QLatin1String("resultsUrl")].toString());
	m_ui->resultsAddressLineEdit->installEventFilter(this);
	m_ui->resultsQueryLineEdit->setText(engineData[QLatin1String("resultsParameters")].toString());
	m_ui->resultsQueryLineEdit->installEventFilter(this);
	m_ui->resultsPostMethodCheckBox->setChecked(engineData[QLatin1String("resultsMethod")].toString() == QLatin1String("post"));
	m_ui->resultsEnctypeComboBox->setCurrentText(engineData[QLatin1String("resultsEnctype")].toString());

	m_ui->suggestionsAddressLineEdit->setText(engineData[QLatin1String("suggestionsUrl")].toString());
	m_ui->suggestionsAddressLineEdit->installEventFilter(this);
	m_ui->suggestionsQueryLineEdit->setText(engineData[QLatin1String("suggestionsParameters")].toString());
	m_ui->suggestionsQueryLineEdit->installEventFilter(this);
	m_ui->suggestionsPostMethodCheckBox->setChecked(engineData[QLatin1String("suggestionsMethod")].toString() == QLatin1String("post"));
	m_ui->suggestionsEnctypeComboBox->setCurrentText(engineData[QLatin1String("suggestionsEnctype")].toString());

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

QVariantHash SearchPropertiesDialog::getEngineData() const
{
	QVariantHash engineData = m_engineData;
	engineData[QLatin1String("icon")] = m_ui->iconButton->icon();
	engineData[QLatin1String("title")] = m_ui->titleLineEdit->text();
	engineData[QLatin1String("description")] = m_ui->descriptionLineEdit->text();
	engineData[QLatin1String("keyword")] = m_ui->keywordLineEdit->text();
	engineData[QLatin1String("isDefault")] = m_ui->defaultSearchCheckBox->isChecked();
	engineData[QLatin1String("resultsUrl")] = m_ui->resultsAddressLineEdit->text();
	engineData[QLatin1String("resultsParameters")] = m_ui->resultsQueryLineEdit->text();
	engineData[QLatin1String("resultsMethod")] = (m_ui->resultsPostMethodCheckBox->isChecked() ? QLatin1String("post") : QLatin1String("get"));
	engineData[QLatin1String("resultsEnctype")] = m_ui->resultsEnctypeComboBox->currentText();
	engineData[QLatin1String("suggestionsUrl")] = m_ui->suggestionsAddressLineEdit->text();
	engineData[QLatin1String("suggestionsParameters")] = m_ui->suggestionsQueryLineEdit->text();
	engineData[QLatin1String("suggestionsMethod")] = (m_ui->suggestionsPostMethodCheckBox->isChecked() ? QLatin1String("post") : QLatin1String("get"));
	engineData[QLatin1String("suggestionsEnctype")] = m_ui->suggestionsEnctypeComboBox->currentText();

	return engineData;
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
