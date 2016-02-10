/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2010 - 2014 David Rosca <nowrep@gmail.com>
* Copyright (C) 2014 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2015 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>

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

#include "ContentBlockingManager.h"

#include <QtCore/QObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkReply>

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

	ContentBlockingInformation getInformation() const;
	QStringList getStyleSheet();
	QMultiHash<QString, QString> getStyleSheetBlackList();
	QMultiHash<QString, QString> getStyleSheetWhiteList();
	bool downloadRules();
	bool isUrlBlocked(const QUrl &baseUrl, const QUrl &requestUrl, ContentBlockingManager::ResourceType resourceType);

protected:
	struct Node
	{
		ContentBlockingRule *rule;
		QChar value;
		QVarLengthArray<Node*, 5> children;

		Node() : rule(NULL), value(0) {}
	};

	void clear();
	void load(bool onlyHeader = false);
	void parseRuleLine(QString line);
	void resolveRuleOptions(ContentBlockingRule *rule, ContentBlockingManager::ResourceType resourceType, bool &isBlocked);
	void parseStyleSheetRule(const QStringList &line, QMultiHash<QString, QString> &list);
	void addRule(ContentBlockingRule *rule, const QString &ruleString);
	void deleteNode(Node *node);
	bool loadRules();
	bool resolveDomainExceptions(const QString &url, const QStringList &ruleList);
	bool checkUrlSubstring(Node *node, const QString &subString, QString currentRule, ContentBlockingManager::ResourceType resourceType);
	bool checkRuleMatch(ContentBlockingRule *rule, const QString &currentRule, ContentBlockingManager::ResourceType resourceType);

protected slots:
	void optionChanged(const QString &option);
	void replyFinished();

private:
	Node *m_root;
	QNetworkReply *m_networkReply;
	QString m_requestUrl;
	QString m_requestHost;
	QString m_baseUrlHost;
	QRegularExpression m_domainExpression;
	ContentBlockingInformation m_information;
	QStringList m_requestSubdomainList;
	QStringList m_styleSheet;
	QMultiHash<QString, QString> m_styleSheetBlackList;
	QMultiHash<QString, QString> m_styleSheetWhiteList;
	bool m_enableWildcards;
	bool m_isUpdating;
	bool m_isEmpty;
	bool m_wasLoaded;

signals:
	void profileModified(const QString &profile);
};

}

#endif
