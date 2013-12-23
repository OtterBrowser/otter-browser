#include "HistoryManager.h"
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
	optionChanged("Browser/RememberBrowsingHistory");

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

//TODO remove unused entries and VACUUM
	}
}

void HistoryManager::scheduleCleanup()
{
	if (m_cleanupTimer == 0)
	{
		m_cleanupTimer = startTimer(2500);
	}
}

void HistoryManager::clear()
{
	QSqlDatabase database = QSqlDatabase::database("browsingHistory");
	const QString path = SettingsManager::getPath() + "/browsingHistory.sqlite";

	if (database.isValid())
	{
		database.exec("DELETE FROM \"visits\";");
		database.exec("DELETE FROM \"locations\";");
		database.exec("DELETE FROM \"hosts\";");
		database.exec("DELETE FROM \"icons\";");
		database.exec("VACUUM;");
	}
	else if (QFile::exists(path))
	{
		QFile::remove(path);
	}

	emit m_instance->cleared();
}

void HistoryManager::optionChanged(const QString &option)
{
	if (option == "Browser/RememberBrowsingHistory" || option == "Browser/PrivateMode")
	{
		const bool enabled = (SettingsManager::getValue("Browser/RememberBrowsingHistory").toBool() && !SettingsManager::getValue("Browser/PrivateMode").toBool());

		if (enabled && !m_enabled)
		{
			QSqlDatabase database = QSqlDatabase::addDatabase("QSQLITE", "browsingHistory");
			database.setDatabaseName(SettingsManager::getPath() + "/browsingHistory.sqlite");
			database.open();

			if (!database.tables().contains("visits"))
			{
				QFile file(":/schemas/browsingHistory.sql");
				file.open(QIODevice::ReadOnly);

				const QStringList queries = QString(file.readAll()).split(";\n");

				for (int i = 0; i < queries.count(); ++i)
				{
					database.exec(queries.at(i));
				}
			}
		}
		else if (!enabled && m_enabled)
		{
			QSqlDatabase::database("browsingHistory").close();
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
	pixmap.loadFromData(record.field("icon").value().toByteArray());

	HistoryEntry historyEntry;
	historyEntry.url.setScheme(record.field("scheme").value().toString());
	historyEntry.url.setHost(record.field("host").value().toString());
	historyEntry.url.setPath(record.field("path").value().toString());
	historyEntry.title = record.field("title").value().toString();
	historyEntry.time = QDateTime::fromTime_t(record.field("time").value().toInt(), Qt::LocalTime);
	historyEntry.icon = QIcon(pixmap);
	historyEntry.identifier = record.field("id").value().toLongLong();
	historyEntry.visits = record.field("visits").value().toInt();
	historyEntry.typed = record.field("typed").value().toBool();

	return historyEntry;
}

HistoryEntry HistoryManager::getEntry(qint64 entry)
{
	QSqlQuery query(QSqlDatabase::database("browsingHistory"));
	query.prepare("SELECT \"visits\".\"id\", \"visits\".\"title\", \"locations\".\"scheme\", \"locations\".\"path\", \"hosts\".\"host\", \"icons\".\"icon\", \"visits\".\"time\", \"visits\".\"typed\" FROM \"visits\" LEFT JOIN \"locations\" ON \"visits\".\"location\" = \"locations\".\"id\" LEFT JOIN \"hosts\" ON \"locations\".\"host\" = \"hosts\".\"id\" LEFT JOIN \"icons\" ON \"visits\".\"icon\" = \"icons\".\"id\" WHERE \"visits\".\"id\" = ?;");
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
	QSqlQuery query(QSqlDatabase::database("browsingHistory"));
	query.prepare("SELECT \"visits\".\"id\", \"visits\".\"title\", \"locations\".\"scheme\", \"locations\".\"path\", \"hosts\".\"host\", \"icons\".\"icon\", \"visits\".\"time\", \"visits\".\"typed\" FROM \"visits\" LEFT JOIN \"locations\" ON \"visits\".\"location\" = \"locations\".\"id\" LEFT JOIN \"hosts\" ON \"locations\".\"host\" = \"hosts\".\"id\" LEFT JOIN \"icons\" ON \"visits\".\"icon\" = \"icons\".\"id\"" + (typed ? " \"visits\".\"typed\" = 1" : QString()) + " ORDER BY \"visits\".\"time\" DESC;");
	query.exec();

	while (query.next())
	{
		entries.append(getEntry(query.record()));
	}

	return entries;
}

qint64 HistoryManager::getRecord(const QString &table, const QVariantHash &values)
{
	const QStringList keys = values.keys();
	QStringList placeholders;

	for (int i = 0; i < keys.count(); ++i)
	{
		placeholders.append(QString('?'));
	}

	QSqlQuery selectQuery(QSqlDatabase::database("browsingHistory"));
	selectQuery.prepare(QString("SELECT \"id\" FROM \"%1\" WHERE \"%2\" = ?;").arg(table).arg(keys.join("\" = ? AND \"")));

	for (int i = 0; i < keys.count(); ++i)
	{
		selectQuery.bindValue(i, values[keys.at(i)]);
	}

	selectQuery.exec();

	if (selectQuery.first())
	{
		return selectQuery.record().field("id").value().toLongLong();
	}

	QSqlQuery insertQuery(QSqlDatabase::database("browsingHistory"));
	insertQuery.prepare(QString("INSERT INTO \"%1\" (\"%2\") VALUES(%3);").arg(table).arg(keys.join("\", \"")).arg(placeholders.join(", ")));

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
	hostsRecord["host"] = url.host();

	QUrl simplifiedUrl(url);
	simplifiedUrl.setHost(QString());

	QVariantHash locationsRecord;
	locationsRecord["host"] = getRecord("hosts", hostsRecord);
	locationsRecord["scheme"] = url.scheme();
	locationsRecord["path"] = simplifiedUrl.toString(QUrl::RemovePassword | QUrl::RemoveScheme | QUrl::NormalizePathSegments | QUrl::PreferLocalFile | QUrl::FullyDecoded);

	return getRecord("locations", locationsRecord);
}

qint64 HistoryManager::getIcon(const QIcon &icon)
{
	QByteArray data;
	QBuffer buffer(&data);
	buffer.open(QIODevice::WriteOnly);

	icon.pixmap(QSize(16, 16)).save(&buffer, "PNG");

	QVariantHash record;
	record["icon"] = data;

	return getRecord("icons", record);
}

qint64 HistoryManager::addEntry(const QUrl &url, const QString &title, const QIcon &icon, bool typed)
{
	if (!m_enabled || !url.isValid())
	{
		return -1;
	}

	QSqlQuery query(QSqlDatabase::database("browsingHistory"));
	query.prepare("INSERT INTO \"visits\" (\"location\", \"icon\", \"title\", \"time\", \"typed\") VALUES(?, ?, ?, ?, ?);");
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
	QSqlQuery query(QSqlDatabase::database("browsingHistory"));
	query.prepare("UPDATE \"visits\" SET \"location\" = ?, \"icon\" = ?, \"title\" = ? WHERE \"id\" = ?;");
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
	QSqlQuery query(QSqlDatabase::database("browsingHistory"));
	query.prepare("DELETE FROM \"visits\" WHERE \"id\" = ?;");
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

	QSqlQuery query(QSqlDatabase::database("browsingHistory"));
	query.prepare(QString("DELETE FROM \"visits\" WHERE \"id\" IN(%1);").arg(list.join(", ")));
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
