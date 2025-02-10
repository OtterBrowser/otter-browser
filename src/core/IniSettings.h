/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#ifndef OTTER_INISETTINGS_H
#define OTTER_INISETTINGS_H

#include <QtCore/QObject>
#include <QtCore/QVariantMap>

namespace Otter
{

class IniSettings final : public QObject
{
	Q_OBJECT

public:
	explicit IniSettings(QObject *parent = nullptr);
	explicit IniSettings(const QString &path, QObject *parent = nullptr);

	void clear();
	void beginGroup(const QString &group);
	void removeGroup(const QString &group);
	void endGroup();
	void setComment(const QString &comment);
	void setValue(const QString &key, const QVariant &value);
	QString getComment() const;
	QVariant getValue(const QString &key, const QVariant &fallback = {}) const;
	QStringList getGroups() const;
	QStringList getKeys() const;
	bool save(const QString &path = {}, bool isAtomic = true);
	bool hasError() const;

private:
	QString m_path;
	QString m_group;
	QString m_comment;
	QMap<QString, QVariantMap> m_data;
	bool m_hasError;
};

}

#endif
