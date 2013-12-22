#ifndef OTTER_HISTORYMANAGER_H
#define OTTER_HISTORYMANAGER_H

#include <QtCore/QObject>

namespace Otter
{

class HistoryManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static HistoryManager* getInstance();
	static qint64 addEntry(const QUrl &url, const QString &title, const QIcon &icon, bool typed = false);
	static bool updateEntry(qint64 entry, const QUrl &url, const QString &title, const QIcon &icon);
	static bool removeEntry(qint64 entry);

protected:
	void timerEvent(QTimerEvent *event);
	void scheduleCleanup();
	static qint64 getRecord(const QString &table, const QVariantHash &values);
	static qint64 getLocation(const QUrl &url);
	static qint64 getIcon(const QIcon &icon);

protected slots:
	void optionChanged(const QString &option);

private:
	explicit HistoryManager(QObject *parent = NULL);

	int m_cleanupTimer;

	static HistoryManager *m_instance;
	static bool m_enabled;

signals:
	void entryAdded(qint64 entry);
	void entryUpdated(qint64 entry);
	void entryRemoved(qint64 entry);
};

}

#endif
