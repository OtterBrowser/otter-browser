#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

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
	static void createInstance(QObject *parent = NULL);
	static void restore(const QString &key);
	static void remove(const QString &key);
	static void setDefaultValue(const QString &key, const QVariant &value);
	static void setValue(const QString &key, const QVariant &value);
	static QVariant getDefaultValue(const QString &key);
	static QVariant getValue(const QString &key, const QVariant &value = QVariant());
	static QStringList getKeys(const QString &pattern = QString());
	static bool contains(const QString &key);

private:
	explicit SettingsManager(QObject *parent = NULL);

	QVariant keyValue(const QString &key);
	QStringList valueKeys();

	static SettingsManager *m_instance;
	static QHash<QString, QVariant> m_defaultSettings;
};

}

#endif
