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

#include "WindowsPlatformIntegration.h"
#include "../../../core/Application.h"
#include "../../../core/Console.h"
#include "../../../core/TransfersManager.h"
#include "../../../ui/MainWindow.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QtMath>

namespace Otter
{

WindowsPlatformIntegration::WindowsPlatformIntegration(Application *parent) : PlatformIntegration(parent),
	m_registrationIdentifier(QLatin1String("OtterBrowser")),
	m_applicationFilePath(QDir::toNativeSeparators(QCoreApplication::applicationFilePath())),
	m_applicationRegistration(QLatin1String("HKEY_LOCAL_MACHINE\\Software\\RegisteredApplications"), QSettings::NativeFormat),
	m_propertiesRegistration(QLatin1String("HKEY_CURRENT_USER\\Software\\Classes\\") + m_registrationIdentifier, QSettings::NativeFormat)
{
	m_registrationPairs << qMakePair(QLatin1String("http"), ProtocolType) << qMakePair(QLatin1String("https"), ProtocolType) << qMakePair(QLatin1String("ftp"), ProtocolType)
	<< qMakePair(QLatin1String(".htm"), ExtensionType) << qMakePair(QLatin1String(".html"), ExtensionType) << qMakePair(QLatin1String(".xhtml"), ExtensionType);

	if (QSysInfo::windowsVersion() >= QSysInfo::WV_WINDOWS7)
	{
		connect(Application::getInstance(), SIGNAL(windowRemoved(MainWindow*)), this, SLOT(removeWindow(MainWindow*)));
		connect(TransfersManager::getInstance(), SIGNAL(transferUpdated(TransferInformation*)), this , SLOT(updateTaskbarButtons()));
		connect(TransfersManager::getInstance(), SIGNAL(transferStarted(TransferInformation*)), this , SLOT(updateTaskbarButtons()));
		connect(TransfersManager::getInstance(), SIGNAL(transferFinished(TransferInformation*)), this , SLOT(updateTaskbarButtons()));
		connect(TransfersManager::getInstance(), SIGNAL(transferRemoved(TransferInformation*)), this , SLOT(updateTaskbarButtons()));
		connect(TransfersManager::getInstance(), SIGNAL(transferStopped(TransferInformation*)), this , SLOT(updateTaskbarButtons()));
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
	const QList<TransferInformation*> transfers = TransfersManager::getInstance()->getTransfers();
	qint64 bytesTotal = 0;
	qint64 bytesReceived = 0;
	bool hasActiveTransfers = false;

	for (int i = 0; i < transfers.count(); ++i)
	{
		if (transfers[i]->state == RunningTransfer && transfers[i]->bytesTotal > 0)
		{
			hasActiveTransfers = true;
			bytesTotal += transfers[i]->bytesTotal;
			bytesReceived += transfers[i]->bytesReceived;
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
		IApplicationAssociationRegistrationUI *applicationAssociationRegistrationUI = 0;
		HRESULT result = CoCreateInstance(CLSID_ApplicationAssociationRegistrationUI, 0, CLSCTX_INPROC_SERVER, IID_IApplicationAssociationRegistrationUI, (LPVOID*)&applicationAssociationRegistrationUI);

		if (result == S_OK && applicationAssociationRegistrationUI)
		{
			result = applicationAssociationRegistrationUI->LaunchAdvancedAssociationUI(m_registrationIdentifier.toStdWString().c_str());

			applicationAssociationRegistrationUI->Release();

			if (result == S_OK)
			{
				return true;
			}
		}

		Console::addMessage(QCoreApplication::translate("main", "Failed to start association dialog: %0").arg(result), Otter::OtherMessageCategory, ErrorMessageLevel);
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
	return isBrowserRegistered() ? true : m_applicationRegistration.isWritable();
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
