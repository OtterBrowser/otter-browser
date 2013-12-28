#ifndef OTTER_SETTINGSMANAGER_H
#define OTTER_SETTINGSMANAGER_H

#include <QtCore/QObject>
#include <QtCore/QVariant>

namespace Otter
{

class SettingsManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(const QString &path, QObject *parent = NULL);
	static void registerOption(const QString &key);
	static void setDefaultValue(const QString &key, const QVariant &value);
	static void setValue(const QString &key, const QVariant &value);
	static SettingsManager* getInstance();
	static QString getPath();
	static QVariant getDefaultValue(const QString &key);
	static QVariant getValue(const QString &key);

private:
	explicit SettingsManager(const QString &path, QObject *parent = NULL);

	static SettingsManager *m_instance;
	static QString m_path;
	static QHash<QString, QVariant> m_defaults;

signals:
	void valueChanged(QString key, QVariant value);
};

}

#endif
