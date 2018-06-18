/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "QtWebEngineUrlRequestInterceptor.h"
#include "../../../../core/Console.h"
#include "../../../../core/ContentFiltersManager.h"
#include "../../../../core/NetworkManagerFactory.h"
#include "../../../../core/SettingsManager.h"
#include "../../../../core/Utils.h"

#include <QtCore/QCoreApplication>

namespace Otter
{

QtWebEngineUrlRequestInterceptor::QtWebEngineUrlRequestInterceptor(QObject *parent) : QWebEngineUrlRequestInterceptor(parent),
	m_clearTimer(0),
	m_areImagesEnabled(SettingsManager::getOption(SettingsManager::Permissions_EnableImagesOption).toString() != QLatin1String("disabled"))
{
	m_clearTimer = startTimer(1800000);

	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &QtWebEngineUrlRequestInterceptor::handleOptionChanged);
	connect(SettingsManager::getInstance(), &SettingsManager::hostOptionChanged, this, &QtWebEngineUrlRequestInterceptor::handleOptionChanged);
}

void QtWebEngineUrlRequestInterceptor::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_clearTimer)
	{
		clearContentBlockingInformation();
	}
}

void QtWebEngineUrlRequestInterceptor::clearContentBlockingInformation()
{
	m_blockedElements.clear();
	m_contentBlockingProfiles.clear();
}

void QtWebEngineUrlRequestInterceptor::handleOptionChanged(int identifier)
{
	switch (identifier)
	{
		case SettingsManager::ContentBlocking_ProfilesOption:
			clearContentBlockingInformation();

			break;
		case SettingsManager::Permissions_EnableImagesOption:
			m_areImagesEnabled = (SettingsManager::getOption(SettingsManager::Permissions_EnableImagesOption).toString() != QLatin1String("disabled"));

			break;
		default:
			break;
	}
}

QStringList QtWebEngineUrlRequestInterceptor::getBlockedElements(const QString &domain) const
{
	return m_blockedElements.value(domain);
}

void QtWebEngineUrlRequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo &request)
{
	if (!m_areImagesEnabled && request.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeImage)
	{
		request.block(true);

		return;
	}

	if (!m_contentBlockingProfiles.contains(request.firstPartyUrl().host()))
	{
		const QString host(Utils::extractHost(request.firstPartyUrl()));

		if (SettingsManager::getOption(SettingsManager::ContentBlocking_EnableContentBlockingOption, host).toBool())
		{
			m_contentBlockingProfiles[host] = ContentFiltersManager::getProfileIdentifiers(SettingsManager::getOption(SettingsManager::ContentBlocking_ProfilesOption, host).toStringList());
		}
		else
		{
			m_contentBlockingProfiles[host] = {};
		}
	}

	const QVector<int> contentBlockingProfiles(m_contentBlockingProfiles.value(request.firstPartyUrl().host()));

	if (!contentBlockingProfiles.isEmpty())
	{
		NetworkManager::ResourceType resourceType(NetworkManager::OtherType);
		bool storeBlockedUrl(true);

		switch (request.resourceType())
		{
			case QWebEngineUrlRequestInfo::ResourceTypeMainFrame:
				resourceType = NetworkManager::MainFrameType;

				break;
			case QWebEngineUrlRequestInfo::ResourceTypeSubFrame:
				resourceType = NetworkManager::SubFrameType;

				break;
			case QWebEngineUrlRequestInfo::ResourceTypeStylesheet:
				resourceType = NetworkManager::StyleSheetType;
				storeBlockedUrl = false;

				break;
			case QWebEngineUrlRequestInfo::ResourceTypeScript:
				resourceType = NetworkManager::ScriptType;
				storeBlockedUrl = false;

				break;
			case QWebEngineUrlRequestInfo::ResourceTypeImage:
				resourceType = NetworkManager::ImageType;

				break;
			case QWebEngineUrlRequestInfo::ResourceTypeObject:
			case QWebEngineUrlRequestInfo::ResourceTypeMedia:
				resourceType = NetworkManager::ObjectType;

				break;
			case QWebEngineUrlRequestInfo::ResourceTypePluginResource:
				resourceType = NetworkManager::ObjectSubrequestType;
				storeBlockedUrl = false;

				break;
			case QWebEngineUrlRequestInfo::ResourceTypeXhr:
				resourceType = NetworkManager::XmlHttpRequestType;

				break;
			default:
				break;
		}

		const ContentFiltersManager::CheckResult result(ContentFiltersManager::checkUrl(contentBlockingProfiles, request.firstPartyUrl(), request.requestUrl(), resourceType));

		if (result.isBlocked)
		{
			const ContentFiltersProfile *profile(ContentFiltersManager::getProfile(result.profile));

			Console::addMessage(QCoreApplication::translate("main", "Request blocked by rule from profile %1:\n%2").arg(profile ? profile->getTitle() : QCoreApplication::translate("main", "(Unknown)")).arg(result.rule), Console::NetworkCategory, Console::LogLevel, request.requestUrl().toString(), -1);

			if (storeBlockedUrl && !m_blockedElements.value(request.firstPartyUrl().host()).contains(request.requestUrl().url()))
			{
				m_blockedElements[request.firstPartyUrl().host()].append(request.requestUrl().url());
			}

			request.block(true);

			return;
		}
	}

	const NetworkManagerFactory::DoNotTrackPolicy doNotTrackPolicy(NetworkManagerFactory::getDoNotTrackPolicy());

	if (doNotTrackPolicy != NetworkManagerFactory::SkipTrackPolicy)
	{
		request.setHttpHeader(QStringLiteral("DNT").toLatin1(), ((doNotTrackPolicy == NetworkManagerFactory::DoNotAllowToTrackPolicy) ? QStringLiteral("1") : QStringLiteral("0")).toLatin1());
	}

	if (!SettingsManager::getOption(SettingsManager::Network_EnableReferrerOption).toBool())
	{
		request.setHttpHeader(QStringLiteral("Referer").toLatin1(), QByteArray());
	}
}

}
