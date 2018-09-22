/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Piotr Wójcik <chocimier@tlen.pl>
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

#include "LocaleDialog.h"
#include "../core/Application.h"
#include "../core/SettingsManager.h"

#include "ui_LocaleDialog.h"

#include <QtCore/QCollator>

namespace Otter
{

LocaleDialog::LocaleDialog(QWidget *parent) : Dialog(parent),
	m_ui(new Ui::LocaleDialog)
{
	m_ui->setupUi(this);

	const QList<QFileInfo> locales(QDir(Application::getLocalePath()).entryInfoList({QLatin1String("otter-browser_*.qm")}, QDir::Files, QDir::Name));
	QVector<QPair<QString, QString> > entries;

	for (int i = 0; i < locales.count(); ++i)
	{
		const QString name(locales.at(i).baseName().remove(QLatin1String("otter-browser_")));
		const QLocale locale(Utils::createLocale(name));

		if (locale.nativeCountryName().isEmpty() || locale.nativeLanguageName().isEmpty())
		{
			entries.append({tr("Unknown [%1]").arg(name), name});
		}
		else
		{
			entries.append({QStringLiteral("%1 - %2 [%3]").arg(locale.nativeLanguageName()).arg(locale.nativeCountryName()).arg(locale.name()), locale.name()});
		}
	}

	QCollator collator;
	collator.setCaseSensitivity(Qt::CaseInsensitive);

	std::sort(entries.begin(), entries.end(), [&](const QPair<QString, QString> &first, const QPair<QString, QString> &second)
	{
		return (collator.compare(first.first, second.first) < 0);
	});

	for (int i = 0; i < entries.count(); ++i)
	{
		m_ui->languageComboBox->addItem(entries.at(i).first, entries.at(i).second);
	}

	const QString currentLocale(SettingsManager::getOption(SettingsManager::Browser_LocaleOption).toString());

	m_ui->languageComboBox->setCurrentIndex((currentLocale.endsWith(QLatin1String(".qm"))) ? 1 : qMax(0, m_ui->languageComboBox->findData(currentLocale)));
	m_ui->customFilePathWidget->setEnabled(m_ui->languageComboBox->currentIndex() == 1);
	m_ui->customFilePathWidget->setFilters({tr("Translation files (*.qm)")});

	if (m_ui->languageComboBox->currentIndex() == 1)
	{
		m_ui->customFilePathWidget->setPath(currentLocale);
	}

	connect(this, &LocaleDialog::accepted, this, &LocaleDialog::save);
	connect(m_ui->languageComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &LocaleDialog::handleCurrentIndexChanged);
}

LocaleDialog::~LocaleDialog()
{
	delete m_ui;
}

void LocaleDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void LocaleDialog::handleCurrentIndexChanged(int index)
{
	m_ui->customFilePathWidget->setEnabled(index == 1);
}

void LocaleDialog::save()
{
	QString locale;

	if (m_ui->languageComboBox->currentIndex() == 0)
	{
		locale = QLatin1String("system");
	}
	else if (m_ui->languageComboBox->currentIndex() == 1)
	{
		locale = m_ui->customFilePathWidget->getPath();
	}
	else
	{
		locale = m_ui->languageComboBox->currentData(Qt::UserRole).toString();
	}

	SettingsManager::setOption(SettingsManager::Browser_LocaleOption, locale);
}

}
