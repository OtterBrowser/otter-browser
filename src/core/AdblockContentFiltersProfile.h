/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2010 - 2014 David Rosca <nowrep@gmail.com>
* Copyright (C) 2014 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>

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

#ifndef OTTER_ADBLOCKCONTENTFILTERSPROFILE_H
#define OTTER_ADBLOCKCONTENTFILTERSPROFILE_H

#include "ContentFiltersManager.h"

#include <QtCore/QRegularExpression>

namespace Otter
{

class DataFetchJob;

class AdblockContentFiltersProfile final : public ContentFiltersProfile
{
	Q_OBJECT

public:
	enum RuleType
	{
		AnyRule = 0,
		ActiveRule,
		CosmeticRule,
		WildcardRule
	};

	struct HeaderInformation final
	{
		QString title;
		QString errorString;
		QUrl updateUrl;
		ProfileError error = NoError;

		bool hasError() const
		{
			return (error != NoError);
		}
	};

	explicit AdblockContentFiltersProfile(const ProfileSummary &summary, const QStringList &languages, ProfileFlags flags, QObject *parent = nullptr);

	void clear() override;
	void setProfileSummary(const ProfileSummary &summary) override;
	QString getName() const override;
	QString getTitle() const override;
	QString getPath() const override;
	QUrl getUpdateUrl() const override;
	QDateTime getLastUpdate() const override;
	ProfileSummary getProfileSummary() const override;
	ContentFiltersManager::CosmeticFiltersResult getCosmeticFilters(const QStringList &domains, bool isDomainOnly) override;
	ContentFiltersManager::CheckResult checkUrl(const QUrl &baseUrl, const QUrl &requestUrl, NetworkManager::ResourceType resourceType) override;
	static HeaderInformation loadHeader(QIODevice *rulesDevice);
	static QHash<RuleType, quint32> loadRulesInformation(const ProfileSummary &summary, QIODevice *rulesDevice);
	QVector<QLocale::Language> getLanguages() const override;
	ProfileCategory getCategory() const override;
	ContentFiltersManager::CosmeticFiltersMode getCosmeticFiltersMode() const override;
	ProfileError getError() const override;
	ProfileFlags getFlags() const override;
	int getUpdateInterval() const override;
	int getUpdateProgress() const override;
	static bool create(const ProfileSummary &summary, QIODevice *rulesDevice = nullptr, bool canOverwriteExisting = false);
	static bool create(const QUrl &url, bool canOverwriteExisting = false);
	bool update(const QUrl &url = {}) override;
	bool remove() override;
	bool areWildcardsEnabled() const override;
	bool isUpdating() const override;

protected:
	enum RuleOption : quint16
	{
		NoOption = 0,
		ThirdPartyOption = 1,
		StyleSheetOption = 2,
		ScriptOption = 4,
		ImageOption = 8,
		ObjectOption = 16,
		ObjectSubRequestOption = 32,
		SubDocumentOption = 64,
		XmlHttpRequestOption = 128,
		WebSocketOption = 256,
		PopupOption = 512,
		ElementHideOption = 1024,
		GenericHideOption = 2048
	};

	Q_DECLARE_FLAGS(RuleOptions, RuleOption)

	enum RuleMatch
	{
		ContainsMatch = 0,
		StartMatch,
		EndMatch,
		ExactMatch
	};

	struct Node final
	{
		struct Rule final
		{
			QString rule;
			QStringList blockedDomains;
			QStringList allowedDomains;
			RuleOptions ruleOptions = NoOption;
			RuleOptions ruleExceptions = NoOption;
			RuleMatch ruleMatch = ContainsMatch;
			bool isException = false;
			bool needsDomainCheck = false;
		};

		QChar value;
		QVarLengthArray<Node*, 1> children;
		QVarLengthArray<Rule*, 1> rules;
	};

	struct Request final
	{
		QString baseHost;
		QString requestHost;
		QString requestUrl;
		NetworkManager::ResourceType resourceType = NetworkManager::OtherType;

		explicit Request(const QUrl &baseUrlValue, const QUrl &requestUrlValue, NetworkManager::ResourceType resourceTypeValue) : baseHost(baseUrlValue.host()), requestHost(requestUrlValue.host()), requestUrl(requestUrlValue.toString()), resourceType(resourceTypeValue)
		{
			if (requestUrl.startsWith(QLatin1String("//")))
			{
				requestUrl = requestUrl.mid(2);
			}
		}
	};

	void loadHeader();
	void parseRuleLine(const QString &rule);
	void deleteNode(Node *node) const;
	QMultiHash<QString, QString> parseStyleSheetRule(const QStringList &line);
	ContentFiltersManager::CheckResult checkUrlSubstring(const Node *node, const QString &substring, QString currentRule, const Request &request) const;
	ContentFiltersManager::CheckResult checkRuleMatch(const Node::Rule *rule, const QString &currentRule, const Request &request) const;
	ContentFiltersManager::CheckResult evaluateNodeRules(const Node *node, const QString &currentRule, const Request &request) const;
	bool loadRules();
	bool domainContains(const QString &host, const QStringList &domains) const;

protected slots:
	void raiseError(const QString &message, ProfileError error);
	void handleJobFinished(bool isSuccess);

private:
	Node *m_root;
	DataFetchJob *m_dataFetchJob;
	ProfileSummary m_summary;
	QRegularExpression m_domainExpression;
	QStringList m_cosmeticFiltersRules;
	QVector<QLocale::Language> m_languages;
	QMultiHash<QString, QString> m_cosmeticFiltersDomainRules;
	QMultiHash<QString, QString> m_cosmeticFiltersDomainExceptions;
	ProfileError m_error;
	ProfileFlags m_flags;
	bool m_wasLoaded;

	static QHash<QString, RuleOption> m_options;
	static QHash<NetworkManager::ResourceType, RuleOption> m_resourceTypes;
};

}

#endif
