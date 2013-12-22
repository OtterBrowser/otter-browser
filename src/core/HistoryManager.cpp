#include "HistoryManager.h"
#include "SettingsManager.h"

#include <QtCore/QBuffer>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QTimerEvent>
#include <QtCore/QUrl>
#include <QtGui/QIcon>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlField>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlResult>

namespace Otter
{

HistoryManager* HistoryManager::m_instance = NULL;
bool HistoryManager::m_enabled = true;

HistoryManager::HistoryManager(QObject *parent) : QObject(parent),
	m_cleanupTimer(0)
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

	m_enabled = (SettingsManager::getValue("Browser/RememberBrowsingHistory").toBool() && !SettingsManager::getValue("Browser/PrivateMode").toBool());

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

void HistoryManager::optionChanged(const QString &option)
{
	if (option == "Browser/RememberBrowsingHistory" || option == "Browser/PrivateMode")
	{
		m_enabled = (SettingsManager::getValue("Browser/RememberBrowsingHistory").toBool() && !SettingsManager::getValue("Browser/PrivateMode").toBool());
	}
}

HistoryManager* HistoryManager::getInstance()
{
	return m_instance;
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

	QVariantHash locationsRecord;
	locationsRecord["host"] = getRecord("hosts", hostsRecord);
	locationsRecord["scheme"] = url.scheme();
	locationsRecord["path"] = url.path();

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
	if (!m_enabled)
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

	if (query.first())
	{
		const qint64 entry = query.record().field("id").value().toLongLong();

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

}
