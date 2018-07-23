/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2016 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_WINDOWSPLATFORMINTEGRATION_H
#define OTTER_WINDOWSPLATFORMINTEGRATION_H

#include "../../../core/PlatformIntegration.h"

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

#if !defined(IApplicationActivationManager)
enum ACTIVATEOPTIONS
{
	AO_NONE = 0,
	AO_DESIGNMODE = 1,
	AO_NOERRORUI = 2,
	AO_NOSPLASHSCREEN = 4
};

MIDL_INTERFACE("2e941141-7f97-4756-ba1d-9decde894a3d")
IApplicationActivationManager : public IUnknown
{
public:

	virtual HRESULT STDMETHODCALLTYPE ActivateApplication(
		/* [in] */ __RPC__in LPCWSTR appUserModelId,
		/* [unique][in] */ __RPC__in_opt LPCWSTR arguments,
		/* [in] */ ACTIVATEOPTIONS options,
		/* [out] */ __RPC__out DWORD *processId) = 0;
};

const GUID IID_IApplicationActivationManager = {0x2e941141,0x7f97,0x4756, {0xba,0x1d,0x9d,0xec,0xde,0x89,0x4a,0x3d}};
const CLSID CLSID_ApplicationActivationManager = {0x45BA127D,0x10A8,0x46EA, {0x8A,0xB7,0x56,0xEA,0x90,0x78,0x94,0x3C}};
#endif

enum RegistrationType
{
	ExtensionType = 0,
	ProtocolType
};

class MainWindow;

class WindowsPlatformIntegration final : public PlatformIntegration
{
	Q_OBJECT

public:
	explicit WindowsPlatformIntegration(QObject *parent);

	void runApplication(const QString &command, const QUrl &url = {}) const override;
	void startLinkDrag(const QUrl &url, const QString &title, const QPixmap &pixmap, QObject *parent = nullptr) const override;
	Style* createStyle(const QString &name) const override;
	QVector<ApplicationInformation> getApplicationsForMimeType(const QMimeType &mimeType) override;
	QString getPlatformName() const override;
	bool canShowNotifications() const override;
	bool canSetAsDefaultBrowser() const override;
	bool isDefaultBrowser() const override;

public slots:
	void showNotification(Notification *notification) override;
	bool setAsDefaultBrowser() override;

protected:
	void timerEvent(QTimerEvent *event);
	void getApplicationInformation(ApplicationInformation &information);
	QString getUpdaterBinary() const override;
	bool registerToSystem();
	bool isBrowserRegistered() const;

protected slots:
	void removeWindow(MainWindow *mainWindow);
	void updateTaskbarButtons();

private:
	QString m_registrationIdentifier;
	QString m_applicationFilePath;
	QSettings m_applicationRegistration;
	QSettings m_propertiesRegistration;
	QVector<QPair<QString, RegistrationType> > m_registrationPairs;
	QHash<MainWindow*, QWinTaskbarButton*> m_taskbarButtons;
	int m_cleanupTimer;

	static QProcessEnvironment m_environment;
};

}

#endif
