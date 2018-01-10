/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "Updater.h"
#include "Application.h"
#include "Console.h"
#include "PlatformIntegration.h"
#include "TransfersManager.h"

#include <QtCore/QDir>
#include <QtCore/QtMath>
#include <QtCore/QStandardPaths>
#include <QtXmlPatterns/QXmlSchema>
#include <QtXmlPatterns/QXmlSchemaValidator>

namespace Otter
{

Updater::Updater(const UpdateChecker::UpdateInformation &information, QObject *parent) : QObject(parent),
	m_transfer(nullptr),
	m_transfersCount(0),
	m_transfersSuccessful(true)
{
	const QString path(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QLatin1String("/OtterBrowser/"));
	QDir directory(path);

	if (!directory.exists())
	{
		QDir().mkdir(path);
	}
	else if (directory.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries).count() > 0)
	{
		directory.setNameFilters({QLatin1String("*.*")});
		directory.setFilter(QDir::Files);

		for (int i = 0; i < directory.entryList().count(); ++i)
		{
			directory.remove(directory.entryList().at(i));
		}
	}

	clearUpdate();

	downloadFile(information.scriptUrl, path);

	m_transfer = downloadFile(information.fileUrl, path);
	m_transfer->setUpdateInterval(500);

	connect(m_transfer, &Transfer::progressChanged, this, &Updater::updateProgress);
}

void Updater::handleTransferFinished()
{
	Transfer *transfer(qobject_cast<Transfer*>(sender()));

	if (transfer)
	{
		const QString path(transfer->getTarget());

		if ((transfer->getState() == Transfer::FinishedState) && QFile::exists(path))
		{
			if (QFileInfo(path).suffix() == QLatin1String("xml"))
			{
				QXmlSchema schema;

				if (!schema.load(QUrl::fromLocalFile(SessionsManager::getReadableDataPath(QLatin1String("schemas/update.xsd")))) || !QXmlSchemaValidator(schema).validate(QUrl::fromLocalFile(path)))
				{
					Console::addMessage(QCoreApplication::translate("main", "Downloaded update script is not valid: %1").arg(path), Console::OtherCategory, Console::ErrorLevel);

					m_transfersSuccessful = false;
				}
				else
				{
					QFile file(SessionsManager::getWritableDataPath(QLatin1String("update.txt")));

					if (file.open(QIODevice::ReadWrite | QIODevice::Text))
					{
						QTextStream stream(&file);
						stream << path;

						file.close();
					}
				}
			}
		}
		else
		{
			Console::addMessage(QCoreApplication::translate("main", "Unable to download update: %1").arg(transfer->getSource().url()), Console::OtherCategory, Console::ErrorLevel);

			m_transfersSuccessful = false;
		}

		transfer->deleteLater();
	}
	else
	{
		m_transfersSuccessful = false;
	}

	++m_transfersCount;

	if (m_transfersCount == 2)
	{
		if (!m_transfersSuccessful)
		{
			clearUpdate();
		}

		emit finished(m_transfersSuccessful);

		deleteLater();
	}
}

void Updater::updateProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	emit progress(qFloor(Utils::calculatePercent(bytesReceived, bytesTotal)));
}

void Updater::clearUpdate()
{
	QFile::remove(SessionsManager::getWritableDataPath(QLatin1String("update.txt")));
}

Transfer* Updater::downloadFile(const QUrl &url, const QString &path)
{
	const QString urlString(url.path());
	Transfer *transfer(new Transfer(url, path + urlString.mid(urlString.lastIndexOf(QLatin1Char('/')) + 1), (Transfer::CanOverwriteOption), this));

	connect(transfer, &Transfer::finished, this, &Updater::handleTransferFinished);

	return transfer;
}

QString Updater::getScriptPath()
{
	QFile file(SessionsManager::getWritableDataPath(QLatin1String("update.txt")));

	if (file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QTextStream content(&file);

		if (!content.atEnd())
		{
			return content.readLine();
		}
	}

	return {};
}

bool Updater::installUpdate()
{
	const PlatformIntegration *integration(Application::getPlatformIntegration());

	if (integration)
	{
		return integration->installUpdate();
	}

	return false;
}

bool Updater::isReadyToInstall(QString path)
{
	if (path.isEmpty())
	{
		path = getScriptPath();
	}

	return QFileInfo::exists(path);
}

}
