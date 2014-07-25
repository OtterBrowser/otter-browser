/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "ContentBlockingManager.h"
#include "Console.h"
#include "ContentBlockingList.h"
#include "SettingsManager.h"
#include "SessionsManager.h"

#include <QtCore/QDir>
#include <QtCore/QSettings>

namespace Otter
{

ContentBlockingManager* ContentBlockingManager::m_instance = NULL;
QList<ContentBlockingList*> ContentBlockingManager::m_blockingLists;
QByteArray ContentBlockingManager::m_hidingRules;
QMultiHash<QString, QString> ContentBlockingManager::m_specificDomainHidingRules;
QMultiHash<QString, QString> ContentBlockingManager::m_hidingRulesExceptions;
bool ContentBlockingManager::m_isContentBlockingEnabled = false;

ContentBlockingManager::ContentBlockingManager(QObject *parent) : QObject(parent)
{
}

void ContentBlockingManager::createInstance(QObject *parent)
{
	m_instance = new ContentBlockingManager(parent);

	loadLists();
}

void ContentBlockingManager::loadLists()
{
	const QString adBlockPath = SessionsManager::getProfilePath() + QLatin1String("/adblock/");
	const QString configPath = SessionsManager::getProfilePath() + QLatin1String("/adblock.ini");
	const QDir directory(adBlockPath);

	if (!directory.exists())
	{
		QDir().mkpath(adBlockPath);
	}

	if (!QFile::exists(configPath))
	{
		QFile::copy(QLatin1String(":/other/adblock.ini"), configPath);
		QFile::setPermissions(configPath, (QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther));
	}

	const QStringList definitions = QDir(QLatin1String(":/adblock/")).entryList(QDir::Files);

	for (int i = 0; i < definitions.count(); ++i)
	{
		const QString filePath = directory.filePath(definitions.at(i));

		if (!QFile::exists(filePath))
		{
			QFile::copy(QLatin1String(":/adblock/") + definitions.at(i), filePath);
			QFile::setPermissions(filePath, (QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther));
		}
	}

	QStringList adBlockList = directory.entryList(QStringList(QLatin1String("*.txt")), QDir::Files);
	QSettings adblock(configPath, QSettings::IniFormat);
	adblock.setIniCodec("UTF-8");

	const QStringList entries = adblock.childGroups();

	for (int i = 0; i < entries.count(); ++i)
	{
		const QString entry = entries.at(i);
		int indexOfFile = adBlockList.indexOf(adblock.value(QStringLiteral("%1/file").arg(entry)).toString());

		if (indexOfFile >= 0)
		{
			ContentBlockingList *definition = new ContentBlockingList();
			definition->setFile(adBlockPath, adBlockList.at(indexOfFile));
			definition->setListName(adblock.value(QStringLiteral("%1/title").arg(entry)).toString());
			definition->setConfigListName(entry);

			if (adblock.value(QStringLiteral("%1/enabled").arg(entry)).toBool())
			{
				connect(definition, SIGNAL(updateCustomStyleSheets()), ContentBlockingManager::getInstance(), SLOT(updateCustomStyleSheets()));

				definition->setEnabled(true);

				m_isContentBlockingEnabled = true;
			}
			else
			{
				definition->setEnabled(false);
			}

			m_blockingLists.append(definition);

			adBlockList.removeAt(indexOfFile);
		}
		else
		{
			adblock.remove(entry);
		}
	}

	for (int i = 0; i < adBlockList.count(); ++i)
	{
		const QString value = adBlockList.at(i);
		const QString parsedValue = value.endsWith(QLatin1String(".txt")) ? value.left(value.length() - 4) : value;

		adblock.setValue(QStringLiteral("%1/title").arg(parsedValue), value);
		adblock.setValue(QStringLiteral("%1/file").arg(parsedValue), value);
		adblock.setValue(QStringLiteral("%1/enabled").arg(parsedValue), false);

		ContentBlockingList* definition = new ContentBlockingList();
		definition->setFile(adBlockPath, value);
		definition->setEnabled(false);

		m_blockingLists.append(definition);
	}
}

void ContentBlockingManager::updateLists()
{
	QSettings adblock(SessionsManager::getProfilePath() + QLatin1String("/adblock.ini"), QSettings::IniFormat);
	adblock.setIniCodec("UTF-8");

	const QStringList entries = adblock.childGroups();

	m_isContentBlockingEnabled = false;

	for (int i = 0; i < entries.count(); ++i)
	{
		for (int j = 0; j < m_blockingLists.count(); ++j)
		{
			if (m_blockingLists.at(j)->getFileName() == adblock.value(QStringLiteral("%1/file").arg(entries.at(i))).toString())
			{
				if (adblock.value(QStringLiteral("%1/enabled").arg(entries.at(i))).toBool())
				{
					connect(m_blockingLists.at(j), SIGNAL(updateCustomStyleSheets()), ContentBlockingManager::getInstance(), SLOT(updateCustomStyleSheets()));

					m_blockingLists.at(j)->setEnabled(true);

					m_isContentBlockingEnabled = true;

					break;
				}
				else
				{
					disconnect(m_blockingLists.at(j), SIGNAL(updateCustomStyleSheets()), ContentBlockingManager::getInstance(), SLOT(updateCustomStyleSheets()));

					m_blockingLists.at(j)->setEnabled(false);

					break;
				}
			}
		}
	}
}

QList<ContentBlockingList*> ContentBlockingManager::getBlockingDefinitions()
{
	return m_blockingLists;
}

void ContentBlockingManager::updateCustomStyleSheets()
{
	m_hidingRules.clear();
	m_specificDomainHidingRules.clear();
	m_hidingRulesExceptions.clear();

	for (int i = 0; i < m_blockingLists.count(); ++i)
	{
		m_hidingRules.append(m_blockingLists.at(i)->getCssRules());
		m_specificDomainHidingRules += m_blockingLists.at(i)->getSpecificDomainHidingRules();
		m_hidingRulesExceptions += m_blockingLists.at(i)->getHidingRulesExceptions();
	}

	emit styleSheetsUpdated();
}

QStringList ContentBlockingManager::createSubdomainList(const QString domain)
{
	QStringList subdomainList;

	int dotPosition = domain.lastIndexOf(QLatin1Char('.'));
	dotPosition = domain.lastIndexOf(QLatin1Char('.'), dotPosition - 1);

	while (dotPosition != -1)
	{
		subdomainList.append(domain.mid(dotPosition + 1));

		dotPosition = domain.lastIndexOf(QLatin1Char('.'), dotPosition - 1);
	}

	subdomainList.append(domain);

	return subdomainList;
}

QByteArray ContentBlockingManager::getStyleSheetHidingRules()
{
	return m_hidingRules;
}

QMultiHash<QString, QString> ContentBlockingManager::getSpecificDomainHidingRules()
{
	return m_specificDomainHidingRules;
}

QMultiHash<QString, QString> ContentBlockingManager::getHidingRulesExceptions()
{
	return m_hidingRulesExceptions;
}

ContentBlockingManager* ContentBlockingManager::getInstance()
{
	return m_instance;
}

bool ContentBlockingManager::isUrlBlocked(const QNetworkRequest &request)
{
	const QString scheme = request.url().scheme();

	if (scheme != QLatin1String("http") && scheme != QLatin1String("https"))
	{
		return false;
	}

	for (int i = 0; i < m_blockingLists.count(); ++i)
	{
		if (m_blockingLists.at(i)->isEnabled())
		{
			if (m_blockingLists.at(i)->isUrlBlocked(request))
			{
				return true;
			}
		}
	}

	return false;
}

bool ContentBlockingManager::isContentBlockingEnabled()
{
	return m_isContentBlockingEnabled;
}

}
