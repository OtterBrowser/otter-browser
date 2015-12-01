/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "HistoryManager.h"
#include "SessionsManager.h"
#include "SettingsManager.h"
#include "Utils.h"

#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QTimerEvent>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlField>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlResult>

namespace Otter
{

HistoryManager* HistoryManager::m_instance = NULL;
HistoryModel* HistoryManager::m_model = NULL;
QStandardItemModel* HistoryManager::m_typedHistoryModel = NULL;
bool HistoryManager::m_isEnabled = false;
bool HistoryManager::m_isStoringFavicons = true;

HistoryManager::HistoryManager(QObject *parent) : QObject(parent),
	m_cleanupTimer(0)
{
	m_dayTimer = startTimer(QTime::currentTime().msecsTo(QTime(23, 59, 59, 999)));

	optionChanged(QLatin1String("History/RememberBrowsing"));
	optionChanged(QLatin1String("History/StoreFavicons"));

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString)));
}

void HistoryManager::createInstance(QObject *parent)
{
	if (!m_instance)
	{
		m_instance = new HistoryManager(parent);
	}
}

void HistoryManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_cleanupTimer)
	{
		killTimer(m_cleanupTimer);

		m_cleanupTimer = 0;

		QSqlDatabase database = QSqlDatabase::database(QLatin1String("browsingHistory"));

		if (!database.isValid())
		{
			return;
		}

		QSqlQuery query(QSqlDatabase::database(QLatin1String("browsingHistory")));
		query.prepare(QLatin1String("SELECT COUNT(*) AS \"amount\" FROM \"visits\";"));
		query.exec();

		if (query.next())
		{
			const int amount = query.record().field(QLatin1String("amount")).value().toInt();

			if (amount > SettingsManager::getValue(QLatin1String("History/BrowsingLimitAmountGlobal")).toInt())
			{
				removeOldEntries();
			}
		}

		database.exec(QLatin1String("DELETE FROM \"icons\" WHERE \"id\" NOT IN(SELECT DISTINCT \"icon\" FROM \"visits\");"));
		database.exec(QLatin1String("DELETE FROM \"locations\" WHERE \"id\" NOT IN(SELECT DISTINCT \"location\" FROM \"visits\");"));
		database.exec(QLatin1String("DELETE FROM \"hosts\" WHERE \"id\" NOT IN(SELECT DISTINCT \"host\" FROM \"locations\");"));
		database.exec(QLatin1String("VACUUM;"));

		m_instance->updateTypedHistoryModel();
	}
	else if (event->timerId() == m_dayTimer)
	{
		killTimer(m_dayTimer);

		removeOldEntries(QDateTime::currentDateTime().addDays(SettingsManager::getValue(QLatin1String("History/BrowsingLimitPeriod")).toInt()));

		emit dayChanged();

		m_dayTimer = startTimer(QTime::currentTime().msecsTo(QTime(23, 59, 59, 999)));
	}
}

void HistoryManager::scheduleCleanup()
{
	if (m_cleanupTimer == 0)
	{
		m_cleanupTimer = startTimer(2500);
	}
}

void HistoryManager::removeOldEntries(const QDateTime &date)
{
	int timestamp = (date.isValid() ? date.toTime_t() : 0);

	if (timestamp == 0)
	{
		QSqlQuery query(QSqlDatabase::database(QLatin1String("browsingHistory")));
		query.prepare(QStringLiteral("SELECT \"visits\".\"time\" FROM \"visits\" ORDER BY \"visits\".\"time\" DESC LIMIT %1, 1;").arg(SettingsManager::getValue(QLatin1String("History/BrowsingLimitAmountGlobal")).toInt()));
		query.exec();

		if (query.next())
		{
			timestamp = query.record().field(QLatin1String("time")).value().toInt();
		}

		if (timestamp == 0)
		{
			return;
		}
	}

	QList<quint64> entries;
	QSqlQuery query(QSqlDatabase::database(QLatin1String("browsingHistory")));
	query.prepare(QLatin1String("SELECT \"visits\".\"id\" FROM \"visits\" WHERE \"visits\".\"time\" <= ?;"));
	query.bindValue(0, timestamp);
	query.exec();

	while (query.next())
	{
		entries.append(query.record().field(QLatin1String("id")).value().toULongLong());
	}

	removeEntries(entries);
}

void HistoryManager::clearHistory(int period)
{
	QSqlDatabase database = QSqlDatabase::database(QLatin1String("browsingHistory"));

	if (period > 0 && !database.isValid())
	{
		database = QSqlDatabase::addDatabase(QLatin1String("QSQLITE"), QLatin1String("browsingHistory"));
		database.setDatabaseName(SessionsManager::getWritableDataPath(QLatin1String("browsingHistory.sqlite")));
		database.open();
		database.exec(QStringLiteral("PRAGMA journal_mode = %1;").arg(SettingsManager::getValue(QLatin1String("Browser/SqliteJournalMode")).toString()));
	}

	const QString path = SessionsManager::getWritableDataPath(QLatin1String("browsingHistory.sqlite"));

	if (database.isValid())
	{
		if (period > 0)
		{
			if (m_model)
			{
				QSqlQuery query(QSqlDatabase::database(QLatin1String("browsingHistory")));
				query.prepare(QLatin1String("SELECT \"visits\".\"id\" FROM \"visits\" WHERE \"visits\".\"time\" >= ?;"));
				query.bindValue(0, (QDateTime::currentDateTime().toTime_t() - (period * 3600)));
				query.exec();

				while (query.next())
				{
					m_model->removeEntry(query.record().field(QLatin1String("id")).value().toULongLong());
				}
			}

			database.exec(QStringLiteral("DELETE FROM \"visits\" WHERE \"time\" >= %1;").arg(QDateTime::currentDateTime().toTime_t() - (period * 3600)));

			m_instance->scheduleCleanup();
		}
		else
		{
			if (m_model)
			{
				m_model->clear();
			}

			database.exec(QLatin1String("DELETE FROM \"visits\";"));
			database.exec(QLatin1String("DELETE FROM \"locations\";"));
			database.exec(QLatin1String("DELETE FROM \"hosts\";"));
			database.exec(QLatin1String("DELETE FROM \"icons\";"));
			database.exec(QLatin1String("VACUUM;"));
		}
	}
	else if (QFile::exists(path))
	{
		QFile::remove(path);
	}

	m_instance->updateTypedHistoryModel();

	emit m_instance->cleared();
}

void HistoryManager::optionChanged(const QString &option)
{
	if (option == QLatin1String("History/RememberBrowsing") || option == QLatin1String("Browser/PrivateMode"))
	{
		const bool enabled = (SettingsManager::getValue(QLatin1String("History/RememberBrowsing")).toBool() && !SettingsManager::getValue(QLatin1String("Browser/PrivateMode")).toBool());

		if (enabled && !m_isEnabled)
		{
			QSqlDatabase database = QSqlDatabase::addDatabase(QLatin1String("QSQLITE"), QLatin1String("browsingHistory"));
			database.setDatabaseName(SessionsManager::getWritableDataPath(QLatin1String("browsingHistory.sqlite")));
			database.open();
			database.exec(QStringLiteral("PRAGMA journal_mode = %1;").arg(SettingsManager::getValue(QLatin1String("Browser/SqliteJournalMode")).toString()));

			if (!database.tables().contains(QLatin1String("visits")))
			{
				QFile file(QLatin1String(":/schemas/browsingHistory.sql"));
				file.open(QIODevice::ReadOnly);

				QTextStream stream(&file);

				while (!stream.atEnd())
				{
					database.exec(stream.readLine());
				}
			}
		}
		else if (!enabled && m_isEnabled)
		{
			QSqlDatabase::database(QLatin1String("browsingHistory")).close();
		}

		m_isEnabled = enabled;
	}
	else if (option == QLatin1String("History/StoreFavicons"))
	{
		m_isStoringFavicons = SettingsManager::getValue(option).toBool();
	}
}

void HistoryManager::updateTypedHistoryModel()
{
	if (!m_typedHistoryModel)
	{
		return;
	}

	m_typedHistoryModel->clear();

	if (!m_model)
	{
		getModel();
	}

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		QStandardItem *item = m_model->item(i, 0);

		if (item && item->data(HistoryModel::TypedInRole).toBool())
		{
			m_typedHistoryModel->appendRow(new QStandardItem(item->icon(), item->data(HistoryModel::UrlRole).toUrl().toDisplayString()));
		}
	}

	emit typedHistoryModelModified();
}

HistoryManager* HistoryManager::getInstance()
{
	return m_instance;
}

HistoryModel* HistoryManager::getModel()
{
	if (!m_model)
	{
		m_model = new HistoryModel(m_instance);

		QSqlQuery query(QSqlDatabase::database(QLatin1String("browsingHistory")));
		query.prepare(QLatin1String("SELECT \"visits\".\"id\", \"visits\".\"title\", \"locations\".\"scheme\", \"locations\".\"path\", \"hosts\".\"host\", \"icons\".\"icon\", \"visits\".\"time\", \"visits\".\"typed\" FROM \"visits\" LEFT JOIN \"locations\" ON \"visits\".\"location\" = \"locations\".\"id\" LEFT JOIN \"hosts\" ON \"locations\".\"host\" = \"hosts\".\"id\" LEFT JOIN \"icons\" ON \"visits\".\"icon\" = \"icons\".\"id\" ORDER BY \"visits\".\"time\" DESC;"));
		query.exec();

		while (query.next())
		{
			const QSqlRecord record = query.record();

			if (record.isEmpty())
			{
				continue;
			}

			QPixmap pixmap;
			pixmap.loadFromData(record.field(QLatin1String("icon")).value().toByteArray());

			QUrl url(record.field(QLatin1String("path")).value().toString());
			url.setHost(record.field(QLatin1String("host")).value().toString());
			url.setScheme(record.field(QLatin1String("scheme")).value().toString());

			m_model->addEntry(url, record.field(QLatin1String("title")).value().toString(), QIcon(pixmap), record.field(QLatin1String("typed")).value().toBool(), QDateTime::fromTime_t(record.field(QLatin1String("time")).value().toInt(), Qt::LocalTime), record.field(QLatin1String("id")).value().toULongLong());
		}
	}

	return m_model;
}

QStandardItemModel* HistoryManager::getTypedHistoryModel()
{
	if (!m_typedHistoryModel && m_instance)
	{
		m_typedHistoryModel = new QStandardItemModel(m_instance);

		m_instance->updateTypedHistoryModel();
	}

	return m_typedHistoryModel;
}

QIcon HistoryManager::getIcon(const QUrl &url)
{
	if (url.scheme() == QLatin1String("about"))
	{
		if (url.path() == QLatin1String("bookmarks"))
		{
			return Utils::getIcon(QLatin1String("bookmarks"));
		}

		if (url.path() == QLatin1String("cache"))
		{
			return Utils::getIcon(QLatin1String("cache"));
		}

		if (url.path() == QLatin1String("config"))
		{
			return Utils::getIcon(QLatin1String("configuration"));
		}

		if (url.path() == QLatin1String("cookies"))
		{
			return Utils::getIcon(QLatin1String("cookies"));
		}

		if (url.path() == QLatin1String("history"))
		{
			return Utils::getIcon(QLatin1String("view-history"));
		}

		if (url.path() == QLatin1String("notes"))
		{
			return Utils::getIcon(QLatin1String("notes"));
		}

		if (url.path() == QLatin1String("transfers"))
		{
			return Utils::getIcon(QLatin1String("transfers"));
		}

		return Utils::getIcon(QLatin1String("text-html"));
	}

	quint64 location = getLocation(url, false);

	if (location == 0)
	{
		location = getLocation(url.adjusted(QUrl::RemovePath | QUrl::RemoveFragment), false);
	}

	if (location > 0)
	{
		QSqlQuery query(QSqlDatabase::database(QLatin1String("browsingHistory")));
		query.prepare(QLatin1String("SELECT \"icons\".\"icon\" FROM \"visits\" LEFT JOIN \"icons\" ON \"visits\".\"icon\" = \"icons\".\"id\" WHERE \"visits\".\"location\" = ? LIMIT 1;"));
		query.bindValue(0, location);
		query.exec();

		if (query.first())
		{
			QPixmap pixmap;
			pixmap.loadFromData(query.record().field(QLatin1String("icon")).value().toByteArray());

			if (!pixmap.isNull())
			{
				return QIcon(pixmap);
			}
		}
	}

	return Utils::getIcon(QLatin1String("text-html"));
}

HistoryEntryItem* HistoryManager::getEntry(quint64 identifier)
{
	if (!m_model)
	{
		getModel();
	}

	return m_model->getEntry(identifier);
}

QList<HistoryModel::HistoryEntryMatch> HistoryManager::findEntries(const QString &prefix)
{
	if (!m_model)
	{
		getModel();
	}

	return m_model->findEntries(prefix);
}

quint64 HistoryManager::getRecord(const QLatin1String &table, const QVariantHash &values, bool canCreate)
{
	const QStringList keys = values.keys();
	QStringList placeholders;

	for (int i = 0; i < keys.count(); ++i)
	{
		placeholders.append(QString('?'));
	}

	QSqlQuery selectQuery(QSqlDatabase::database(QLatin1String("browsingHistory")));
	selectQuery.prepare(QStringLiteral("SELECT \"id\" FROM \"%1\" WHERE \"%2\" = ?;").arg(table).arg(keys.join(QLatin1String("\" = ? AND \""))));

	for (int i = 0; i < keys.count(); ++i)
	{
		selectQuery.bindValue(i, values[keys.at(i)]);
	}

	selectQuery.exec();

	if (selectQuery.first())
	{
		return selectQuery.record().field(QLatin1String("id")).value().toULongLong();
	}

	if (!canCreate)
	{
		return 0;
	}

	QSqlQuery insertQuery(QSqlDatabase::database(QLatin1String("browsingHistory")));
	insertQuery.prepare(QStringLiteral("INSERT INTO \"%1\" (\"%2\") VALUES(%3);").arg(table).arg(keys.join(QLatin1String("\", \""))).arg(placeholders.join(QLatin1String(", "))));

	for (int i = 0; i < keys.count(); ++i)
	{
		insertQuery.bindValue(i, values[keys.at(i)]);
	}

	insertQuery.exec();

	return insertQuery.lastInsertId().toULongLong();
}

quint64 HistoryManager::getLocation(const QUrl &url, bool canCreate)
{
	QVariantHash hostsRecord;
	hostsRecord[QLatin1String("host")] = url.host();

	QUrl simplifiedUrl(url);
	simplifiedUrl.setScheme(QString());
	simplifiedUrl.setHost(QString());

	QVariantHash locationsRecord;
	locationsRecord[QLatin1String("host")] = getRecord(QLatin1String("hosts"), hostsRecord, canCreate);
	locationsRecord[QLatin1String("scheme")] = url.scheme();
	locationsRecord[QLatin1String("path")] = simplifiedUrl.toString(QUrl::RemovePassword | QUrl::NormalizePathSegments);

	return getRecord(QLatin1String("locations"), locationsRecord, canCreate);
}

quint64 HistoryManager::getIcon(const QIcon &icon, bool canCreate)
{
	if (!m_isStoringFavicons)
	{
		return 0;
	}

	QByteArray data;
	QBuffer buffer(&data);
	buffer.open(QIODevice::WriteOnly);

	icon.pixmap(QSize(16, 16)).save(&buffer, "PNG");

	QVariantHash record;
	record[QLatin1String("icon")] = data;

	return getRecord(QLatin1String("icons"), record, canCreate);
}

quint64 HistoryManager::addEntry(const QUrl &url, const QString &title, const QIcon &icon, bool isTypedIn)
{
	if (!m_isEnabled || !url.isValid() || !SettingsManager::getValue(QLatin1String("History/RememberBrowsing"), url).toBool())
	{
		return 0;
	}

	const QDateTime date(QDateTime::currentDateTime());
	QSqlQuery query(QSqlDatabase::database(QLatin1String("browsingHistory")));
	query.prepare(QLatin1String("INSERT INTO \"visits\" (\"location\", \"icon\", \"title\", \"time\", \"typed\") VALUES(?, ?, ?, ?, ?);"));
	query.bindValue(0, getLocation(url));
	query.bindValue(1, getIcon(icon));
	query.bindValue(2, title);
	query.bindValue(3, date.toTime_t());
	query.bindValue(4, isTypedIn);
	query.exec();

	if (!query.lastInsertId().isNull())
	{
		const quint64 identifier = query.lastInsertId().toULongLong();

		if (m_model)
		{
			m_model->addEntry(url, title, icon, isTypedIn, date, identifier);
		}

		if (isTypedIn)
		{
			m_instance->updateTypedHistoryModel();
		}

		emit m_instance->entryAdded(identifier);

		return identifier;
	}

	return 0;
}

bool HistoryManager::hasEntry(const QUrl &url)
{
	return (m_isEnabled && getLocation(url, false) > 0);
}

bool HistoryManager::updateEntry(quint64 identifier, const QUrl &url, const QString &title, const QIcon &icon)
{
	if (!m_isEnabled || !url.isValid())
	{
		return false;
	}

	if (!SettingsManager::getValue(QLatin1String("History/RememberBrowsing"), url).toBool())
	{
		removeEntry(identifier);

		return false;
	}

	QSqlQuery query(QSqlDatabase::database(QLatin1String("browsingHistory")));
	query.prepare(QLatin1String("UPDATE \"visits\" SET \"location\" = ?, \"icon\" = ?, \"title\" = ? WHERE \"id\" = ?;"));
	query.bindValue(0, getLocation(url));
	query.bindValue(1, getIcon(icon));
	query.bindValue(2, title);
	query.bindValue(3, identifier);
	query.exec();

	const bool success = (query.numRowsAffected() > 0);

	if (success)
	{
		if (m_model)
		{
			HistoryEntryItem *item = m_model->getEntry(identifier);

			if (item)
			{
				item->setData(url, HistoryModel::UrlRole);
				item->setData(title, HistoryModel::TitleRole);
				item->setIcon(icon);
			}
		}

		m_instance->scheduleCleanup();

		emit m_instance->entryUpdated(identifier);
	}

	return success;
}

bool HistoryManager::removeEntry(quint64 identifier)
{
	if (!m_isEnabled)
	{
		return false;
	}

	QSqlQuery query(QSqlDatabase::database(QLatin1String("browsingHistory")));
	query.prepare(QLatin1String("DELETE FROM \"visits\" WHERE \"id\" = ?;"));
	query.bindValue(0, identifier);
	query.exec();

	const bool success = (query.numRowsAffected() > 0);

	if (success)
	{
		if (m_model)
		{
			m_model->removeEntry(identifier);
		}

		m_instance->scheduleCleanup();

		emit m_instance->entryRemoved(identifier);
	}

	return success;
}

bool HistoryManager::removeEntries(const QList<quint64> &identifiers)
{
	if (!m_isEnabled)
	{
		return false;
	}

	QStringList list;

	for (int i = 0; i < identifiers.count(); ++i)
	{
		if (identifiers.at(i) > 0)
		{
			list.append(QString::number(identifiers.at(i)));
		}
	}

	if (list.isEmpty())
	{
		return false;
	}

	QSqlQuery query(QSqlDatabase::database(QLatin1String("browsingHistory")));
	query.prepare(QStringLiteral("DELETE FROM \"visits\" WHERE \"id\" IN(%1);").arg(list.join(QLatin1String(", "))));
	query.exec();

	const bool success = (query.numRowsAffected() > 0);

	if (success)
	{
		m_instance->scheduleCleanup();

		for (int i = 0; i < identifiers.count(); ++i)
		{
			if (m_model)
			{
				m_model->removeEntry(identifiers.at(i));
			}

			emit m_instance->entryRemoved(identifiers.at(i));
		}
	}

	return success;
}

}
