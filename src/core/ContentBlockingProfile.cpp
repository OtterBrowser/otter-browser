/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2010 - 2014 David Rosca <nowrep@gmail.com>
* Copyright (C) 2014 - 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ContentBlockingProfile.h"
#include "Console.h"
#include "NetworkManager.h"
#include "NetworkManagerFactory.h"
#include "SessionsManager.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QTextStream>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace Otter
{

QVector<QChar> ContentBlockingProfile::m_separators({QLatin1Char('_'), QLatin1Char('-'), QLatin1Char('.'), QLatin1Char('%')});
QHash<QString, ContentBlockingProfile::RuleOption> ContentBlockingProfile::m_options({{QLatin1String("third-party"), ThirdPartyOption}, {QLatin1String("stylesheet"), StyleSheetOption}, {QLatin1String("image"), ImageOption}, {QLatin1String("script"), ScriptOption}, {QLatin1String("object"), ObjectOption}, {QLatin1String("object-subrequest"), ObjectSubRequestOption}, {QLatin1String("object_subrequest"), ObjectSubRequestOption}, {QLatin1String("subdocument"), SubDocumentOption}, {QLatin1String("xmlhttprequest"), XmlHttpRequestOption}, {QLatin1String("websocket"), WebSocketOption}, {QLatin1String("elemhide"), ElementHideOption}, {QLatin1String("generichide"), GenericHideOption}});
QHash<NetworkManager::ResourceType, ContentBlockingProfile::RuleOption> ContentBlockingProfile::m_resourceTypes({{NetworkManager::ImageType, ImageOption}, {NetworkManager::ScriptType, ScriptOption}, {NetworkManager::StyleSheetType, StyleSheetOption}, {NetworkManager::ObjectType, ObjectOption}, {NetworkManager::XmlHttpRequestType, XmlHttpRequestOption}, {NetworkManager::SubFrameType, SubDocumentOption}, {NetworkManager::ObjectSubrequestType, ObjectSubRequestOption}, {NetworkManager::WebSocketType, WebSocketOption}});

ContentBlockingProfile::ContentBlockingProfile(const QString &name, const QString &title, const QUrl &updateUrl, const QDateTime lastUpdate, const QStringList languages, int updateInterval, const ProfileCategory &category, const ProfileFlags &flags, QObject *parent) : QObject(parent),
	m_root(nullptr),
	m_networkReply(nullptr),
	m_name(name),
	m_title(title),
	m_updateUrl(updateUrl),
	m_lastUpdate(lastUpdate),
	m_languages({QLocale::AnyLanguage}),
	m_category(category),
	m_flags(flags),
	m_updateInterval(updateInterval),
	m_isUpdating(false),
	m_isEmpty(true),
	m_wasLoaded(false)
{
	if (!languages.isEmpty())
	{
		m_languages.clear();

		for (int i = 0; i < languages.count(); ++i)
		{
			m_languages.append(QLocale(languages.at(i)).language());
		}
	}

	loadHeader(getPath());
}

void ContentBlockingProfile::clear()
{
	if (!m_wasLoaded)
	{
		return;
	}

	if (m_root)
	{
		QtConcurrent::run(this, &ContentBlockingProfile::deleteNode, m_root);
	}

	m_styleSheet.clear();
	m_styleSheetWhiteList.clear();
	m_styleSheetBlackList.clear();

	m_wasLoaded = false;
}

void ContentBlockingProfile::loadHeader(const QString &path)
{
	QFile file(path);

	if (!file.exists())
	{
		return;
	}

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		Console::addMessage(QCoreApplication::translate("main", "Failed to open content blocking profile file: %1").arg(file.errorString()), Console::OtherCategory, Console::ErrorLevel, file.fileName());

		return;
	}

	QTextStream stream(&file);

	if (!stream.readLine().trimmed().startsWith(QLatin1String("[Adblock Plus"), Qt::CaseInsensitive))
	{
		Console::addMessage(QCoreApplication::translate("main", "Failed to load content blocking profile file: invalid header"), Console::OtherCategory, Console::ErrorLevel, file.fileName());

		file.close();

		return;
	}

	while (!stream.atEnd())
	{
		QString line(stream.readLine().trimmed());

		if (!line.startsWith(QLatin1Char('!')))
		{
			m_isEmpty = false;

			break;
		}

		if (line.startsWith(QLatin1String("! Title: ")) && !m_flags.testFlag(HasCustomTitleFlag))
		{
			m_title = line.remove(QLatin1String("! Title: "));

			continue;
		}
	}

	file.close();

	if (!m_isUpdating && m_updateInterval > 0 && (!m_lastUpdate.isValid() || m_lastUpdate.daysTo(QDateTime::currentDateTime()) > m_updateInterval))
	{
		downloadRules();
	}
}

void ContentBlockingProfile::parseRuleLine(QString line)
{
	if (line.indexOf(QLatin1Char('!')) == 0 || line.isEmpty())
	{
		return;
	}

	if (line.startsWith(QLatin1String("##")))
	{
		if (ContentBlockingManager::getCosmeticFiltersMode() == ContentBlockingManager::AllFiltersMode)
		{
			m_styleSheet.append(line.mid(2));
		}

		return;
	}

	if (line.contains(QLatin1String("##")))
	{
		if (ContentBlockingManager::getCosmeticFiltersMode() != ContentBlockingManager::NoFiltersMode)
		{
			parseStyleSheetRule(line.split(QLatin1String("##")), m_styleSheetBlackList);
		}

		return;
	}

	if (line.contains(QLatin1String("#@#")))
	{
		if (ContentBlockingManager::getCosmeticFiltersMode() != ContentBlockingManager::NoFiltersMode)
		{
			parseStyleSheetRule(line.split(QLatin1String("#@#")), m_styleSheetWhiteList);
		}

		return;
	}

	const QString rule(line);
	QStringList options;
	const int optionSeparator(line.indexOf(QLatin1Char('$')));

	if (optionSeparator >= 0)
	{
		options = line.mid(optionSeparator + 1).split(QLatin1Char(','), QString::SkipEmptyParts);

		line = line.left(optionSeparator);
	}

	if (line.endsWith(QLatin1Char('*')))
	{
		line = line.left(line.length() - 1);
	}

	if (line.startsWith(QLatin1Char('*')))
	{
		line = line.mid(1);
	}

	if (!ContentBlockingManager::areWildcardsEnabled() && line.contains(QLatin1Char('*')))
	{
		return;
	}

	QStringList allowedDomains;
	QStringList blockedDomains;
	RuleOptions ruleOptions;
	RuleMatch ruleMatch(ContainsMatch);
	bool isException(false);
	bool needsDomainCheck(false);

	if (line.startsWith(QLatin1String("@@")))
	{
		line = line.mid(2);

		isException = true;
	}

	if (line.startsWith(QLatin1String("||")))
	{
		line = line.mid(2);

		needsDomainCheck = true;
	}

	if (line.startsWith(QLatin1Char('|')))
	{
		ruleMatch = StartMatch;

		line = line.mid(1);
	}

	if (line.endsWith(QLatin1Char('|')))
	{
		ruleMatch = (ruleMatch == StartMatch ? ExactMatch : EndMatch);

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
			else if (!optionException)
			{
				ruleOptions |= option;
			}
			else if (option != WebSocketOption)
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

void ContentBlockingProfile::parseStyleSheetRule(const QStringList &line, QMultiHash<QString, QString> &list)
{
	const QStringList domains(line.at(0).split(QLatin1Char(',')));

	for (int i = 0; i < domains.count(); ++i)
	{
		list.insert(domains.at(i), line.at(1));
	}
}

void ContentBlockingProfile::addRule(ContentBlockingRule *rule, const QString &ruleString)
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

void ContentBlockingProfile::deleteNode(Node *node)
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

ContentBlockingManager::CheckResult ContentBlockingProfile::checkUrlSubstring(Node *node, const QString &subString, QString currentRule, NetworkManager::ResourceType resourceType)
{
	ContentBlockingManager::CheckResult result;
	ContentBlockingManager::CheckResult currentResult;

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
			Node *nextNode(node->children.at(j));

			if (nextNode->value == QLatin1Char('*'))
			{
				const QString wildcardSubString(subString.mid(i));

				for (int k = 0; k < wildcardSubString.length(); ++k)
				{
					currentResult = checkUrlSubstring(nextNode, wildcardSubString.right(wildcardSubString.length() - k), currentRule + wildcardSubString.left(k), resourceType);

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

ContentBlockingManager::CheckResult ContentBlockingProfile::checkRuleMatch(ContentBlockingRule *rule, const QString &currentRule, NetworkManager::ResourceType resourceType)
{
	switch (rule->ruleMatch)
	{
		case StartMatch:
			if (!m_requestUrl.startsWith(currentRule))
			{
				return ContentBlockingManager::CheckResult();
			}

			break;
		case EndMatch:
			if (!m_requestUrl.endsWith(currentRule))
			{
				return ContentBlockingManager::CheckResult();
			}

			break;
		case ExactMatch:
			if (m_requestUrl != currentRule)
			{
				return ContentBlockingManager::CheckResult();
			}

			break;
		default:
			if (!m_requestUrl.contains(currentRule))
			{
				return ContentBlockingManager::CheckResult();
			}

			break;
	}

	const QStringList requestSubdomainList(ContentBlockingManager::createSubdomainList(m_requestHost));

	if (rule->needsDomainCheck && !requestSubdomainList.contains(currentRule.left(currentRule.indexOf(m_domainExpression))))
	{
		return ContentBlockingManager::CheckResult();
	}

	const bool hasBlockedDomains(!rule->blockedDomains.isEmpty());
	const bool hasAllowedDomains(!rule->allowedDomains.isEmpty());
	bool isBlocked(hasBlockedDomains ? resolveDomainExceptions(m_baseUrlHost, rule->blockedDomains) : true);
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
			const bool supportsException(iterator.value() != WebSocketOption);

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
			}
		}
	}

	if (isBlocked)
	{
		ContentBlockingManager::CheckResult result;
		result.rule = rule->rule;

		if (rule->isException)
		{
			result.isBlocked = false;
			result.isException = true;

			if (rule->ruleOptions.testFlag(ElementHideOption))
			{
				result.comesticFiltersMode = ContentBlockingManager::NoFiltersMode;
			}
			else if (rule->ruleOptions.testFlag(GenericHideOption))
			{
				result.comesticFiltersMode = ContentBlockingManager::DomainOnlyFiltersMode;
			}

			return result;
		}

		result.isBlocked = true;

		return result;
	}

	return ContentBlockingManager::CheckResult();
}

void ContentBlockingProfile::replyFinished()
{
	m_isUpdating = false;

	if (!m_networkReply)
	{
		return;
	}

	m_networkReply->deleteLater();

	const QByteArray downloadedDataHeader(m_networkReply->readLine());
	const QByteArray downloadedDataChecksum(m_networkReply->readLine());
	const QByteArray downloadedData(m_networkReply->readAll());

	if (m_networkReply->error() != QNetworkReply::NoError || !downloadedDataHeader.trimmed().startsWith(QByteArray("[Adblock Plus")))
	{
		Console::addMessage(QCoreApplication::translate("main", "Failed to update content blocking profile: %1").arg(m_networkReply->errorString()), Console::OtherCategory, Console::ErrorLevel, getPath());

		return;
	}

	if (downloadedDataChecksum.contains(QByteArray("! Checksum: ")))
	{
		QByteArray checksum(downloadedDataChecksum);
		const QByteArray verifiedChecksum(QCryptographicHash::hash(downloadedDataHeader + QString(downloadedData).replace(QRegExp(QLatin1String("^*\n{2,}")), QLatin1String("\n")).toStdString().c_str(), QCryptographicHash::Md5));

		if (verifiedChecksum.toBase64().replace(QByteArray("="), QByteArray()) != checksum.replace(QByteArray("! Checksum: "), QByteArray()).replace(QByteArray("\n"), QByteArray()))
		{
			Console::addMessage(QCoreApplication::translate("main", "Failed to update content blocking profile: checksum mismatch"), Console::OtherCategory, Console::ErrorLevel, getPath());

			return;
		}
	}

	QDir().mkpath(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking")));

	QFile file(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking/%1.txt")).arg(m_name));

	if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate))
	{
		Console::addMessage(QCoreApplication::translate("main", "Failed to update content blocking profile: %1").arg(file.errorString()), Console::OtherCategory, Console::ErrorLevel, file.fileName());

		return;
	}

	file.write(downloadedDataHeader);
	file.write(downloadedDataChecksum);
	file.write(downloadedData);
	file.close();

	m_lastUpdate = QDateTime::currentDateTime();

	if (file.error() != QFile::NoError)
	{
// TODO
	}

	clear();
	loadHeader(getPath());

	if (m_wasLoaded)
	{
		loadRules();
	}

	emit profileModified(m_name);
}

void ContentBlockingProfile::setUpdateInterval(int interval)
{
	if (interval != m_updateInterval)
	{
		m_updateInterval = interval;

		emit profileModified(m_name);
	}
}

void ContentBlockingProfile::setUpdateUrl(const QUrl &url)
{
	if (url.isValid() && url != m_updateUrl)
	{
		m_updateUrl = url;
		m_flags |= HasCustomUpdateUrlFlag;

		emit profileModified(m_name);
	}
}

void ContentBlockingProfile::setCategory(const ProfileCategory &category)
{
	if (category != m_category)
	{
		m_category = category;

		emit profileModified(m_name);
	}
}

void ContentBlockingProfile::setTitle(const QString &title)
{
	if (title != m_title)
	{
		m_title = title;
		m_flags |= HasCustomTitleFlag;

		emit profileModified(m_name);
	}
}

QString ContentBlockingProfile::getName() const
{
	return m_name;
}

QString ContentBlockingProfile::getTitle() const
{
	return (m_title.isEmpty() ? tr("(Unknown)") : m_title);
}

QString ContentBlockingProfile::getPath() const
{
	return SessionsManager::getWritableDataPath(QLatin1String("contentBlocking/%1.txt")).arg(m_name);
}

QDateTime ContentBlockingProfile::getLastUpdate() const
{
	return m_lastUpdate;
}

QUrl ContentBlockingProfile::getUpdateUrl() const
{
	return m_updateUrl;
}

ContentBlockingManager::CheckResult ContentBlockingProfile::checkUrl(const QUrl &baseUrl, const QUrl &requestUrl, NetworkManager::ResourceType resourceType)
{
	ContentBlockingManager::CheckResult result;

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
		ContentBlockingManager::CheckResult currentResult(checkUrlSubstring(m_root, m_requestUrl.right(m_requestUrl.length() - i), QString(), resourceType));

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

QStringList ContentBlockingProfile::getStyleSheet()
{
	if (!m_wasLoaded)
	{
		loadRules();
	}

	return m_styleSheet;
}

QStringList ContentBlockingProfile::getStyleSheetBlackList(const QString &domain)
{
	if (!m_wasLoaded)
	{
		loadRules();
	}

	return m_styleSheetBlackList.values(domain);
}

QStringList ContentBlockingProfile::getStyleSheetWhiteList(const QString &domain)
{
	if (!m_wasLoaded)
	{
		loadRules();
	}

	return m_styleSheetWhiteList.values(domain);
}

QVector<QLocale::Language> ContentBlockingProfile::getLanguages() const
{
	return m_languages;
}

ContentBlockingProfile::ProfileCategory ContentBlockingProfile::getCategory() const
{
	return m_category;
}

ContentBlockingProfile::ProfileFlags ContentBlockingProfile::getFlags() const
{
	return m_flags;
}

int ContentBlockingProfile::getUpdateInterval() const
{
	return m_updateInterval;
}

bool ContentBlockingProfile::downloadRules()
{
	if (m_isUpdating)
	{
		return false;
	}

	if (!m_updateUrl.isValid())
	{
		const QString path(getPath());

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

	QNetworkRequest request(m_updateUrl);
	request.setHeader(QNetworkRequest::UserAgentHeader, NetworkManagerFactory::getUserAgent());

	m_networkReply = NetworkManagerFactory::getNetworkManager()->get(request);

	connect(m_networkReply, SIGNAL(finished()), this, SLOT(replyFinished()));

	m_isUpdating = true;

	return true;
}

ContentBlockingManager::CheckResult ContentBlockingProfile::evaluateRulesInNode(Node *node, const QString &currentRule, NetworkManager::ResourceType resourceType)
{
	ContentBlockingManager::CheckResult result;

	for (int i = 0; i < node->rules.count(); ++i)
	{
		if (node->rules.at(i))
		{
			ContentBlockingManager::CheckResult currentResult(checkRuleMatch(node->rules.at(i), currentRule, resourceType));

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

bool ContentBlockingProfile::loadRules()
{
	if (m_isEmpty && !m_updateUrl.isEmpty())
	{
		downloadRules();

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

bool ContentBlockingProfile::resolveDomainExceptions(const QString &url, const QStringList &ruleList)
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

}
