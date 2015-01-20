/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "WindowsPlatformIntegration.h"
#include "../../../core/Application.h"
#include "../../../core/Console.h"
#include "../../../core/Transfer.h"
#include "../../../core/TransfersManager.h"
#include "../../../ui/MainWindow.h"

#include <Windows.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QtMath>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QFileIconProvider>

namespace Otter
{

QProcessEnvironment WindowsPlatformIntegration::m_environment;

WindowsPlatformIntegration::WindowsPlatformIntegration(Application *parent) : PlatformIntegration(parent),
	m_registrationIdentifier(QLatin1String("OtterBrowser")),
	m_applicationFilePath(QDir::toNativeSeparators(QCoreApplication::applicationFilePath())),
	m_applicationRegistration(QLatin1String("HKEY_LOCAL_MACHINE\\Software\\RegisteredApplications"), QSettings::NativeFormat),
	m_propertiesRegistration(QLatin1String("HKEY_CURRENT_USER\\Software\\Classes\\") + m_registrationIdentifier, QSettings::NativeFormat),
	m_cleanupTimer(0)
{
	m_registrationPairs << qMakePair(QLatin1String("http"), ProtocolType) << qMakePair(QLatin1String("https"), ProtocolType) << qMakePair(QLatin1String("ftp"), ProtocolType)
	<< qMakePair(QLatin1String(".htm"), ExtensionType) << qMakePair(QLatin1String(".html"), ExtensionType) << qMakePair(QLatin1String(".xhtml"), ExtensionType);

	if (QSysInfo::windowsVersion() >= QSysInfo::WV_WINDOWS7)
	{
		connect(Application::getInstance(), SIGNAL(windowRemoved(MainWindow*)), this, SLOT(removeWindow(MainWindow*)));
		connect(TransfersManager::getInstance(), SIGNAL(transferChanged(Transfer*)), this , SLOT(updateTaskbarButtons()));
		connect(TransfersManager::getInstance(), SIGNAL(transferStarted(Transfer*)), this , SLOT(updateTaskbarButtons()));
		connect(TransfersManager::getInstance(), SIGNAL(transferFinished(Transfer*)), this , SLOT(updateTaskbarButtons()));
		connect(TransfersManager::getInstance(), SIGNAL(transferRemoved(Transfer*)), this , SLOT(updateTaskbarButtons()));
		connect(TransfersManager::getInstance(), SIGNAL(transferStopped(Transfer*)), this , SLOT(updateTaskbarButtons()));
	}
}

void WindowsPlatformIntegration::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_cleanupTimer)
	{
		killTimer(m_cleanupTimer);

		m_cleanupTimer = 0;
		m_environment.clear();
	}
}

void WindowsPlatformIntegration::removeWindow(MainWindow *window)
{
	if (m_taskbarButtons.contains(window))
	{
		m_taskbarButtons.remove(window);
	}
}

void WindowsPlatformIntegration::updateTaskbarButtons()
{
	const QVector<Transfer*> transfers = TransfersManager::getInstance()->getTransfers();
	qint64 bytesTotal = 0;
	qint64 bytesReceived = 0;
	bool hasActiveTransfers = false;

	for (int i = 0; i < transfers.count(); ++i)
	{
		if (transfers[i]->getState() == Transfer::RunningState && transfers[i]->getBytesTotal() > 0)
		{
			hasActiveTransfers = true;
			bytesTotal += transfers[i]->getBytesTotal();
			bytesReceived += transfers[i]->getBytesReceived();
		}
	}

	const QList<MainWindow*> windows = Application::getInstance()->getWindows();

	for (int i = 0; i < windows.count(); ++i)
	{
		MainWindow *window = windows.at(i);

		if (hasActiveTransfers)
		{
			if (!m_taskbarButtons.contains(window))
			{
				m_taskbarButtons[window] = new QWinTaskbarButton(window);
				m_taskbarButtons[window]->setWindow(window->windowHandle());
				m_taskbarButtons[window]->progress()->show();
			}

			m_taskbarButtons[window]->progress()->setValue((bytesReceived > 0) ? qFloor(((qreal) bytesReceived / bytesTotal) * 100) : 0);
		}
		else if (m_taskbarButtons.contains(window))
		{
			m_taskbarButtons[window]->progress()->reset();
			m_taskbarButtons[window]->progress()->hide();
			m_taskbarButtons[window]->deleteLater();
			m_taskbarButtons.remove(window);
		}
	}
}

void WindowsPlatformIntegration::runApplication(const QString &command, const QString &fileName) const
{
	const QString nativeFileName = QDir::toNativeSeparators(fileName);

	if (command.isEmpty())
	{
		if (!nativeFileName.isEmpty())
		{
			QDesktopServices::openUrl(QUrl(QLatin1String("file:///") + nativeFileName));
		}

		return;
	}

	const int indexOfExecutable = command.indexOf(QLatin1String(".exe"), 0, Qt::CaseInsensitive);

	if (indexOfExecutable == -1)
	{
		Console::addMessage(tr("Failed to run command \"%1\", file is not executable").arg(command), OtherMessageCategory, ErrorMessageLevel);

		return;
	}

	const QString application = command.left(indexOfExecutable + 4);
	QStringList arguments = command.mid(indexOfExecutable + 5).split(QLatin1Char(' '), QString::SkipEmptyParts);
	const int indexOfPlaceholder = arguments.indexOf(QLatin1String("%1"));

	if (nativeFileName.isEmpty())
	{
		if (indexOfPlaceholder > -1)
		{
			arguments.removeAt(indexOfPlaceholder);
		}
	}
	else
	{
		if (indexOfPlaceholder > -1)
		{
			arguments.replace(indexOfPlaceholder, arguments.at(indexOfPlaceholder).arg(nativeFileName));
		}
		else
		{
			arguments.append(nativeFileName);
		}
	}

	if (!QProcess::startDetached(application, arguments))
	{
		Console::addMessage(tr("Failed to run command \"%1\" (arguments: \"%2\")").arg(command).arg(arguments.join(QLatin1Char(' '))), OtherMessageCategory, ErrorMessageLevel);
	}
}

void WindowsPlatformIntegration::getApplicationInformation(ApplicationInformation &information)
{
	const QString rootPath = information.command.left(information.command.indexOf(QLatin1String("\\"))).remove(QLatin1Char('%'));

	if (m_environment.contains(rootPath))
	{
		information.command.replace(QLatin1Char('%') + rootPath + QLatin1Char('%'), m_environment.value(rootPath));
	}

	const QString fullApplicationPath = information.command.left(information.command.indexOf(QLatin1String(".exe"), 0, Qt::CaseInsensitive) + 4);
	const QFileInfo fileInfo(fullApplicationPath);
	const QFileIconProvider fileIconProvider;
	information.icon = fileIconProvider.icon(fileInfo);

	HKEY key = NULL;
	TCHAR readBuffer[128];
	DWORD bufferSize = sizeof(readBuffer);

	if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\Shell\\MuiCache"), 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx(key, fullApplicationPath.toStdWString().c_str(), NULL, NULL, (LPBYTE)readBuffer, &bufferSize) == ERROR_SUCCESS)
		{
			information.name = QString::fromWCharArray(readBuffer);
		}

		RegCloseKey(key);
	}

	if (information.name.isEmpty())
	{
		information.name = fileInfo.baseName();
	}
}

QList<ApplicationInformation> WindowsPlatformIntegration::getApplicationsForMimeType(const QMimeType &mimeType)
{
	QList<ApplicationInformation> applications;
	const QString suffix = mimeType.preferredSuffix();

	if (suffix.isEmpty())
	{
		Console::addMessage(tr("There is no valid suffix for given MIME type\n %1").arg(mimeType.name()), OtherMessageCategory, ErrorMessageLevel);

		return QList<ApplicationInformation>();
	}

	if (m_cleanupTimer != 0)
	{
		killTimer(m_cleanupTimer);

		m_cleanupTimer = 0;
	}

	if (m_environment.isEmpty())
	{
		m_environment = QProcessEnvironment::systemEnvironment();
	}

	// Vista+ applications
	const QSettings defaultAssociation(QLatin1String("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\.") + suffix, QSettings::NativeFormat);
	QString defaultApplication = defaultAssociation.value(QLatin1String("."), QString()).toString();
	QStringList associations;

	if (defaultApplication.isEmpty())
	{
		const QSettings defaultAssociation(QLatin1String("HKEY_LOCAL_MACHINE\\Software\\Classes\\.") + suffix, QSettings::NativeFormat);

		defaultApplication = defaultAssociation.value(QLatin1String("."), QString()).toString();
	}

	if (!defaultApplication.isEmpty())
	{
		associations.append(defaultApplication);
	}

	const QSettings modernAssociations(QLatin1String("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\.") + suffix + QLatin1String("\\OpenWithProgids"), QSettings::NativeFormat);

	associations.append(modernAssociations.childKeys());
	associations.removeAt(associations.indexOf(QLatin1String(".")));
	associations.removeDuplicates();

	for (int i = 0; i < associations.count(); ++i)
	{
		const QString value = associations.at(i);

		if (m_registrationIdentifier == value)
		{
			continue;
		}

		ApplicationInformation information;
		const QSettings applicationPath(QLatin1String("HKEY_LOCAL_MACHINE\\Software\\Classes\\") + value + QLatin1String("\\shell\\open\\command"), QSettings::NativeFormat);

		information.command = applicationPath.value(QLatin1String("."), QString()).toString().remove(QLatin1Char('"'));

		if (information.command.contains(QLatin1String("explorer.exe"), Qt::CaseInsensitive))
		{
			information.command = information.command.left(information.command.indexOf(QLatin1String(".exe"), 0, Qt::CaseInsensitive) + 4);
			information.command += " %1";
		}

		if (information.command.isEmpty())
		{
			Console::addMessage(tr("Failed to load a valid application path for MIME type %1:\n%2").arg(suffix).arg(value), OtherMessageCategory, ErrorMessageLevel);

			continue;
		}

		getApplicationInformation(information);

		applications.append(information);
	}

	// Win XP applications
	const QSettings legacyAssociations(QLatin1String("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\.") + suffix + QLatin1String("\\OpenWithList"), QSettings::NativeFormat);
	const QString order = legacyAssociations.value(QLatin1String("MRUList"), QString()).toString();
	const QString applicationFileName = QFileInfo(QCoreApplication::applicationFilePath()).fileName();

	associations = legacyAssociations.childKeys();
	associations.removeAt(associations.indexOf(QLatin1String("MRUList")));

	for (int i = 0; i < associations.count(); ++i)
	{
		ApplicationInformation information;
		const QString value = legacyAssociations.value(order.at(i)).toString();

		if (applicationFileName == value)
		{
			continue;
		}

		const QSettings applicationPath(QLatin1String("HKEY_CURRENT_USER\\SOFTWARE\\Classes\\Applications\\") + value + QLatin1String("\\shell\\open\\command"), QSettings::NativeFormat);

		information.command = applicationPath.value(QLatin1String("."), QString()).toString().remove(QLatin1Char('"'));

		if (information.command.isEmpty())
		{
			const QSettings applicationPath(QLatin1String("HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\Applications\\") + value + QLatin1String("\\shell\\open\\command"), QSettings::NativeFormat);

			information.command = applicationPath.value(QLatin1String("."), QString()).toString().remove(QLatin1Char('"'));

			if (information.command.isEmpty())
			{
				const QSettings applicationPath(QLatin1String("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\") + value, QSettings::NativeFormat);

				information.command = applicationPath.value(QLatin1String("."), QString()).toString();

				if (!information.command.isEmpty())
				{
					information.command.append(QLatin1String(" %1"));
				}
				else
				{
					Console::addMessage(tr("Failed to load a valid application path for MIME type %1:\n%2").arg(suffix).arg(value), OtherMessageCategory, ErrorMessageLevel);

					continue;
				}
			}
		}

		getApplicationInformation(information);

		bool exclude = false;

		for (int j = 0; j < applications.length(); ++j)
		{
			if (applications.at(j).name == information.name)
			{
				exclude = true;

				break;
			}
		}

		if (!exclude)
		{
			applications.append(information);
		}
	}

	m_cleanupTimer = startTimer(5 * 60000);

	return applications;
}

bool WindowsPlatformIntegration::setAsDefaultBrowser()
{
	if (!isBrowserRegistered() && !registerToSystem())
	{
		return false;
	}

	QSettings registry(QLatin1String("HKEY_CURRENT_USER\\Software"), QSettings::NativeFormat);

	for (int i = 0; i < m_registrationPairs.count(); ++i)
	{
		if (m_registrationPairs.at(i).second == ProtocolType)
		{
			registry.setValue(QLatin1String("Classes/") + m_registrationPairs.at(i).first + QLatin1String("/DefaultIcon/."), m_applicationFilePath + QLatin1String(",1"));
			registry.setValue(QLatin1String("Classes/") + m_registrationPairs.at(i).first + QLatin1String("/shell/open/command/."), QLatin1String("\"") + m_applicationFilePath + QLatin1String("\" \"%1\""));
		}
		else
		{
			registry.setValue(QLatin1String("Classes/") + m_registrationPairs.at(i).first + QLatin1String("/."), m_registrationIdentifier);
		}
	}

	registry.setValue(QLatin1String("Clients/StartmenuInternet/."), m_registrationIdentifier);
	registry.sync();

	if (QSysInfo::windowsVersion() >= QSysInfo::WV_VISTA)
	{
		IApplicationAssociationRegistrationUI *applicationAssociationRegistrationUI = NULL;
		HRESULT result = CoCreateInstance(CLSID_ApplicationAssociationRegistrationUI, NULL, CLSCTX_INPROC_SERVER, IID_IApplicationAssociationRegistrationUI, (LPVOID*)&applicationAssociationRegistrationUI);

		if (result == S_OK && applicationAssociationRegistrationUI)
		{
			result = applicationAssociationRegistrationUI->LaunchAdvancedAssociationUI(m_registrationIdentifier.toStdWString().c_str());

			applicationAssociationRegistrationUI->Release();

			if (result == S_OK)
			{
				return true;
			}
		}

		Console::addMessage(QCoreApplication::translate("main", "Failed to run File Associations Manager, error code: %1").arg(result), Otter::OtherMessageCategory, ErrorMessageLevel);
	}
	else
	{
		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_DWORD | SHCNF_FLUSH, NULL, NULL);
		Sleep(1000);
	}

	return true;
}

bool WindowsPlatformIntegration::registerToSystem()
{
	m_applicationRegistration.setValue(m_registrationIdentifier, QLatin1String("Software\\Clients\\StartMenuInternet\\OtterBrowser\\Capabilities"));
	m_applicationRegistration.sync();

	m_propertiesRegistration.setValue(QLatin1String("/."), QLatin1String("Otter Browser Document"));
	m_propertiesRegistration.setValue(QLatin1String("FriendlyTypeName"), QLatin1String("Otter Browser Document"));
	m_propertiesRegistration.setValue(QLatin1String("DefaultIcon/."), m_applicationFilePath + QLatin1String(",1"));
	m_propertiesRegistration.setValue(QLatin1String("EditFlags"), 2);
	m_propertiesRegistration.setValue(QLatin1String("shell/open/ddeexec/."), QString());
	m_propertiesRegistration.setValue(QLatin1String("shell/open/command/."), QLatin1String("\"") +  m_applicationFilePath + QLatin1String("\" \"%1\""));
	m_propertiesRegistration.sync();

	QSettings capabilities(QLatin1String("HKEY_LOCAL_MACHINE\\Software\\Clients\\StartMenuInternet\\") + m_registrationIdentifier, QSettings::NativeFormat);
	capabilities.setValue(QLatin1String("./"), QLatin1String("Otter Browser"));
	capabilities.setValue(QLatin1String("Capabilities/ApplicationDescription"), QLatin1String("Web browser controlled by the user, not vice-versa"));
	capabilities.setValue(QLatin1String("Capabilities/ApplicationIcon"), m_applicationFilePath + QLatin1String(",0"));
	capabilities.setValue(QLatin1String("Capabilities/ApplicationName"), QLatin1String("Otter Browser"));

	for (int i = 0; i < m_registrationPairs.count(); ++i)
	{
		if (m_registrationPairs.at(i).second == ProtocolType)
		{
			capabilities.setValue(QLatin1String("Capabilities/UrlAssociations/") + m_registrationPairs.at(i).first, m_registrationIdentifier);
		}
		else
		{
			capabilities.setValue(QLatin1String("Capabilities/FileAssociations/") + m_registrationPairs.at(i).first, m_registrationIdentifier);
		}
	}

	capabilities.setValue(QLatin1String("Capabilities/Startmenu/StartMenuInternet"), m_registrationIdentifier);
	capabilities.setValue(QLatin1String("DefaultIcon/."), m_applicationFilePath + QLatin1String(",0"));
	capabilities.setValue(QLatin1String("InstallInfo/IconsVisible"), 1);
	// TODO ----------------------------------------------------------------
	//capabilities.setValue("InstallInfo/HideIconsCommand", applicationFilePath + " -command");
	//capabilities.setValue("InstallInfo/ReinstallCommand", applicationFilePath + " -command");
	//capabilities.setValue("InstallInfo/ShowIconsCommand", applicationFilePath + " -command");
	// ---------------------------------------------------------------------
	capabilities.setValue(QLatin1String("shell/open/command/."), QLatin1String("\"") + m_applicationFilePath + QLatin1String("\""));
	capabilities.sync();

	if (m_applicationRegistration.status() != QSettings::NoError || capabilities.status() != QSettings::NoError)
	{
		Console::addMessage(QCoreApplication::translate("main", "Failed to register application to system registry: %0, %1").arg(m_applicationRegistration.status(), capabilities.status()), Otter::OtherMessageCategory, ErrorMessageLevel);

		return false;
	}

	return true;
}

bool WindowsPlatformIntegration::canSetAsDefaultBrowser() const
{
	return (isBrowserRegistered() ? true : m_applicationRegistration.isWritable());
}

bool WindowsPlatformIntegration::isBrowserRegistered() const
{
	if (m_applicationRegistration.value(m_registrationIdentifier).isNull() || !m_propertiesRegistration.value(QLatin1String("/shell/open/command/."), QString()).toString().contains(m_applicationFilePath))
	{
		return false;
	}

	return true;
}

bool WindowsPlatformIntegration::isDefaultBrowser() const
{
	bool isDefault = true;

	if (m_applicationRegistration.value(m_registrationIdentifier).isNull())
	{
		return false;
	}

	const QSettings registry(QLatin1String("HKEY_CURRENT_USER\\Software"), QSettings::NativeFormat);

	for (int i = 0; i < m_registrationPairs.count(); ++i)
	{
		if (m_registrationPairs.at(i).second == ProtocolType)
		{
			isDefault &= (registry.value(QLatin1String("Classes/") + m_registrationPairs.at(i).first + QLatin1String("/shell/open/command/."), QString()).toString().contains(m_applicationFilePath));
		}
		else
		{
			isDefault &= (registry.value(QLatin1String("Classes/") + m_registrationPairs.at(i).first + QLatin1String("/."), QString()).toString() == m_registrationIdentifier);

			if (QSysInfo::windowsVersion() >= QSysInfo::WV_VISTA)
			{
				isDefault &= (registry.value(QLatin1String("Microsoft/Windows/CurrentVersion/Explorer/FileExts/") + m_registrationPairs.at(i).first + QLatin1String("/UserChoice/Progid")).toString() == m_registrationIdentifier);
			}
		}
	}

	isDefault &= (registry.value("Clients/StartmenuInternet/.", QString()).toString() == m_registrationIdentifier);

	return isDefault;
}

}
