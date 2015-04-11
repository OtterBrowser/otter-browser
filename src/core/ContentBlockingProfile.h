/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2010 - 2014 David Rosca <nowrep@gmail.com>
* Copyright (C) 2014 - 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>

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

#ifndef OTTER_CONTENTBLOCKINGPROFILE_H
#define OTTER_CONTENTBLOCKINGPROFILE_H

#include "NetworkManager.h"

#include <QtCore/QObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QUrl>

namespace Otter
{

struct ContentBlockingInformation
{
	QString name;
	QString title;
	QString path;
	QUrl updateUrl;
};

class ContentBlockingProfile : public QObject
{
	Q_OBJECT

public:
	enum RuleOption
	{
		NoOption = 0,
		ThirdPartyOption = 1,
		StyleSheetOption = 2,
		ScriptOption = 4,
		ImageOption = 8,
		ObjectOption = 16,
		ObjectSubRequestOption = 32,
		SubDocumentOption = 64,
		XmlHttpRequestOption = 128
	};

	Q_DECLARE_FLAGS(RuleOptions, RuleOption)

	struct ContentBlockingRule
	{
		QStringList blockedDomains;
		QStringList allowedDomains;
		RuleOptions ruleOption;
		RuleOptions exceptionRuleOption;
		bool isException;
		bool needsDomainCheck;
	};

	explicit ContentBlockingProfile(const QString &path, QObject *parent = NULL);

	QString getStyleSheet();
	ContentBlockingInformation getInformation() const;
	QMultiHash<QString, QString> getStyleSheetWhiteList();
	QMultiHash<QString, QString> getStyleSheetBlackList();
	bool isUrlBlocked(const QNetworkRequest &request, const QUrl &baseUrl);

protected:
	struct Node
	{
		QChar value;
		ContentBlockingRule *rule;
		QVarLengthArray<Node*, 5> children;

		Node() : value(0), rule(NULL) {}
	};

	void load(bool onlyHeader = false);
	void parseRuleLine(QString line);
	void resolveRuleOptions(ContentBlockingRule *rule, const QNetworkRequest &request, bool &isBlocked);
	void parseStyleSheetRule(const QStringList &line, QMultiHash<QString, QString> &list);
	void addRule(ContentBlockingRule *rule, const QString &ruleString);
	void deleteNode(Node *node);
	void downloadUpdate();
	bool loadRules();
	bool resolveDomainExceptions(const QString &url, const QStringList &ruleList);
	bool checkUrlSubstring(const QString &subString, const QNetworkRequest &request);
	bool checkRuleMatch(ContentBlockingRule *rule, const QNetworkRequest &request);

private slots:
	void updateDownloaded(QNetworkReply *reply);

private:
	Node *m_root;
	QNetworkReply *m_networkReply;
	QString m_styleSheet;
	QString m_currentRule;
	QUrl m_baseUrl;
	QRegularExpression m_domainExpression;
	ContentBlockingInformation m_information;
	QStringList m_requestSubdomainList;
	QMultiHash<QString, QString> m_styleSheetBlackList;
	QMultiHash<QString, QString> m_styleSheetWhiteList;
	bool m_updateRequested;
	bool m_isEmpty;
	bool m_wasLoaded;

	static NetworkManager *m_networkManager;

signals:
	void updateCustomStyleSheets();
};

}

#endif
