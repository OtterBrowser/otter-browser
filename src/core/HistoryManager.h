#ifndef OTTER_HISTORYMANAGER_H
#define OTTER_HISTORYMANAGER_H

#include <QtCore/QObject>
#include <QtCore/QDateTime>
#include <QtCore/QUrl>
#include <QtGui/QIcon>
#include <QtSql/QSqlRecord>

namespace Otter
{

struct HistoryEntry
{
	QUrl url;
	QString title;
	QDateTime time;
	QIcon icon;
	qint64 identifier;
	int visits;
	bool typed;

	HistoryEntry() : identifier(-1), visits(0), typed(false) {}
};

class HistoryManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static void clearHistory(int period = 0);
	static HistoryManager* getInstance();
	static HistoryEntry getEntry(qint64 entry);
	static QList<HistoryEntry> getEntries(bool typed = false);
	static qint64 addEntry(const QUrl &url, const QString &title, const QIcon &icon, bool typed = false);
	static bool updateEntry(qint64 entry, const QUrl &url, const QString &title, const QIcon &icon);
	static bool removeEntry(qint64 entry);
	static bool removeEntries(const QList<qint64> &entries);

protected:
	void timerEvent(QTimerEvent *event);
	void scheduleCleanup();
	void removeOldEntries(const QDateTime &date = QDateTime());
	static HistoryEntry getEntry(const QSqlRecord &record);
	static qint64 getRecord(const QLatin1String &table, const QVariantHash &values);
	static qint64 getLocation(const QUrl &url);
	static qint64 getIcon(const QIcon &icon);

protected slots:
	void optionChanged(const QString &option);

private:
	explicit HistoryManager(QObject *parent = NULL);

	int m_cleanupTimer;
	int m_dayTimer;

	static HistoryManager *m_instance;
	static bool m_enabled;

signals:
	void cleared();
	void entryAdded(qint64 entry);
	void entryUpdated(qint64 entry);
	void entryRemoved(qint64 entry);
	void dayChanged();
};

}

#endif
