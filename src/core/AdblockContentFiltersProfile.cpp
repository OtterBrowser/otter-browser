/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2010 - 2014 David Rosca <nowrep@gmail.com>
* Copyright (C) 2014 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "Job.h"
#include "SessionsManager.h"
#include "../ui/ContentBlockingProfileDialog.h"

#include <QtCore/QBuffer>
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QSaveFile>
#include <QtCore/QTextStream>
#include <QtCore/QThreadPool>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>

namespace Otter
{

QHash<QString, AdblockContentFiltersProfile::RuleOption> AdblockContentFiltersProfile::m_options({{QLatin1String("third-party"), ThirdPartyOption}, {QLatin1String("stylesheet"), StyleSheetOption}, {QLatin1String("image"), ImageOption}, {QLatin1String("script"), ScriptOption}, {QLatin1String("object"), ObjectOption}, {QLatin1String("object-subrequest"), ObjectSubRequestOption}, {QLatin1String("object_subrequest"), ObjectSubRequestOption}, {QLatin1String("subdocument"), SubDocumentOption}, {QLatin1String("xmlhttprequest"), XmlHttpRequestOption}, {QLatin1String("websocket"), WebSocketOption}, {QLatin1String("popup"), PopupOption}, {QLatin1String("elemhide"), ElementHideOption}, {QLatin1String("generichide"), GenericHideOption}});
QHash<NetworkManager::ResourceType, AdblockContentFiltersProfile::RuleOption> AdblockContentFiltersProfile::m_resourceTypes({{NetworkManager::ImageType, ImageOption}, {NetworkManager::ScriptType, ScriptOption}, {NetworkManager::StyleSheetType, StyleSheetOption}, {NetworkManager::ObjectType, ObjectOption}, {NetworkManager::XmlHttpRequestType, XmlHttpRequestOption}, {NetworkManager::SubFrameType, SubDocumentOption},{NetworkManager::PopupType, PopupOption}, {NetworkManager::ObjectSubrequestType, ObjectSubRequestOption}, {NetworkManager::WebSocketType, WebSocketOption}});

AdblockContentFiltersProfile::AdblockContentFiltersProfile(const ContentFiltersProfile::ProfileSummary &summary, const QStringList &languages, ContentFiltersProfile::ProfileFlags flags, QObject *parent) : ContentFiltersProfile(parent),
	m_root(nullptr),
	m_dataFetchJob(nullptr),
	m_summary(summary),
	m_error(NoError),
	m_flags(flags),
	m_wasLoaded(false)
{
	if (!languages.isEmpty())
	{
		m_languages.reserve(languages.count());

		for (int i = 0; i < languages.count(); ++i)
		{
			const QLocale::Language language(QLocale(languages.at(i)).language());

			if (language != QLocale::AnyLanguage && !m_languages.contains(language))
			{
				m_languages.append(language);
			}
		}
	}

	if (m_languages.isEmpty())
	{
		m_languages = {QLocale::AnyLanguage};
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
		QThreadPool::globalInstance()->start([&]()
		{
			deleteNode(m_root);
		});
	}

	m_cosmeticFiltersRules.clear();
	m_cosmeticFiltersDomainExceptions.clear();
	m_cosmeticFiltersDomainRules.clear();

	m_wasLoaded = false;
}

void AdblockContentFiltersProfile::loadHeader()
{
	const QString path(getPath());

	if (!QFile::exists(path))
	{
		return;
	}

	QFile file(path);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		raiseError(QCoreApplication::translate("main", "Failed to open content blocking profile file: %1").arg(file.errorString()), ReadError);

		return;
	}

	const HeaderInformation information(loadHeader(&file));

	file.close();

	if (information.hasError())
	{
		raiseError(information.errorString, information.error);

		return;
	}

	if (!m_flags.testFlag(HasCustomTitleFlag) && !information.title.isEmpty())
	{
		m_summary.title = information.title;
	}

	if (!m_dataFetchJob && m_summary.updateInterval > 0 && (!m_summary.lastUpdate.isValid() || m_summary.lastUpdate.daysTo(QDateTime::currentDateTimeUtc()) > m_summary.updateInterval))
	{
		update();
	}
}

void AdblockContentFiltersProfile::parseRuleLine(const QString &rule)
{
	if (rule.isEmpty() || rule.startsWith(QLatin1Char('!')))
	{
		return;
	}

	const bool areCosmeticFiltersEnabled(m_summary.cosmeticFiltersMode != ContentFiltersManager::NoFilters);

	if (rule.startsWith(QLatin1String("##")))
	{
		if (m_summary.cosmeticFiltersMode == ContentFiltersManager::AllFilters)
		{
			m_cosmeticFiltersRules.append(rule.mid(2));
		}

		return;
	}

	if (rule.contains(QLatin1String("##")))
	{
		if (areCosmeticFiltersEnabled)
		{
			m_cosmeticFiltersDomainRules += parseStyleSheetRule(rule.split(QLatin1String("##")));
		}

		return;
	}

	if (rule.contains(QLatin1String("#@#")))
	{
		if (areCosmeticFiltersEnabled)
		{
			m_cosmeticFiltersDomainExceptions += parseStyleSheetRule(rule.split(QLatin1String("#@#")));
		}

		return;
	}

	const int separatorIndex(rule.indexOf(QLatin1Char('$')));
	const bool hasSeparator(separatorIndex >= 0);
	const QStringList options(hasSeparator ? rule.mid(separatorIndex + 1).split(QLatin1Char(','), Qt::SkipEmptyParts) : QStringList());
	QString line(rule);

	if (hasSeparator)
	{
		line = line.left(separatorIndex);
	}

	if (line.endsWith(QLatin1Char('*')))
	{
		line = line.left(line.length() - 1);
	}

	if (line.startsWith(QLatin1Char('*')))
	{
		line = line.mid(1);
	}

	if (!m_summary.areWildcardsEnabled && line.contains(QLatin1Char('*')))
	{
		return;
	}

	Node::Rule *definition(new Node::Rule());
	definition->rule = rule;
	definition->isException = line.startsWith(QLatin1String("@@"));

	if (definition->isException)
	{
		line = line.mid(2);
	}

	definition->needsDomainCheck = line.startsWith(QLatin1String("||"));

	if (definition->needsDomainCheck)
	{
		line = line.mid(2);
	}

	if (line.startsWith(QLatin1Char('|')))
	{
		definition->ruleMatch = StartMatch;

		line = line.mid(1);
	}

	if (line.endsWith(QLatin1Char('|')))
	{
		definition->ruleMatch = ((definition->ruleMatch == StartMatch) ? ExactMatch : EndMatch);

		line = line.left(line.length() - 1);
	}

	for (int i = 0; i < options.count(); ++i)
	{
		const QString option(options.at(i));
		const bool isOptionException(option.startsWith(QLatin1Char('~')));
		const QString optionName(isOptionException ? option.mid(1) : option);

		if (m_options.contains(optionName))
		{
			const RuleOption ruleOption(m_options.value(optionName));

			if ((!definition->isException || isOptionException) && (ruleOption == ElementHideOption || ruleOption == GenericHideOption))
			{
				continue;
			}

			if (!isOptionException)
			{
				definition->ruleOptions |= ruleOption;
			}
			else if (ruleOption != WebSocketOption && ruleOption != PopupOption)
			{
				definition->ruleExceptions |= ruleOption;
			}
		}
		else if (optionName.startsWith(QLatin1String("domain")))
		{
			const QStringList parsedDomains(option.mid(option.indexOf(QLatin1Char('=')) + 1).split(QLatin1Char('|'), Qt::SkipEmptyParts));

			for (int j = 0; j < parsedDomains.count(); ++j)
			{
				const QString parsedDomain(parsedDomains.at(j));

				if (parsedDomain.startsWith(QLatin1Char('~')))
				{
					definition->allowedDomains.append(parsedDomain.mid(1));
				}
				else
				{
					definition->blockedDomains.append(parsedDomain);
				}
			}
		}
		else
		{
			return;
		}
	}

	Node *node(m_root);

	for (int i = 0; i < line.length(); ++i)
	{
		const QChar value(line.at(i));
		bool hasChildren(false);

		for (int j = 0; j < node->children.count(); ++j)
		{
			Node *nextNode(node->children.at(j));

			if (nextNode->value == value)
			{
				node = nextNode;

				hasChildren = true;

				break;
			}
		}

		if (!hasChildren)
		{
			Node *newNode(new Node());
			newNode->value = value;

			if (value == QLatin1Char('^'))
			{
				node->children.insert(node->children.begin(), newNode);
			}
			else
			{
				node->children.append(newNode);
			}

			node = newNode;
		}
	}

	node->rules.append(definition);
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

QMultiHash<QString, QString> AdblockContentFiltersProfile::parseStyleSheetRule(const QStringList &line)
{
	QMultiHash<QString, QString> list;
	const QStringList domains(line.at(0).split(QLatin1Char(',')));

	for (int i = 0; i < domains.count(); ++i)
	{
		list.insert(domains.at(i), line.at(1));
	}

	return list;
}

ContentFiltersManager::CheckResult AdblockContentFiltersProfile::checkUrlSubstring(const Node *node, const QString &substring, QString currentRule, const Request &request) const
{
	ContentFiltersManager::CheckResult result;
	ContentFiltersManager::CheckResult currentResult;

	for (int i = 0; i < substring.length(); ++i)
	{
		const QChar character(substring.at(i));
		bool hasChildren(false);

		currentResult = evaluateNodeRules(node, currentRule, request);

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
				const QString wildcardSubString(substring.mid(i));

				for (int k = 0; k < wildcardSubString.length(); ++k)
				{
					currentResult = checkUrlSubstring(nextNode, wildcardSubString.right(wildcardSubString.length() - k), (currentRule + wildcardSubString.left(k)), request);

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

			if (nextNode->value == QLatin1Char('^') && !character.isDigit() && !character.isLetter() && character != QLatin1Char('_') && character != QLatin1Char('-') && character != QLatin1Char('.') && character != QLatin1Char('%'))
			{
				currentResult = checkUrlSubstring(nextNode, substring.mid(i), currentRule, request);

				if (currentResult.isBlocked)
				{
					result = currentResult;
				}
				else if (currentResult.isException)
				{
					return currentResult;
				}
			}

			if (nextNode->value == character)
			{
				node = nextNode;

				hasChildren = true;

				break;
			}
		}

		if (!hasChildren)
		{
			return result;
		}

		currentRule += character;
	}

	currentResult = evaluateNodeRules(node, currentRule, request);

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
		if (node->children.at(i)->value != QLatin1Char('^'))
		{
			continue;
		}

		currentResult = evaluateNodeRules(node, currentRule, request);

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

ContentFiltersManager::CheckResult AdblockContentFiltersProfile::checkRuleMatch(const Node::Rule *rule, const QString &currentRule, const Request &request) const
{
	switch (rule->ruleMatch)
	{
		case StartMatch:
			if (!request.requestUrl.startsWith(currentRule))
			{
				return {};
			}

			break;
		case EndMatch:
			if (!request.requestUrl.endsWith(currentRule))
			{
				return {};
			}

			break;
		case ExactMatch:
			if (request.requestUrl != currentRule)
			{
				return {};
			}

			break;
		default:
			if (!request.requestUrl.contains(currentRule))
			{
				return {};
			}

			break;
	}

	const QStringList requestSubdomainList(ContentFiltersManager::createSubdomainList(request.requestHost));

	if (rule->needsDomainCheck && !requestSubdomainList.contains(currentRule.left(currentRule.indexOf(m_domainExpression))))
	{
		return {};
	}

	const bool hasBlockedDomains(!rule->blockedDomains.isEmpty());
	const bool hasAllowedDomains(!rule->allowedDomains.isEmpty());
	bool isBlocked(true);

	if (hasBlockedDomains)
	{
		isBlocked = domainContains(request.baseHost, rule->blockedDomains);

		if (!isBlocked)
		{
			return {};
		}
	}

	isBlocked = (hasAllowedDomains ? !domainContains(request.baseHost, rule->allowedDomains) : isBlocked);

	if (rule->ruleOptions.testFlag(ThirdPartyOption) || rule->ruleExceptions.testFlag(ThirdPartyOption))
	{
		if (request.baseHost.isEmpty() || requestSubdomainList.contains(request.baseHost))
		{
			isBlocked = rule->ruleExceptions.testFlag(ThirdPartyOption);
		}
		else if (!hasBlockedDomains && !hasAllowedDomains)
		{
			isBlocked = rule->ruleOptions.testFlag(ThirdPartyOption);
		}
	}

	if (rule->ruleOptions != NoOption || rule->ruleExceptions != NoOption)
	{
		QHash<NetworkManager::ResourceType, RuleOption>::const_iterator iterator;

		for (iterator = m_resourceTypes.constBegin(); iterator != m_resourceTypes.constEnd(); ++iterator)
		{
			const bool supportsException(iterator.value() != WebSocketOption && iterator.value() != PopupOption);

			if (!rule->ruleOptions.testFlag(iterator.value()) && !(supportsException && rule->ruleExceptions.testFlag(iterator.value())))
			{
				continue;
			}

			if (request.resourceType == iterator.key())
			{
				isBlocked = (isBlocked ? rule->ruleOptions.testFlag(iterator.value()) : isBlocked);
			}
			else if (supportsException)
			{
				isBlocked = (isBlocked ? rule->ruleExceptions.testFlag(iterator.value()) : isBlocked);
			}
			else
			{
				isBlocked = false;
			}
		}
	}
	else if (request.resourceType == NetworkManager::PopupType)
	{
		isBlocked = false;
	}

	if (!isBlocked)
	{
		return {};
	}

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

void AdblockContentFiltersProfile::raiseError(const QString &message, ProfileError error)
{
	m_error = error;

	Console::addMessage(message, Console::OtherCategory, Console::ErrorLevel, getPath());

	emit profileModified();
}

void AdblockContentFiltersProfile::handleJobFinished(bool isSuccess)
{
	if (!m_dataFetchJob)
	{
		return;
	}

	QIODevice *device(m_dataFetchJob->getData());

	m_dataFetchJob->deleteLater();
	m_dataFetchJob = nullptr;

	if (!isSuccess)
	{
		raiseError(QCoreApplication::translate("main", "Failed to update content blocking profile: %1").arg(device ? device->errorString() : tr("Download failure")), DownloadError);

		return;
	}

	QBuffer buffer;
	buffer.setData(device->readAll());
	buffer.open(QIODevice::ReadOnly | QIODevice::Text);

	const HeaderInformation information(loadHeader(&buffer));

	buffer.reset();

	if (information.hasError())
	{
		raiseError(information.errorString, information.error);

		return;
	}

	Utils::ensureDirectoryExists(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking")));

	QSaveFile file(getPath());

	if (!file.open(QIODevice::WriteOnly))
	{
		raiseError(QCoreApplication::translate("main", "Failed to update content blocking profile: %1").arg(file.errorString()), DownloadError);

		return;
	}

	file.write(buffer.data());

	m_summary.lastUpdate = QDateTime::currentDateTimeUtc();

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

	emit profileModified();
}

void AdblockContentFiltersProfile::setProfileSummary(const ContentFiltersProfile::ProfileSummary &summary)
{
	const bool needsReload(summary.cosmeticFiltersMode != m_summary.cosmeticFiltersMode || summary.areWildcardsEnabled != m_summary.areWildcardsEnabled);

	if (summary.title != m_summary.title)
	{
		m_flags |= HasCustomTitleFlag;
	}
	else if (!needsReload && summary.updateUrl == m_summary.updateUrl && summary.updateInterval == m_summary.updateInterval && summary.category == m_summary.category)
	{
		return;
	}

	m_summary = summary;

	if (needsReload)
	{
		clear();
	}

	emit profileModified();
}

QString AdblockContentFiltersProfile::getName() const
{
	return m_summary.name;
}

QString AdblockContentFiltersProfile::getTitle() const
{
	return (m_summary.title.isEmpty() ? tr("(Unknown)") : m_summary.title);
}

QString AdblockContentFiltersProfile::getPath() const
{
	return SessionsManager::getWritableDataPath(QLatin1String("contentBlocking/%1.txt")).arg(m_summary.name);
}

QDateTime AdblockContentFiltersProfile::getLastUpdate() const
{
	return m_summary.lastUpdate;
}

QUrl AdblockContentFiltersProfile::getUpdateUrl() const
{
	return m_summary.updateUrl;
}

ContentFiltersProfile::ProfileSummary AdblockContentFiltersProfile::getProfileSummary() const
{
	return m_summary;
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
		const QString domain(domains.at(i));

		result.rules.append(m_cosmeticFiltersDomainRules.values(domain));
		result.exceptions.append(m_cosmeticFiltersDomainExceptions.values(domain));
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

	const Request request(baseUrl, requestUrl, resourceType);

	for (int i = 0; i < request.requestUrl.length(); ++i)
	{
		const ContentFiltersManager::CheckResult currentResult(checkUrlSubstring(m_root, request.requestUrl.right(request.requestUrl.length() - i), {}, request));

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

ContentFiltersManager::CheckResult AdblockContentFiltersProfile::evaluateNodeRules(const Node *node, const QString &currentRule, const Request &request) const
{
	ContentFiltersManager::CheckResult result;

	for (int i = 0; i < node->rules.count(); ++i)
	{
		Node::Rule *rule(node->rules.at(i));

		if (!rule)
		{
			continue;
		}

		ContentFiltersManager::CheckResult currentResult(checkRuleMatch(rule, currentRule, request));

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

AdblockContentFiltersProfile::HeaderInformation AdblockContentFiltersProfile::loadHeader(QIODevice *rulesDevice)
{
	HeaderInformation information;
	QTextStream stream(rulesDevice);
	const QString header(stream.readLine());

	if (!header.contains(QLatin1String("[Adblock"), Qt::CaseInsensitive))
	{
		information.errorString = QCoreApplication::translate("main", "Failed to update content blocking profile: invalid header");
		information.error = ParseError;

		return information;
	}

	int lineNumber(1);

	while (!stream.atEnd())
	{
		const QString line(stream.readLine().trimmed());

		if (line.startsWith(QLatin1String("! Title: ")))
		{
			information.title = line.section(QLatin1Char(':'), 1).trimmed();

			break;
		}

		if (lineNumber > 50)
		{
			break;
		}

		++lineNumber;
	}

	return information;
}

QHash<AdblockContentFiltersProfile::RuleType, quint32> AdblockContentFiltersProfile::loadRulesInformation(const ContentFiltersProfile::ProfileSummary &summary, QIODevice *rulesDevice)
{
	QHash<RuleType, quint32> information({{AnyRule, 0}, {ActiveRule, 0}, {CosmeticRule, 0}, {WildcardRule, 0}});
	QTextStream stream(rulesDevice);
	stream.readLine();

	while (!stream.atEnd())
	{
		const QString line(stream.readLine().trimmed());

		if (line.isEmpty() || line.startsWith(QLatin1Char('!')))
		{
			continue;
		}

		++information[AnyRule];

		if (line.startsWith(QLatin1String("##")))
		{
			++information[CosmeticRule];

			if (summary.cosmeticFiltersMode == ContentFiltersManager::AllFilters)
			{
				++information[ActiveRule];
			}

			continue;
		}

		if (line.contains(QLatin1String("##")))
		{
			++information[CosmeticRule];

			if (summary.cosmeticFiltersMode != ContentFiltersManager::NoFilters)
			{
				++information[ActiveRule];
			}

			continue;
		}

		if (line.contains(QLatin1String("#@#")))
		{
			++information[CosmeticRule];

			if (summary.cosmeticFiltersMode != ContentFiltersManager::NoFilters)
			{
				++information[ActiveRule];
			}

			continue;
		}

		if (line.contains(QLatin1Char('*')))
		{
			++information[WildcardRule];

			if (summary.areWildcardsEnabled)
			{
				++information[ActiveRule];
			}

			continue;
		}

		++information[ActiveRule];
	}

	return information;
}

QVector<QLocale::Language> AdblockContentFiltersProfile::getLanguages() const
{
	return m_languages;
}

ContentFiltersProfile::ProfileCategory AdblockContentFiltersProfile::getCategory() const
{
	return m_summary.category;
}

ContentFiltersManager::CosmeticFiltersMode AdblockContentFiltersProfile::getCosmeticFiltersMode() const
{
	return m_summary.cosmeticFiltersMode;
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
	return m_summary.updateInterval;
}

int AdblockContentFiltersProfile::getUpdateProgress() const
{
	return (m_dataFetchJob ? m_dataFetchJob->getProgress() : -1);
}

bool AdblockContentFiltersProfile::create(const ContentFiltersProfile::ProfileSummary &summary, QIODevice *rulesDevice, bool canOverwriteExisting)
{
	const QString path(SessionsManager::getWritableDataPath(QStringLiteral("contentBlocking/%1.txt")).arg(summary.name));

	if (SessionsManager::isReadOnly() || (!canOverwriteExisting && QFile::exists(path)))
	{
		Console::addMessage(QCoreApplication::translate("main", "Failed to create a content blocking profile: %1").arg(tr("File already exists")), Console::OtherCategory, Console::ErrorLevel, path);

		return false;
	}

	if (rulesDevice)
	{
		Utils::ensureDirectoryExists(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking")));

		QFile file(path);

		if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			Console::addMessage(QCoreApplication::translate("main", "Failed to create a content blocking profile: %1").arg(file.errorString()), Console::OtherCategory, Console::ErrorLevel, file.fileName());

			return false;
		}

		file.write(rulesDevice->readAll());
		file.close();
	}

	ProfileFlags flags(NoFlags);

	if (!summary.title.isEmpty())
	{
		flags |= HasCustomTitleFlag;
	}

	AdblockContentFiltersProfile *profile(new AdblockContentFiltersProfile(summary, {}, flags, ContentFiltersManager::getInstance()));

	ContentFiltersManager::addProfile(profile);

	if (!rulesDevice && summary.updateUrl.isValid())
	{
		profile->update();
	}

	return true;
}

bool AdblockContentFiltersProfile::create(const QUrl &url, bool canOverwriteExisting)
{
	if (!canOverwriteExisting && ContentFiltersManager::getProfile(url))
	{
		QMessageBox::critical(QApplication::activeWindow(), tr("Error"), tr("Profile with this address already exists."), QMessageBox::Close);

		return false;
	}

	if (QMessageBox::question(QApplication::activeWindow(), tr("Question"), tr("Do you want to add content blocking profile from this URL?\n\n%1").arg(url.toString()), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
	{
		return false;
	}

	ContentFiltersProfile::ProfileSummary profileSummary;
	profileSummary.updateUrl = url;

	ContentBlockingProfileDialog dialog(profileSummary, {}, QApplication::activeWindow());

	if (dialog.exec() == QDialog::Rejected)
	{
		return false;
	}

	profileSummary = dialog.getProfile();

	QFile file(dialog.getRulesPath());

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QMessageBox::critical(QApplication::activeWindow(), tr("Error"), tr("Failed to create content blocking profile file."), QMessageBox::Close);

		return false;
	}

	profileSummary.name = Utils::createIdentifier(QFileInfo(url.path()).baseName(), ContentFiltersManager::getProfileNames());

	const bool result(create(profileSummary, &file, canOverwriteExisting));

	if (!result)
	{
		QMessageBox::critical(QApplication::activeWindow(), tr("Error"), tr("Failed to create content blocking profile file."), QMessageBox::Close);
	}

	file.close();
	file.remove();

	return result;
}

bool AdblockContentFiltersProfile::loadRules()
{
	const QString path(getPath());

	m_error = NoError;

	if (!QFile::exists(path) && !m_summary.updateUrl.isEmpty())
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

	QFile file(path);
	file.open(QIODevice::ReadOnly | QIODevice::Text);

	QTextStream stream(&file);
	stream.readLine(); // skip header

	m_root = new Node();

	while (!stream.atEnd())
	{
		parseRuleLine(stream.readLine());
	}

	file.close();

	return true;
}

bool AdblockContentFiltersProfile::update(const QUrl &url)
{
	if (m_dataFetchJob || thread() != QThread::currentThread())
	{
		return false;
	}

	const QUrl updateUrl(url.isValid() ? url : m_summary.updateUrl);

	if (!updateUrl.isValid())
	{
		if (updateUrl.isEmpty())
		{
			raiseError(QCoreApplication::translate("main", "Failed to update content blocking profile, update URL is empty"), DownloadError);
		}
		else
		{
			raiseError(QCoreApplication::translate("main", "Failed to update content blocking profile, update URL (%1) is invalid").arg(updateUrl.toString()), DownloadError);
		}

		return false;
	}

	m_dataFetchJob = new DataFetchJob(updateUrl, this);

	connect(m_dataFetchJob, &Job::jobFinished, this, &AdblockContentFiltersProfile::handleJobFinished);
	connect(m_dataFetchJob, &Job::progressChanged, this, &AdblockContentFiltersProfile::updateProgressChanged);

	m_dataFetchJob->start();

	emit profileModified();

	return true;
}

bool AdblockContentFiltersProfile::remove()
{
	const QString path(getPath());

	if (m_dataFetchJob)
	{
		m_dataFetchJob->cancel();
		m_dataFetchJob->deleteLater();
		m_dataFetchJob = nullptr;
	}

	if (QFile::exists(path))
	{
		return QFile::remove(path);
	}

	return true;
}

bool AdblockContentFiltersProfile::domainContains(const QString &host, const QStringList &domains) const
{
	for (int i = 0; i < domains.count(); ++i)
	{
		if (host.contains(domains.at(i)))
		{
			return true;
		}
	}

	return false;
}

bool AdblockContentFiltersProfile::areWildcardsEnabled() const
{
	return m_summary.areWildcardsEnabled;
}

bool AdblockContentFiltersProfile::isUpdating() const
{
	return (m_dataFetchJob != nullptr);
}

}
