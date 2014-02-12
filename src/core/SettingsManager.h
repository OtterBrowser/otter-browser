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
	static QVariant getDefaultValue(const QString &key);
	static QVariant getValue(const QString &key);

protected:
	explicit SettingsManager(const QString &path, QObject *parent = NULL);

private:
	static SettingsManager *m_instance;
	static QString m_path;
	static QHash<QString, QVariant> m_defaults;

signals:
	void valueChanged(QString key, QVariant value);
};

}

#endif
