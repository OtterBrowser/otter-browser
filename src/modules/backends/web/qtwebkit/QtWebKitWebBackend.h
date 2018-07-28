/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_QTWEBKITWEBBACKEND_H
#define OTTER_QTWEBKITWEBBACKEND_H

#include "../../../../core/WebBackend.h"

namespace Otter
{

class QtWebKitPage;
class QtWebKitSpellChecker;

class QtWebKitWebBackend final : public WebBackend
{
	Q_OBJECT

public:
	enum OptionIdentifier
	{
		QtWebKitBackend_EnableMediaOption = 0,
		QtWebKitBackend_EnableMediaSourceOption,
		QtWebKitBackend_EnableWebSecurityOption
	};

	explicit QtWebKitWebBackend(QObject *parent = nullptr);

	WebWidget* createWidget(const QVariantMap &parameters, ContentsWidget *parent = nullptr) override;
	QString getName() const override;
	QString getTitle() const override;
	QString getDescription() const override;
	QString getVersion() const override;
	QString getEngineVersion() const override;
	QString getSslVersion() const override;
	QString getUserAgent(const QString &pattern = {}) const override;
	QUrl getHomePage() const override;
	BackendCapabilities getCapabilities() const override;
	static int getOptionIdentifier(OptionIdentifier identifier);
	bool requestThumbnail(const QUrl &url, const QSize &size) override;

protected:
	static QtWebKitWebBackend* getInstance();
	static QString getActiveDictionary();

protected slots:
	void handleOptionChanged(int identifier);
	void setActiveWidget(WebWidget *widget);

private:
	bool m_isInitialized;

	static QtWebKitWebBackend* m_instance;
	static QPointer<WebWidget> m_activeWidget;
	static QMap<QString, QString> m_userAgentComponents;
	static QMap<QString, QString> m_userAgents;
	static int m_enableMediaOption;
	static int m_enableMediaSourceOption;
	static int m_enableWebSecurityOption;

signals:
	void activeDictionaryChanged(const QString &dictionary);

friend class QtWebKitSpellChecker;
};

class QtWebKitWebPageThumbnailJob final : public WebPageThumbnailJob
{
	Q_OBJECT

public:
	explicit QtWebKitWebPageThumbnailJob(const QUrl &url, const QSize &size, QObject *parent = nullptr);

	bool isRunning() const;

public slots:
	void start();
	void cancel();

protected slots:
	void handlePageLoadFinished(bool result);

private:
	QtWebKitPage *m_page;
	QString m_title;
	QUrl m_url;
	QSize m_size;
	QPixmap m_pixmap;
};

}

#endif
