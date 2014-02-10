/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtCore/QTimerEvent>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlField>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlResult>

namespace Otter
{

HistoryManager* HistoryManager::m_instance = NULL;
bool HistoryManager::m_enabled = false;

HistoryManager::HistoryManager(QObject *parent) : QObject(parent),
	m_cleanupTimer(0)
{
	m_dayTimer = startTimer(QTime::currentTime().msecsTo(QTime(23, 59, 59, 999)));

	optionChanged(QLatin1String("History/RememberBrowsing"));

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString)));
}

void HistoryManager::createInstance(QObject *parent)
{
	m_instance = new HistoryManager(parent);
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

	QList<qint64> entries;
	QSqlQuery query(QSqlDatabase::database(QLatin1String("browsingHistory")));
	query.prepare(QLatin1String("SELECT \"visits\".\"id\" FROM \"visits\" WHERE \"visits\".\"time\" <= ?;"));
	query.bindValue(0, timestamp);
	query.exec();

	while (query.next())
	{
		entries.append(query.record().field("id").value().toLongLong());
	}

	removeEntries(entries);
}

void HistoryManager::clearHistory(int period)
{
	QSqlDatabase database = QSqlDatabase::database(QLatin1String("browsingHistory"));

	if (period > 0 && !database.isValid())
	{
		database = QSqlDatabase::addDatabase(QLatin1String("QSQLITE"), QLatin1String("browsingHistory"));
		database.setDatabaseName(SessionsManager::getProfilePath() + QLatin1String("/browsingHistory.sqlite"));
		database.open();
	}

	const QString path = SessionsManager::getProfilePath() + QLatin1String("/browsingHistory.sqlite");

	if (database.isValid())
	{
		if (period > 0)
		{
			database.exec(QStringLiteral("DELETE FROM \"visits\" WHERE \"time\" >= %1;").arg(QDateTime::currentDateTime().toTime_t() - (period * 3600)));

			m_instance->scheduleCleanup();
		}
		else
		{
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

	emit m_instance->cleared();
}

void HistoryManager::optionChanged(const QString &option)
{
	if (option == QLatin1String("History/RememberBrowsing") || option == QLatin1String("Browser/PrivateMode"))
	{
		const bool enabled = (SettingsManager::getValue(QLatin1String("History/RememberBrowsing")).toBool() && !SettingsManager::getValue(QLatin1String("Browser/PrivateMode")).toBool());

		if (enabled && !m_enabled)
		{
			QSqlDatabase database = QSqlDatabase::addDatabase(QLatin1String("QSQLITE"), QLatin1String("browsingHistory"));
			database.setDatabaseName(SessionsManager::getProfilePath() + QLatin1String("/browsingHistory.sqlite"));
			database.open();

			if (!database.tables().contains(QLatin1String("visits")))
			{
				QFile file(QLatin1String(":/schemas/browsingHistory.sql"));
				file.open(QIODevice::ReadOnly);

				const QStringList queries = QString(file.readAll()).split(QLatin1String(";\n"));

				for (int i = 0; i < queries.count(); ++i)
				{
					database.exec(queries.at(i));
				}
			}
		}
		else if (!enabled && m_enabled)
		{
			QSqlDatabase::database(QLatin1String("browsingHistory")).close();
		}

		m_enabled = enabled;
	}
}

HistoryManager* HistoryManager::getInstance()
{
	return m_instance;
}

HistoryEntry HistoryManager::getEntry(const QSqlRecord &record)
{
	if (record.isEmpty())
	{
		return HistoryEntry();
	}

	QPixmap pixmap;
	pixmap.loadFromData(record.field(QLatin1String("icon")).value().toByteArray());

	HistoryEntry historyEntry;
	historyEntry.url.setScheme(record.field(QLatin1String("scheme")).value().toString());
	historyEntry.url.setHost(record.field(QLatin1String("host")).value().toString());
	historyEntry.url.setPath(record.field(QLatin1String("path")).value().toString());
	historyEntry.title = record.field(QLatin1String("title")).value().toString();
	historyEntry.time = QDateTime::fromTime_t(record.field(QLatin1String("time")).value().toInt(), Qt::LocalTime);
	historyEntry.icon = QIcon(pixmap);
	historyEntry.identifier = record.field(QLatin1String("id")).value().toLongLong();
	historyEntry.visits = record.field(QLatin1String("visits")).value().toInt();
	historyEntry.typed = record.field(QLatin1String("typed")).value().toBool();

	return historyEntry;
}

HistoryEntry HistoryManager::getEntry(qint64 entry)
{
	QSqlQuery query(QSqlDatabase::database(QLatin1String("browsingHistory")));
	query.prepare(QLatin1String("SELECT \"visits\".\"id\", \"visits\".\"title\", \"locations\".\"scheme\", \"locations\".\"path\", \"hosts\".\"host\", \"icons\".\"icon\", \"visits\".\"time\", \"visits\".\"typed\" FROM \"visits\" LEFT JOIN \"locations\" ON \"visits\".\"location\" = \"locations\".\"id\" LEFT JOIN \"hosts\" ON \"locations\".\"host\" = \"hosts\".\"id\" LEFT JOIN \"icons\" ON \"visits\".\"icon\" = \"icons\".\"id\" WHERE \"visits\".\"id\" = ?;"));
	query.bindValue(0, entry);
	query.exec();

	if (query.first())
	{
		return getEntry(query.record());
	}

	return HistoryEntry();
}

QList<HistoryEntry> HistoryManager::getEntries(bool typed)
{
	QList<HistoryEntry> entries;
	QSqlQuery query(QSqlDatabase::database(QLatin1String("browsingHistory")));
	query.prepare(QLatin1String("SELECT \"visits\".\"id\", \"visits\".\"title\", \"locations\".\"scheme\", \"locations\".\"path\", \"hosts\".\"host\", \"icons\".\"icon\", \"visits\".\"time\", \"visits\".\"typed\" FROM \"visits\" LEFT JOIN \"locations\" ON \"visits\".\"location\" = \"locations\".\"id\" LEFT JOIN \"hosts\" ON \"locations\".\"host\" = \"hosts\".\"id\" LEFT JOIN \"icons\" ON \"visits\".\"icon\" = \"icons\".\"id\"") + (typed ? QLatin1String(" \"visits\".\"typed\" = 1") : QString()) + QLatin1String(" ORDER BY \"visits\".\"time\" DESC;"));
	query.exec();

	while (query.next())
	{
		entries.append(getEntry(query.record()));
	}

	return entries;
}

qint64 HistoryManager::getRecord(const QLatin1String &table, const QVariantHash &values)
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
		return selectQuery.record().field(QLatin1String("id")).value().toLongLong();
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

qint64 HistoryManager::getLocation(const QUrl &url)
{
	QVariantHash hostsRecord;
	hostsRecord[QLatin1String("host")] = url.host();

	QUrl simplifiedUrl(url);
	simplifiedUrl.setScheme(QString());
	simplifiedUrl.setHost(QString());

	QVariantHash locationsRecord;
	locationsRecord[QLatin1String("host")] = getRecord(QLatin1String("hosts"), hostsRecord);
	locationsRecord[QLatin1String("scheme")] = url.scheme();
	locationsRecord[QLatin1String("path")] = simplifiedUrl.toString(QUrl::RemovePassword | QUrl::NormalizePathSegments);

	return getRecord(QLatin1String("locations"), locationsRecord);
}

qint64 HistoryManager::getIcon(const QIcon &icon)
{
	QByteArray data;
	QBuffer buffer(&data);
	buffer.open(QIODevice::WriteOnly);

	icon.pixmap(QSize(16, 16)).save(&buffer, "PNG");

	QVariantHash record;
	record[QLatin1String("icon")] = data;

	return getRecord(QLatin1String("icons"), record);
}

qint64 HistoryManager::addEntry(const QUrl &url, const QString &title, const QIcon &icon, bool typed)
{
	if (!m_enabled || !url.isValid())
	{
		return -1;
	}

	QSqlQuery query(QSqlDatabase::database(QLatin1String("browsingHistory")));
	query.prepare(QLatin1String("INSERT INTO \"visits\" (\"location\", \"icon\", \"title\", \"time\", \"typed\") VALUES(?, ?, ?, ?, ?);"));
	query.bindValue(0, getLocation(url));
	query.bindValue(1, getIcon(icon));
	query.bindValue(2, title);
	query.bindValue(3, QDateTime::currentDateTime().toTime_t());
	query.bindValue(4, typed);
	query.exec();

	if (!query.lastInsertId().isNull())
	{
		const qint64 entry = query.lastInsertId().toLongLong();

		emit m_instance->entryAdded(entry);

		return entry;
	}

	return -1;
}

bool HistoryManager::updateEntry(qint64 entry, const QUrl &url, const QString &title, const QIcon &icon)
{
	QSqlQuery query(QSqlDatabase::database(QLatin1String("browsingHistory")));
	query.prepare(QLatin1String("UPDATE \"visits\" SET \"location\" = ?, \"icon\" = ?, \"title\" = ? WHERE \"id\" = ?;"));
	query.bindValue(0, getLocation(url));
	query.bindValue(1, getIcon(icon));
	query.bindValue(2, title);
	query.bindValue(3, entry);
	query.exec();

	const bool success = (query.numRowsAffected() > 0);

	if (success)
	{
		m_instance->scheduleCleanup();

		emit m_instance->entryUpdated(entry);
	}

	return success;
}

bool HistoryManager::removeEntry(qint64 entry)
{
	QSqlQuery query(QSqlDatabase::database(QLatin1String("browsingHistory")));
	query.prepare(QLatin1String("DELETE FROM \"visits\" WHERE \"id\" = ?;"));
	query.bindValue(0, entry);
	query.exec();

	const bool success = (query.numRowsAffected() > 0);

	if (success)
	{
		m_instance->scheduleCleanup();

		emit m_instance->entryRemoved(entry);
	}

	return success;
}

bool HistoryManager::removeEntries(const QList<qint64> &entries)
{
	QStringList list;

	for (int i = 0; i < entries.count(); ++i)
	{
		if (entries.at(i) >= 0)
		{
			list.append(QString::number(entries.at(i)));
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

		for (int i = 0; i < entries.count(); ++i)
		{
			emit m_instance->entryRemoved(entries.at(i));
		}
	}

	return success;
}

}
