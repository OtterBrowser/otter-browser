/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014, 2016 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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
#include <QtCore/QUrl>
#include <QtCore/QVariant>

namespace Otter
{

class SettingsManager : public QObject
{
	Q_OBJECT

public:
	enum OptionType
	{
		UnknownType = 0,
		BooleanType,
		ColorType,
		EnumerationType,
		FontType,
		IconType,
		IntegerType,
		ListType,
		PathType,
		StringType
	};

	struct OptionDefinition
	{
		QString name;
		QVariant defaultValue;
		QStringList choices;
		OptionType type;
	};

	static void createInstance(const QString &path, QObject *parent = NULL);
	static void removeOverride(const QUrl &url, const QString &key = QString());
	static void setDefinition(const QString &key, const OptionDefinition &definition);
	static void setValue(const QString &key, const QVariant &value, const QUrl &url = QUrl());
	static SettingsManager* getInstance();
	static QString getReport();
	static QVariant getValue(const QString &key, const QUrl &url = QUrl());
	static QStringList getOptions();
	static OptionDefinition getDefinition(const QString &key);
	static bool hasOverride(const QUrl &url, const QString &key = QString());

protected:
	explicit SettingsManager(QObject *parent = NULL);

	static QString getHost(const QUrl &url);

private:
	static SettingsManager *m_instance;
	static QString m_globalPath;
	static QString m_overridePath;
	static QHash<QString, OptionDefinition> m_options;
	static QHash<QString, QVariant> m_defaults;

signals:
	void valueChanged(const QString &key, const QVariant &value);
	void valueChanged(const QString &key, const QVariant &value, const QUrl &url);
};

}

#endif
