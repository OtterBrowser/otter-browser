/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2010-2014 David Rosca <nowrep@gmail.com>
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

#ifndef OTTER_CONTENTBLOCKINGLIST_H
#define OTTER_CONTENTBLOCKINGLIST_H

#include "NetworkManager.h"

#include <QtCore/QObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QUrl>

namespace Otter
{

class ContentBlockingList : public QObject
{
	Q_OBJECT

public:
	explicit ContentBlockingList(QObject *parent = NULL);

	enum RuleType
	{
		StandardFilterRule = 1,
		ElementHidingRule = 2,
		ExceptionRule = 4,
	};

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
		QString rule;
		QStringList blockedDomains;
		QStringList allowedDomains;
		RuleType ruleType;
		RuleOptions ruleOption;
		RuleOptions exceptionRuleOption;
		bool isException;
		bool needsDomainCheck;
	};

	void setEnabled(const bool enabled);
	void setFile(const QString &path, const QString &name);
	void setListName(const QString &title);
	void setConfigListName(const QString &name);
	QString getFileName() const;
	QString getListName() const;
	QString getConfigListName() const;
	QString getCssRules() const;
	QDateTime getLastUpdate() const;
	QMultiHash<QString, QString> getSpecificDomainHidingRules() const;
	QMultiHash<QString, QString> getHidingRulesExceptions() const;
	bool isEnabled() const;
	bool isUrlBlocked(const QNetworkRequest &request, const QUrl &baseUrl);

protected:
	struct Node
	{
		QChar value;
		ContentBlockingRule rule;
		QHash<QChar, Node*> children;

		Node() : value(0) {}
	};

	void parseRules();
	void loadRuleFile();
	void clear();
	void parseRuleLine(QString line);
	void resolveRuleOptions(const ContentBlockingRule &rule, const QNetworkRequest &request, bool &isBlocked);
	void parseCssRule(const QStringList &line, QMultiHash<QString, QString> &list);
	void addRule(const ContentBlockingRule &rule);
	void deleteNode(Node *node);
	void downloadUpdate();
	bool resolveDomainExceptions(const QString &url, const QStringList &ruleList);
	bool checkUrlSubstring(const QString &subString, const QNetworkRequest &request);
	bool checkRuleMatch(const ContentBlockingRule &rule, const QNetworkRequest &request);

private slots:
	void updateDownloaded(QNetworkReply *reply);

private:
	Node *m_root;
	QDateTime m_lastUpdate;
	QString m_fullFilePath;
	QString m_fileName;
	QString m_listName;
	QString m_configListName;
	QString m_cssHidingRules;
	QUrl m_baseUrl;
	QUrl m_updateUrl;
	NetworkManager m_networkManager;
	QMultiHash<QString, QString> m_cssSpecificDomainHidingRules;
	QMultiHash<QString, QString> m_cssHidingRulesExceptions;
	QRegularExpression m_domainExpression;
	QStringList m_requestSubdomainList;
	int m_daysToExpire;
	bool m_isUpdated;
	bool m_isEnabled;

signals:
	void updateCustomStyleSheets();
};

}

#endif
