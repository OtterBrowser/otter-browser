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

#ifndef OTTER_WINDOWSPLATFORMINTEGRATION_H
#define OTTER_WINDOWSPLATFORMINTEGRATION_H

#include "../../../core/PlatformIntegration.h"

#include <QtCore/QObject>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QSettings>
#include <QtWinExtras/QtWin>
#include <QtWinExtras/QWinTaskbarButton>
#include <QtWinExtras/QWinTaskbarProgress>

#ifndef NOMINMAX // VC++ fix - http://qt-project.org/forums/viewthread/22133
#define NOMINMAX
#endif

#include <shlobj.h> // has to be after NOMINMAX!

namespace Otter
{

// Support for versions of shlobj.h that don't include the Vista+ associations API's (MinGW in Qt?)
#if !defined(IApplicationAssociationRegistrationUI)
MIDL_INTERFACE("1f76a169-f994-40ac-8fc8-0959e8874710")
IApplicationAssociationRegistrationUI : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE LaunchAdvancedAssociationUI(
		/* [string][in] */ __RPC__in_string LPCWSTR pszAppRegistryName) = 0;

};

const GUID IID_IApplicationAssociationRegistrationUI = {0x1f76a169,0xf994,0x40ac, {0x8f,0xc8,0x09,0x59,0xe8,0x87,0x47,0x10}};
#endif

enum RegistrationType
{
	ExtensionType = 0,
	ProtocolType
};

class MainWindow;

class WindowsPlatformIntegration : public PlatformIntegration
{
	Q_OBJECT

public:
	explicit WindowsPlatformIntegration(Application *parent);

	void runApplication(const QString &command, const QUrl &url = QUrl()) const;
	QList<ApplicationInformation> getApplicationsForMimeType(const QMimeType &mimeType);
	bool canShowNotifications() const;
	bool canSetAsDefaultBrowser() const;
	bool isDefaultBrowser() const;

public slots:
	void showNotification(Notification *notification);
	bool setAsDefaultBrowser();

protected:
	void timerEvent(QTimerEvent *event);
	void getApplicationInformation(ApplicationInformation &information);
	bool registerToSystem();
	bool isBrowserRegistered() const;

protected slots:
	void removeWindow(MainWindow *window);
	void updateTaskbarButtons();

private:
	QString m_registrationIdentifier;
	QString m_applicationFilePath;
	QSettings m_applicationRegistration;
	QSettings m_propertiesRegistration;
	QList<QPair<QString, RegistrationType> > m_registrationPairs;
	QHash<MainWindow*, QWinTaskbarButton*> m_taskbarButtons;
	int m_cleanupTimer;

	static QProcessEnvironment m_environment;
};

}

#endif
