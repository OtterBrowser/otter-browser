/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_QTWEBKITWEBBACKEND_H
#define OTTER_QTWEBKITWEBBACKEND_H

#include "../../../../core/WebBackend.h"

namespace Otter
{

class QtWebKitPage;
class QtWebKitSpellChecker;

class QtWebKitWebBackend : public WebBackend
{
	Q_OBJECT

public:
	enum OptionIdentifier
	{
		QtWebKitBackend_EnableMediaOption = 0,
		QtWebKitBackend_EnableMediaSourceOption
	};

	explicit QtWebKitWebBackend(QObject *parent = nullptr);
	~QtWebKitWebBackend();

	WebWidget* createWidget(bool isPrivate = false, ContentsWidget *parent = nullptr);
	QString getTitle() const;
	QString getDescription() const;
	QString getVersion() const;
	QString getEngineVersion() const;
	QString getSslVersion() const;
	QString getUserAgent(const QString &pattern = QString()) const;
	QUrl getHomePage() const;
	QIcon getIcon() const;
	QList<SpellCheckManager::DictionaryInformation> getDictionaries() const;
	static int getOptionIdentifier(OptionIdentifier identifier);
	bool requestThumbnail(const QUrl &url, const QSize &size);

protected:
	static QtWebKitWebBackend* getInstance();
	static QString getActiveDictionary();

protected slots:
	void optionChanged(int identifier);
	void pageLoaded(bool success);
	void setActiveWidget(WebWidget *widget);

private:
	QHash<QtWebKitPage*, QPair<QUrl, QSize> > m_thumbnailRequests;
	bool m_isInitialized;

	static QtWebKitWebBackend* m_instance;
	static QPointer<WebWidget> m_activeWidget;
	static QMap<QString, QString> m_userAgentComponents;
	static QMap<QString, QString> m_userAgents;
	static int m_enableMediaOption;
	static int m_enableMediaSourceOption;

signals:
	void activeDictionaryChanged(const QString &dictionary);

friend class QtWebKitSpellChecker;
};

}

#endif
