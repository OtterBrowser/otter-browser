/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_ADDONSMANAGER_H
#define OTTER_ADDONSMANAGER_H

#include <QtCore/QCoreApplication>
#include <QtCore/QUrl>
#include <QtGui/QIcon>

namespace Otter
{

class WebBackend;

class AddonsManager : public QObject
{
	Q_OBJECT

public:
	struct SpecialPageInformation
	{
		QString title;
		QString description;
		QUrl url;
		QIcon icon;

		SpecialPageInformation() {}

		SpecialPageInformation(const QString &valueTitle, const QString &valueDescription, const QUrl &valueUrl, const QIcon &valueIcon) : title(valueTitle), description(valueDescription), url(valueUrl), icon(valueIcon) {}

		QString getTitle() const
		{
			return QCoreApplication::translate("addons", title.toUtf8().constData());
		}

		QString getDescription() const
		{
			return QCoreApplication::translate("addons", description.toUtf8().constData());
		}
	};

	static void createInstance(QObject *parent = NULL);
	static void registerWebBackend(WebBackend *backend, const QString &name);
	static void registerSpecialPage(const SpecialPageInformation &information, const QString &name);
	static WebBackend* getWebBackend(const QString &name = QString());
	static SpecialPageInformation getSpecialPage(const QString &name);
	static QStringList getWebBackends();
	static QStringList getSpecialPages();

protected:
	explicit AddonsManager(QObject *parent = NULL);

private:
	static AddonsManager *m_instance;
	static QHash<QString, WebBackend*> m_webBackends;
	static QHash<QString, SpecialPageInformation> m_specialPages;
};

}

#endif
