/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "QtWebEngineUrlRequestInterceptor.h"
#include "../../../../core/Console.h"
#include "../../../../core/ContentBlockingManager.h"
#include "../../../../core/SettingsManager.h"

#include <QtCore/QCoreApplication>

namespace Otter
{

QtWebEngineUrlRequestInterceptor::QtWebEngineUrlRequestInterceptor(QObject *parent) : QWebEngineUrlRequestInterceptor(parent)
{
	optionChanged(QLatin1String("Content/BlockingProfiles"), SettingsManager::getValue(QLatin1String("Content/BlockingProfiles")));

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
}

void QtWebEngineUrlRequestInterceptor::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("Content/BlockingProfiles"))
	{
		m_contentBlockingProfiles = ContentBlockingManager::getProfileList(value.toStringList());
	}
}

void QtWebEngineUrlRequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo &request)
{
	if (m_contentBlockingProfiles.isEmpty())
	{
		return;
	}

	ContentBlockingManager::ResourceType resourceType(ContentBlockingManager::OtherType);

	switch (request.resourceType())
	{
		case QWebEngineUrlRequestInfo::ResourceTypeMainFrame:
			resourceType = ContentBlockingManager::MainFrameType;

			break;
		case QWebEngineUrlRequestInfo::ResourceTypeSubFrame:
			resourceType = ContentBlockingManager::SubFrameType;

			break;
		case QWebEngineUrlRequestInfo::ResourceTypeStylesheet:
			resourceType = ContentBlockingManager::StyleSheetType;

			break;
		case QWebEngineUrlRequestInfo::ResourceTypeScript:
			resourceType = ContentBlockingManager::ScriptType;

			break;
		case QWebEngineUrlRequestInfo::ResourceTypeImage:
			resourceType = ContentBlockingManager::ImageType;

			break;
		case QWebEngineUrlRequestInfo::ResourceTypeObject:
		case QWebEngineUrlRequestInfo::ResourceTypeMedia:
			resourceType = ContentBlockingManager::ObjectType;

			break;
		case QWebEngineUrlRequestInfo::ResourceTypeXhr:
			resourceType = ContentBlockingManager::XmlHttpRequestType;

			break;
		default:
			break;
	}

	const ContentBlockingManager::CheckResult result(ContentBlockingManager::checkUrl(m_contentBlockingProfiles, request.firstPartyUrl(), request.requestUrl(), resourceType));

	if (result.isBlocked)
	{
		Console::addMessage(QCoreApplication::translate("main", "Blocked request"), Otter::NetworkMessageCategory, LogMessageLevel, request.requestUrl().toString(), -1);

		request.block(true);
	}
}

}
