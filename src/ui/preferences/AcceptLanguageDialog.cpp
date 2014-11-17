/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#include "AcceptLanguageDialog.h"
#include "../../core/SettingsManager.h"
#include "../../core/Utils.h"

#include <QtCore/QList>
#include <QtCore/QLocale>
#include <QtGui/QKeyEvent>

#include "ui_AcceptLanguageDialog.h"

namespace Otter
{

AcceptLanguageDialog::AcceptLanguageDialog(QWidget *parent) : QDialog(parent),
	m_ui(new Ui::AcceptLanguageDialog)
{
	m_ui->setupUi(this);

	QStringList labels;
	labels << tr("Name") << tr("Code");

	m_model = new QStandardItemModel(this);
	m_model->setHorizontalHeaderLabels(labels);

	m_ui->chosenLanguagesViewWidget->setModel(m_model);
	m_ui->chosenLanguagesViewWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

	QStringList chosenLanguages = SettingsManager::getValue("Network/AcceptLanguage").toString().split(QLatin1Char(','), QString::SkipEmptyParts);

	for (int i = 0; i < chosenLanguages.count(); ++i)
	{
		chooseLanguage(chosenLanguages.at(i).section(QLatin1Char(';'), 0, 0));
	}

	const QList<QLocale> allLocales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);
	QList<QPair<QString, QString> > allLanguages;

	for (int i = 0; i < allLocales.count(); ++i)
	{
		const QLocale locale = allLocales.at(i);

		if (locale != QLocale::c())
		{
			allLanguages << QPair<QString, QString>(QStringLiteral("%1 - %2 [%3]").arg(locale.nativeLanguageName(), locale.nativeCountryName(), locale.bcp47Name()), locale.bcp47Name());
		}
	}

	qSort(allLanguages.begin(), allLanguages.end(), languageOrder);

	allLanguages.prepend(QPair<QString, QString>(tr("Any other"), QLatin1String("*")));
	allLanguages.prepend(QPair<QString, QString>(tr("System language (%1 - %2)").arg(QLocale::system().nativeLanguageName()).arg(QLocale::system().nativeCountryName()), QString("system")));

	for (int i = 0; i < allLanguages.count(); ++i)
	{
		m_ui->languagesComboBox->addItem(allLanguages.at(i).first, allLanguages.at(i).second);
	}

	m_ui->moveDownButton->setIcon(Utils::getIcon(QLatin1String("arrow-down")));
	m_ui->moveUpButton->setIcon(Utils::getIcon(QLatin1String("arrow-up")));
	m_ui->languagesComboBox->installEventFilter(this);

	connect(m_ui->moveDownButton, SIGNAL(clicked()), m_ui->chosenLanguagesViewWidget, SLOT(moveDownRow()));
	connect(m_ui->moveUpButton, SIGNAL(clicked()), m_ui->chosenLanguagesViewWidget, SLOT(moveUpRow()));
	connect(m_ui->removeButton, SIGNAL(clicked()), m_ui->chosenLanguagesViewWidget, SLOT(removeRow()));
	connect(m_ui->chooseLanguageButton, SIGNAL(clicked()), this, SLOT(languageFromComboBox()));
}

AcceptLanguageDialog::~AcceptLanguageDialog()
{
	delete m_ui;
}

void AcceptLanguageDialog::languageFromComboBox()
{
	const int index = m_ui->languagesComboBox->currentIndex();

	if (m_ui->languagesComboBox->currentText() == m_ui->languagesComboBox->itemText(index))
	{
		chooseLanguage(m_ui->languagesComboBox->currentData().toString());
	}
	else
	{
		chooseLanguage(m_ui->languagesComboBox->currentText());
	}
}

void AcceptLanguageDialog::chooseLanguage(const QString &languageCode)
{
	QString languageName;

	if (languageCode == QLatin1String("*"))
	{
		languageName = tr("Any other");
	}
	else if (languageCode == QLatin1String("system"))
	{
		languageName = tr("System language (%1 - %2)").arg(QLocale::system().nativeLanguageName()).arg(QLocale::system().nativeCountryName());
	}
	else
	{
		const QLocale locale(languageCode);

		if (locale == QLocale::c())
		{
			languageName = tr("Custom");
		}
		else
		{
			languageName = locale.nativeLanguageName() + QLatin1String(" - ") + locale.nativeCountryName();
		}
	}

	QList<QStandardItem*> items;
	items.append(new QStandardItem(languageName));
	items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
	items[0]->setData(languageCode);
	items.append(new QStandardItem((languageCode == QLatin1String("system")) ? QLocale::system().bcp47Name() : languageCode));
	items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

	m_model->appendRow(items);
}

QString AcceptLanguageDialog::getLanguageList()
{
	QString result;
	double step = 0.1;

	if (m_model->rowCount() > 100)
	{
		step = 0.001;
	}
	else if (m_model->rowCount() > 10)
	{
		step = 0.01;
	}

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		if (m_model->item(i))
		{
			if (result.isEmpty())
			{
				result += m_model->item(i)->data().toString();
			}
			else
			{
				result += QStringLiteral(",%1;q=%2").arg(m_model->item(i)->data().toString()).arg(qMax(1 - (i * step), 0.001));
			}
		}
	}

	return result;
}

bool AcceptLanguageDialog::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_ui->languagesComboBox && event->type() == QEvent::KeyPress && (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Enter || static_cast<QKeyEvent*>(event)->key() == Qt::Key_Return))
	{
		languageFromComboBox();

		return true;
	}

	return false;
}

bool AcceptLanguageDialog::languageOrder(const QPair<QString, QString> &first, const QPair<QString, QString> &second)
{
    return (first.first.toLower() < second.first.toLower());
}

}
