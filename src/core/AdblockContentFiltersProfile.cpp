/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2010 - 2014 David Rosca <nowrep@gmail.com>
* Copyright (C) 2014 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "AdblockContentFiltersProfile.h"
#include "Console.h"
#include "NetworkManager.h"
#include "NetworkManagerFactory.h"
#include "SessionsManager.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QSaveFile>
#include <QtCore/QTextStream>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace Otter
{

QVector<QChar> AdblockContentFiltersProfile::m_separators({QLatin1Char('_'), QLatin1Char('-'), QLatin1Char('.'), QLatin1Char('%')});
QHash<QString, AdblockContentFiltersProfile::RuleOption> AdblockContentFiltersProfile::m_options({{QLatin1String("third-party"), ThirdPartyOption}, {QLatin1String("stylesheet"), StyleSheetOption}, {QLatin1String("image"), ImageOption}, {QLatin1String("script"), ScriptOption}, {QLatin1String("object"), ObjectOption}, {QLatin1String("object-subrequest"), ObjectSubRequestOption}, {QLatin1String("object_subrequest"), ObjectSubRequestOption}, {QLatin1String("subdocument"), SubDocumentOption}, {QLatin1String("xmlhttprequest"), XmlHttpRequestOption}, {QLatin1String("websocket"), WebSocketOption}, {QLatin1String("popup"), PopupOption}, {QLatin1String("elemhide"), ElementHideOption}, {QLatin1String("generichide"), GenericHideOption}});
QHash<NetworkManager::ResourceType, AdblockContentFiltersProfile::RuleOption> AdblockContentFiltersProfile::m_resourceTypes({{NetworkManager::ImageType, ImageOption}, {NetworkManager::ScriptType, ScriptOption}, {NetworkManager::StyleSheetType, StyleSheetOption}, {NetworkManager::ObjectType, ObjectOption}, {NetworkManager::XmlHttpRequestType, XmlHttpRequestOption}, {NetworkManager::SubFrameType, SubDocumentOption},{NetworkManager::PopupType, PopupOption}, {NetworkManager::ObjectSubrequestType, ObjectSubRequestOption}, {NetworkManager::WebSocketType, WebSocketOption}});

AdblockContentFiltersProfile::AdblockContentFiltersProfile(const QString &name, const QString &title, const QUrl &updateUrl, const QDateTime &lastUpdate, const QStringList &languages, int updateInterval, const ProfileCategory &category, const ProfileFlags &flags, QObject *parent) : ContentFiltersProfile(parent),
	m_root(nullptr),
	m_networkReply(nullptr),
	m_name(name),
	m_title(title),
	m_updateUrl(updateUrl),
	m_lastUpdate(lastUpdate),
	m_category(category),
	m_error(NoError),
	m_flags(flags),
	m_updateInterval(updateInterval),
	m_isUpdating(false),
	m_isEmpty(true),
	m_wasLoaded(false)
{
	if (languages.isEmpty())
	{
		m_languages = {QLocale::AnyLanguage};
	}
	else
	{
		for (int i = 0; i < languages.count(); ++i)
		{
			m_languages.append(QLocale(languages.at(i)).language());
		}
	}

	loadHeader();
}

void AdblockContentFiltersProfile::clear()
{
	if (!m_wasLoaded)
	{
		return;
	}

	if (m_root)
	{
		QtConcurrent::run(this, &AdblockContentFiltersProfile::deleteNode, m_root);
	}

	m_cosmeticFiltersRules.clear();
	m_cosmeticFiltersDomainExceptions.clear();
	m_cosmeticFiltersDomainRules.clear();

	m_wasLoaded = false;
}

void AdblockContentFiltersProfile::loadHeader()
{
	const QString &path(getPath());

	if (!QFile::exists(path))
	{
		return;
	}

	QFile file(path);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		m_error = ReadError;

		Console::addMessage(QCoreApplication::translate("main", "Failed to open content blocking profile file: %1").arg(file.errorString()), Console::OtherCategory, Console::ErrorLevel, file.fileName());

		return;
	}

	QTextStream stream(&file);

	while (!stream.atEnd())
	{
		QString line(stream.readLine().trimmed());

		if (!line.startsWith(QLatin1Char('!')))
		{
			m_isEmpty = false;

			break;
		}

		if (!m_flags.testFlag(HasCustomTitleFlag) && line.startsWith(QLatin1String("! Title: ")))
		{
			m_title = line.remove(QLatin1String("! Title: "));

			continue;
		}
	}

	file.close();

	if (!m_isUpdating && m_updateInterval > 0 && (!m_lastUpdate.isValid() || m_lastUpdate.daysTo(QDateTime::currentDateTimeUtc()) > m_updateInterval))
	{
		update();
	}
}

void AdblockContentFiltersProfile::parseRuleLine(const QString &rule)
{
	if (rule.indexOf(QLatin1Char('!')) == 0 || rule.isEmpty())
	{
		return;
	}

	if (rule.startsWith(QLatin1String("##")))
	{
		if (ContentFiltersManager::getCosmeticFiltersMode() == ContentFiltersManager::AllFilters)
		{
			m_cosmeticFiltersRules.append(rule.mid(2));
		}

		return;
	}

	if (rule.contains(QLatin1String("##")))
	{
		if (ContentFiltersManager::getCosmeticFiltersMode() != ContentFiltersManager::NoFilters)
		{
			parseStyleSheetRule(rule.split(QLatin1String("##")), m_cosmeticFiltersDomainRules);
		}

		return;
	}

	if (rule.contains(QLatin1String("#@#")))
	{
		if (ContentFiltersManager::getCosmeticFiltersMode() != ContentFiltersManager::NoFilters)
		{
			parseStyleSheetRule(rule.split(QLatin1String("#@#")), m_cosmeticFiltersDomainExceptions);
		}

		return;
	}

	const int optionsSeparator(rule.indexOf(QLatin1Char('$')));
	const QStringList options((optionsSeparator >= 0) ? rule.mid(optionsSeparator + 1).split(QLatin1Char(','), QString::SkipEmptyParts) : QStringList());
	QString line(rule);

	if (optionsSeparator >= 0)
	{
		line = line.left(optionsSeparator);
	}

	if (line.endsWith(QLatin1Char('*')))
	{
		line = line.left(line.length() - 1);
	}

	if (line.startsWith(QLatin1Char('*')))
	{
		line = line.mid(1);
	}

	if (!ContentFiltersManager::areWildcardsEnabled() && line.contains(QLatin1Char('*')))
	{
		return;
	}

	QStringList allowedDomains;
	QStringList blockedDomains;
	RuleOptions ruleOptions;
	RuleMatch ruleMatch(ContainsMatch);
	const bool isException(line.startsWith(QLatin1String("@@")));

	if (isException)
	{
		line = line.mid(2);
	}

	const bool needsDomainCheck(line.startsWith(QLatin1String("||")));

	if (needsDomainCheck)
	{
		line = line.mid(2);
	}

	if (line.startsWith(QLatin1Char('|')))
	{
		ruleMatch = StartMatch;

		line = line.mid(1);
	}

	if (line.endsWith(QLatin1Char('|')))
	{
		ruleMatch = ((ruleMatch == StartMatch) ? ExactMatch : EndMatch);

		line = line.left(line.length() - 1);
	}

	for (int i = 0; i < options.count(); ++i)
	{
		const bool optionException(options.at(i).startsWith(QLatin1Char('~')));
		const QString optionName(optionException ? options.at(i).mid(1) : options.at(i));

		if (m_options.contains(optionName))
		{
			const RuleOption option(m_options.value(optionName));

			if ((!isException || optionException) && (option == ElementHideOption || option == GenericHideOption))
			{
				continue;
			}

			if (!optionException)
			{
				ruleOptions |= option;
			}
			else if (option != WebSocketOption && option != PopupOption)
			{
				ruleOptions |= static_cast<RuleOption>(option * 2);
			}
		}
		else if (optionName.startsWith(QLatin1String("domain")))
		{
			const QStringList parsedDomains(options.at(i).mid(options.at(i).indexOf(QLatin1Char('=')) + 1).split(QLatin1Char('|'), QString::SkipEmptyParts));

			for (int j = 0; j < parsedDomains.count(); ++j)
			{
				if (parsedDomains.at(j).startsWith(QLatin1Char('~')))
				{
					allowedDomains.append(parsedDomains.at(j).mid(1));

					continue;
				}

				blockedDomains.append(parsedDomains.at(j));
			}
		}
		else
		{
			return;
		}
	}

	addRule(new ContentBlockingRule(rule, blockedDomains, allowedDomains, ruleOptions, ruleMatch, isException, needsDomainCheck), line);
}

void AdblockContentFiltersProfile::parseStyleSheetRule(const QStringList &line, QMultiHash<QString, QString> &list) const
{
	const QStringList domains(line.at(0).split(QLatin1Char(',')));

	for (int i = 0; i < domains.count(); ++i)
	{
		list.insert(domains.at(i), line.at(1));
	}
}

void AdblockContentFiltersProfile::addRule(ContentBlockingRule *rule, const QString &ruleString) const
{
	Node *node(m_root);

	for (int i = 0; i < ruleString.length(); ++i)
	{
		const QChar value(ruleString.at(i));
		bool childrenExists(false);

		for (int j = 0; j < node->children.count(); ++j)
		{
			Node *nextNode(node->children.at(j));

			if (nextNode->value == value)
			{
				node = nextNode;

				childrenExists = true;

				break;
			}
		}

		if (!childrenExists)
		{
			Node *newNode(new Node());
			newNode->value = value;

			if (value == QLatin1Char('^'))
			{
				node->children.insert(0, newNode);
			}
			else
			{
				node->children.append(newNode);
			}

			node = newNode;
		}
	}

	node->rules.append(rule);
}

void AdblockContentFiltersProfile::deleteNode(Node *node) const
{
	for (int i = 0; i < node->children.count(); ++i)
	{
		deleteNode(node->children.at(i));
	}

	for (int i = 0; i < node->rules.count(); ++i)
	{
		delete node->rules.at(i);
	}

	delete node;
}

ContentFiltersManager::CheckResult AdblockContentFiltersProfile::checkUrlSubstring(const Node *node, const QString &subString, QString currentRule, NetworkManager::ResourceType resourceType)
{
	ContentFiltersManager::CheckResult result;
	ContentFiltersManager::CheckResult currentResult;

	for (int i = 0; i < subString.length(); ++i)
	{
		const QChar treeChar(subString.at(i));
		bool childrenExists(false);

		currentResult = evaluateRulesInNode(node, currentRule, resourceType);

		if (currentResult.isBlocked)
		{
			result = currentResult;
		}
		else if (currentResult.isException)
		{
			return currentResult;
		}

		for (int j = 0; j < node->children.count(); ++j)
		{
			const Node *nextNode(node->children.at(j));

			if (nextNode->value == QLatin1Char('*'))
			{
				const QString wildcardSubString(subString.mid(i));

				for (int k = 0; k < wildcardSubString.length(); ++k)
				{
					currentResult = checkUrlSubstring(nextNode, wildcardSubString.right(wildcardSubString.length() - k), (currentRule + wildcardSubString.left(k)), resourceType);

					if (currentResult.isBlocked)
					{
						result = currentResult;
					}
					else if (currentResult.isException)
					{
						return currentResult;
					}
				}
			}

			if (nextNode->value == QLatin1Char('^') && !treeChar.isDigit() && !treeChar.isLetter() && !m_separators.contains(treeChar))
			{
				currentResult = checkUrlSubstring(nextNode, subString.mid(i), currentRule, resourceType);

				if (currentResult.isBlocked)
				{
					result = currentResult;
				}
				else if (currentResult.isException)
				{
					return currentResult;
				}
			}

			if (nextNode->value == treeChar)
			{
				node = nextNode;

				childrenExists = true;

				break;
			}
		}

		if (!childrenExists)
		{
			return result;
		}

		currentRule += treeChar;
	}

	currentResult = evaluateRulesInNode(node, currentRule, resourceType);

	if (currentResult.isBlocked)
	{
		result = currentResult;
	}
	else if (currentResult.isException)
	{
		return currentResult;
	}

	for (int i = 0; i < node->children.count(); ++i)
	{
		if (node->children.at(i)->value == QLatin1Char('^'))
		{
			currentResult = evaluateRulesInNode(node, currentRule, resourceType);

			if (currentResult.isBlocked)
			{
				result = currentResult;
			}
			else if (currentResult.isException)
			{
				return currentResult;
			}
		}
	}

	return result;
}

ContentFiltersManager::CheckResult AdblockContentFiltersProfile::checkRuleMatch(const ContentBlockingRule *rule, const QString &currentRule, NetworkManager::ResourceType resourceType) const
{
	switch (rule->ruleMatch)
	{
		case StartMatch:
			if (!m_requestUrl.startsWith(currentRule))
			{
				return {};
			}

			break;
		case EndMatch:
			if (!m_requestUrl.endsWith(currentRule))
			{
				return {};
			}

			break;
		case ExactMatch:
			if (m_requestUrl != currentRule)
			{
				return {};
			}

			break;
		default:
			if (!m_requestUrl.contains(currentRule))
			{
				return {};
			}

			break;
	}

	const QStringList requestSubdomainList(ContentFiltersManager::createSubdomainList(m_requestHost));

	if (rule->needsDomainCheck && !requestSubdomainList.contains(currentRule.left(currentRule.indexOf(m_domainExpression))))
	{
		return {};
	}

	const bool hasBlockedDomains(!rule->blockedDomains.isEmpty());
	const bool hasAllowedDomains(!rule->allowedDomains.isEmpty());
	bool isBlocked(true);

	if (hasBlockedDomains)
	{
		isBlocked = resolveDomainExceptions(m_baseUrlHost, rule->blockedDomains);

		if (!isBlocked)
		{
			return {};
		}
	}

	isBlocked = (hasAllowedDomains ? !resolveDomainExceptions(m_baseUrlHost, rule->allowedDomains) : isBlocked);

	if (rule->ruleOptions.testFlag(ThirdPartyExceptionOption) || rule->ruleOptions.testFlag(ThirdPartyOption))
	{
		if (m_baseUrlHost.isEmpty() || requestSubdomainList.contains(m_baseUrlHost))
		{
			isBlocked = rule->ruleOptions.testFlag(ThirdPartyExceptionOption);
		}
		else if (!hasBlockedDomains && !hasAllowedDomains)
		{
			isBlocked = rule->ruleOptions.testFlag(ThirdPartyOption);
		}
	}

	if (rule->ruleOptions != NoOption)
	{
		QHash<NetworkManager::ResourceType, RuleOption>::const_iterator iterator;

		for (iterator = m_resourceTypes.begin(); iterator != m_resourceTypes.end(); ++iterator)
		{
			const bool supportsException(iterator.value() != WebSocketOption && iterator.value() != PopupOption);

			if (rule->ruleOptions.testFlag(iterator.value()) || (supportsException && rule->ruleOptions.testFlag(static_cast<RuleOption>(iterator.value() * 2))))
			{
				if (resourceType == iterator.key())
				{
					isBlocked = (isBlocked ? rule->ruleOptions.testFlag(iterator.value()) : isBlocked);
				}
				else if (supportsException)
				{
					isBlocked = (isBlocked ? rule->ruleOptions.testFlag(static_cast<RuleOption>(iterator.value() * 2)) : isBlocked);
				}
				else
				{
					isBlocked = false;
				}
			}
		}
	}
	else if (resourceType == NetworkManager::PopupType)
	{
		isBlocked = false;
	}

	if (isBlocked)
	{
		ContentFiltersManager::CheckResult result;
		result.rule = rule->rule;

		if (rule->isException)
		{
			result.isBlocked = false;
			result.isException = true;

			if (rule->ruleOptions.testFlag(ElementHideOption))
			{
				result.comesticFiltersMode = ContentFiltersManager::NoFilters;
			}
			else if (rule->ruleOptions.testFlag(GenericHideOption))
			{
				result.comesticFiltersMode = ContentFiltersManager::DomainOnlyFilters;
			}

			return result;
		}

		result.isBlocked = true;

		return result;
	}

	return {};
}

void AdblockContentFiltersProfile::handleReplyFinished()
{
	m_isUpdating = false;

	if (!m_networkReply)
	{
		return;
	}

	m_networkReply->deleteLater();

	const QByteArray downloadedHeader(m_networkReply->readLine());
	const QByteArray downloadedChecksum(m_networkReply->readLine());
	const QByteArray downloadedData(m_networkReply->readAll());

	if (m_networkReply->error() != QNetworkReply::NoError)
	{
		m_error = DownloadError;

		Console::addMessage(QCoreApplication::translate("main", "Failed to update content blocking profile: %1").arg(m_networkReply->errorString()), Console::OtherCategory, Console::ErrorLevel, getPath());

		return;
	}

	if (downloadedChecksum.contains(QByteArrayLiteral("! Checksum: ")))
	{
		QByteArray checksum(downloadedChecksum);
		const QByteArray verifiedChecksum(QCryptographicHash::hash(downloadedHeader + QString(downloadedData).replace(QRegularExpression(QLatin1String("^.*\n{2,}")), QLatin1String("\n")).toStdString().c_str(), QCryptographicHash::Md5));

		if (verifiedChecksum.toBase64().replace(QByteArrayLiteral("="), QByteArray()) != checksum.replace(QByteArrayLiteral("! Checksum: "), QByteArray()).replace(QByteArrayLiteral("\n"), QByteArray()))
		{
			m_error = ChecksumError;

			Console::addMessage(QCoreApplication::translate("main", "Failed to update content blocking profile: checksum mismatch"), Console::OtherCategory, Console::ErrorLevel, getPath());

			return;
		}
	}

	QDir().mkpath(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking")));

	QSaveFile file(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking/%1.txt")).arg(m_name));

	if (!file.open(QIODevice::WriteOnly))
	{
		m_error = DownloadError;

		Console::addMessage(QCoreApplication::translate("main", "Failed to update content blocking profile: %1").arg(file.errorString()), Console::OtherCategory, Console::ErrorLevel, file.fileName());

		return;
	}

	file.write(downloadedHeader);
	file.write(downloadedChecksum);
	file.write(downloadedData);

	m_lastUpdate = QDateTime::currentDateTimeUtc();

	if (!file.commit())
	{
		Console::addMessage(QCoreApplication::translate("main", "Failed to update content blocking profile: %1").arg(file.errorString()), Console::OtherCategory, Console::ErrorLevel, file.fileName());
	}

	clear();
	loadHeader();

	if (m_wasLoaded)
	{
		loadRules();
	}

	emit profileModified(m_name);
}

void AdblockContentFiltersProfile::setUpdateInterval(int interval)
{
	if (interval != m_updateInterval)
	{
		m_updateInterval = interval;

		emit profileModified(m_name);
	}
}

void AdblockContentFiltersProfile::setUpdateUrl(const QUrl &url)
{
	if (url.isValid() && url != m_updateUrl)
	{
		m_updateUrl = url;
		m_flags |= HasCustomUpdateUrlFlag;

		emit profileModified(m_name);
	}
}

void AdblockContentFiltersProfile::setCategory(ProfileCategory category)
{
	if (category != m_category)
	{
		m_category = category;

		emit profileModified(m_name);
	}
}

void AdblockContentFiltersProfile::setTitle(const QString &title)
{
	if (title != m_title)
	{
		m_title = title;
		m_flags |= HasCustomTitleFlag;

		emit profileModified(m_name);
	}
}

QString AdblockContentFiltersProfile::getName() const
{
	return m_name;
}

QString AdblockContentFiltersProfile::getTitle() const
{
	return (m_title.isEmpty() ? tr("(Unknown)") : m_title);
}

QString AdblockContentFiltersProfile::getPath() const
{
	return SessionsManager::getWritableDataPath(QLatin1String("contentBlocking/%1.txt")).arg(m_name);
}

QDateTime AdblockContentFiltersProfile::getLastUpdate() const
{
	return m_lastUpdate;
}

QUrl AdblockContentFiltersProfile::getUpdateUrl() const
{
	return m_updateUrl;
}

ContentFiltersManager::CheckResult AdblockContentFiltersProfile::evaluateRulesInNode(const Node *node, const QString &currentRule, NetworkManager::ResourceType resourceType) const
{
	ContentFiltersManager::CheckResult result;

	for (int i = 0; i < node->rules.count(); ++i)
	{
		if (node->rules.at(i))
		{
			ContentFiltersManager::CheckResult currentResult(checkRuleMatch(node->rules.at(i), currentRule, resourceType));

			if (currentResult.isBlocked)
			{
				result = currentResult;
			}
			else if (currentResult.isException)
			{
				return currentResult;
			}
		}
	}

	return result;
}

ContentFiltersManager::CheckResult AdblockContentFiltersProfile::checkUrl(const QUrl &baseUrl, const QUrl &requestUrl, NetworkManager::ResourceType resourceType)
{
	ContentFiltersManager::CheckResult result;

	if (!m_wasLoaded && !loadRules())
	{
		return result;
	}

	m_baseUrlHost = baseUrl.host();
	m_requestUrl = requestUrl.url();
	m_requestHost = requestUrl.host();

	if (m_requestUrl.startsWith(QLatin1String("//")))
	{
		m_requestUrl = m_requestUrl.mid(2);
	}

	for (int i = 0; i < m_requestUrl.length(); ++i)
	{
		const ContentFiltersManager::CheckResult currentResult(checkUrlSubstring(m_root, m_requestUrl.right(m_requestUrl.length() - i), {}, resourceType));

		if (currentResult.isBlocked)
		{
			result = currentResult;
		}
		else if (currentResult.isException)
		{
			return currentResult;
		}
	}

	return result;
}

ContentFiltersManager::CosmeticFiltersResult AdblockContentFiltersProfile::getCosmeticFilters(const QStringList &domains, bool isDomainOnly)
{
	if (!m_wasLoaded)
	{
		loadRules();
	}

	ContentFiltersManager::CosmeticFiltersResult result;

	if (!isDomainOnly)
	{
		result.rules = m_cosmeticFiltersRules;
	}

	for (int i = 0; i < domains.count(); ++i)
	{
		result.rules.append(m_cosmeticFiltersDomainRules.values(domains.at(i)));
		result.exceptions.append(m_cosmeticFiltersDomainExceptions.values(domains.at(i)));
	}

	return result;
}

QVector<QLocale::Language> AdblockContentFiltersProfile::getLanguages() const
{
	return m_languages;
}

ContentFiltersProfile::ProfileCategory AdblockContentFiltersProfile::getCategory() const
{
	return m_category;
}

ContentFiltersProfile::ProfileError AdblockContentFiltersProfile::getError() const
{
	return m_error;
}

ContentFiltersProfile::ProfileFlags AdblockContentFiltersProfile::getFlags() const
{
	return m_flags;
}

int AdblockContentFiltersProfile::getUpdateInterval() const
{
	return m_updateInterval;
}

bool AdblockContentFiltersProfile::loadRules()
{
	m_error = NoError;

	if (m_isEmpty && !m_updateUrl.isEmpty())
	{
		update();

		return false;
	}

	m_wasLoaded = true;

	if (m_domainExpression.pattern().isEmpty())
	{
		m_domainExpression = QRegularExpression(QLatin1String("[:\?&/=]"));
		m_domainExpression.optimize();
	}

	QFile file(getPath());
	file.open(QIODevice::ReadOnly | QIODevice::Text);

	QTextStream stream(&file);
	stream.readLine(); // header

	m_root = new Node();

	while (!stream.atEnd())
	{
		parseRuleLine(stream.readLine());
	}

	file.close();

	return true;
}

bool AdblockContentFiltersProfile::update()
{
	if (m_isUpdating)
	{
		return false;
	}

	if (!m_updateUrl.isValid())
	{
		const QString path(getPath());

		m_error = DownloadError;

		if (m_updateUrl.isEmpty())
		{
			Console::addMessage(QCoreApplication::translate("main", "Failed to update content blocking profile, update URL is empty"), Console::OtherCategory, Console::ErrorLevel, path);
		}
		else
		{
			Console::addMessage(QCoreApplication::translate("main", "Failed to update content blocking profile, update URL (%1) is invalid").arg(m_updateUrl.toString()), Console::OtherCategory, Console::ErrorLevel, path);
		}

		return false;
	}

	m_networkReply = NetworkManagerFactory::createRequest(m_updateUrl);

	connect(m_networkReply, &QNetworkReply::finished, this, &AdblockContentFiltersProfile::handleReplyFinished);

	m_isUpdating = true;

	return true;
}

bool AdblockContentFiltersProfile::remove()
{
	const QString path(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking/%1.txt")).arg(m_name));

	if (m_networkReply)
	{
		m_networkReply->abort();
		m_networkReply->deleteLater();
		m_networkReply = nullptr;
	}

	if (QFile::exists(path))
	{
		return QFile::remove(path);
	}

	return true;
}

bool AdblockContentFiltersProfile::resolveDomainExceptions(const QString &url, const QStringList &ruleList) const
{
	for (int i = 0; i < ruleList.count(); ++i)
	{
		if (url.contains(ruleList.at(i)))
		{
			return true;
		}
	}

	return false;
}

bool AdblockContentFiltersProfile::isUpdating() const
{
	return m_isUpdating;
}

}
