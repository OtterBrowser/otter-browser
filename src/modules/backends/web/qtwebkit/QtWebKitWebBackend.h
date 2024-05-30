/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2024 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include <QtWebKit/QWebElement>

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
		QtWebKitBackend_EnableSiteSpecificQuirksOption,
		QtWebKitBackend_EnableWebSecurityOption
	};

	explicit QtWebKitWebBackend(QObject *parent = nullptr);

	WebWidget* createWidget(const QVariantMap &parameters, ContentsWidget *parent = nullptr) override;
	BookmarksImportJob* createBookmarksImportJob(BookmarksModel::Bookmark *folder, const QString &path, bool areDuplicatesAllowed) override;
	WebPageThumbnailJob* createPageThumbnailJob(const QUrl &url, const QSize &size) override;
	CookieJar* getCookieJar() const override;
	QString getName() const override;
	QString getTitle() const override;
	QString getDescription() const override;
	QString getVersion() const override;
	QString getEngineVersion() const override;
	QString getSslVersion() const override;
	QString getUserAgent(const QString &pattern = {}) const override;
	QUrl getHomePage() const override;
	CapabilityScopes getCapabilityScopes(BackendCapability capability) const override;
	static int getOptionIdentifier(OptionIdentifier identifier);
	bool hasSslSupport() const override;

protected:
	static QtWebKitWebBackend* getInstance();
	static QString getActiveDictionary();

protected slots:
	void handleOptionChanged(int identifier);

private:
	bool m_isInitialized;

	static QtWebKitWebBackend* m_instance;
	static QPointer<WebWidget> m_activeWidget;
	static QHash<QString, QString> m_userAgentComponents;
	static QMap<QString, QString> m_userAgents;
	static int m_enableMediaOption;
	static int m_enableMediaSourceOption;
	static int m_enableSiteSpecificQuirksOption;
	static int m_enableWebSecurityOption;

signals:
	void activeDictionaryChanged(const QString &dictionary);

friend class QtWebKitSpellChecker;
};

class QtWebKitBookmarksImportJob final : public BookmarksImportJob
{
	Q_OBJECT

public:
	explicit QtWebKitBookmarksImportJob(BookmarksModel::Bookmark *folder, const QString &path, bool areDuplicatesAllowed, QObject *parent = nullptr);
	bool isRunning() const override;

public slots:
	void start() override;
	void cancel() override;

protected:
	void processElement(const QWebElement &element);

private:
	QString m_path;
	int m_currentAmount;
	int m_totalAmount;
	bool m_isRunning;
};

class QtWebKitWebPageThumbnailJob final : public WebPageThumbnailJob
{
	Q_OBJECT

public:
	explicit QtWebKitWebPageThumbnailJob(const QUrl &url, const QSize &size, QObject *parent = nullptr);

	QString getTitle() const override;
	QPixmap getThumbnail() const override;
	bool isRunning() const override;

public slots:
	void start() override;
	void cancel() override;

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
