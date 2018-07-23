/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "PlatformIntegration.h"
#include "Application.h"
#include "Console.h"
#include "Updater.h"

#include <QtCore/QDir>
#include <QtCore/QMimeData>
#include <QtCore/QProcess>
#include <QtGui/QDrag>
#include <QtWidgets/QMessageBox>

namespace Otter
{

PlatformIntegration::PlatformIntegration(QObject *parent) : QObject(parent)
{
}


void PlatformIntegration::startLinkDrag(const QUrl &url, const QString &title, const QPixmap &pixmap, QObject *parent) const
{
	QDrag *drag(new QDrag(parent));
	QMimeData *mimeData(new QMimeData());
	mimeData->setText(url.toString());
	mimeData->setUrls({url});

	if (!title.isEmpty())
	{
		mimeData->setProperty("x-url-title", title);
	}

	mimeData->setProperty("x-url-string", url.toString());

	drag->setMimeData(mimeData);
	drag->setPixmap(pixmap);
	drag->exec(Qt::CopyAction);
}

QString PlatformIntegration::getPreferredPasswordsBackend() const
{
	return QLatin1String("file");
}

QString PlatformIntegration::getUpdaterBinary() const
{
	return QLatin1String("updater");
}

QString PlatformIntegration::getPlatformName() const
{
	return {};
}

bool PlatformIntegration::canShowNotifications() const
{
	return false;
}

bool PlatformIntegration::setAsDefaultBrowser()
{
	return false;
}

bool PlatformIntegration::canSetAsDefaultBrowser() const
{
	return false;
}

bool PlatformIntegration::isDefaultBrowser() const
{
	return false;
}

void PlatformIntegration::showNotification(Notification *notification)
{
	Q_UNUSED(notification)
}

bool PlatformIntegration::installUpdate() const
{
	const QString updaterPath(QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + QDir::separator() + getUpdaterBinary()));
	const QString scriptPath(Updater::getScriptPath());

	if (Updater::isReadyToInstall(scriptPath) && QFileInfo(updaterPath).isExecutable())
	{
		if (QProcess::startDetached(updaterPath, {QLatin1String("--install-dir"), QCoreApplication::applicationDirPath(), QLatin1String("--package-dir"), QFileInfo(scriptPath).absolutePath(), QLatin1String("--script"), scriptPath}))
		{
			Updater::clearUpdate();

			Application::getInstance()->close();

			return true;
		}
	}

	Console::addMessage(QCoreApplication::translate("main", "Failed to install update\nUpdater: %1\nScript: %2").arg(updaterPath).arg(scriptPath), Console::OtherCategory, Console::ErrorLevel);

	QMessageBox::critical(nullptr, tr("Error"), tr("Failed to install update."), QMessageBox::Close);

	Updater::clearUpdate();

	return false;
}

}
