#ifndef OTTER_SETTINGSMANAGER_H
#define OTTER_SETTINGSMANAGER_H

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

namespace Otter
{

class SettingsManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(const QString &path, QObject *parent = NULL);
	static void restore(const QString &key);
	static void remove(const QString &key);
	static void setDefaultValue(const QString &key, const QVariant &value);
	static void setValue(const QString &key, const QVariant &value);
	static QString getPath();
	static QVariant getDefaultValue(const QString &key);
	static QVariant getValue(const QString &key, const QVariant &value = QVariant());
	static QStringList getKeys(const QString &pattern = QString());
	static bool contains(const QString &key);

private:
	explicit SettingsManager(const QString &path, QObject *parent = NULL);

	QVariant keyValue(const QString &key);
	QStringList valueKeys();

	static SettingsManager *m_instance;
	static QString m_path;
	static QHash<QString, QVariant> m_defaultSettings;
};

}

#endif
