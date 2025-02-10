/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_JSONSETTINGS_H
#define OTTER_JSONSETTINGS_H

#include <QtCore/QJsonDocument>
#include <QtCore/QRect>

namespace Otter
{

class JsonSettings final : public QJsonDocument
{
public:
	explicit JsonSettings();
	explicit JsonSettings(const QString &path);

	void setComment(const QString &comment);
	static QRect readRectangle(const QVariant &value);
	static QPoint readPoint(const QVariant &value);
	static QSize readSize(const QVariant &value);
	QString getComment() const;
	bool save(const QString &path = {}, bool isAtomic = true);
	bool hasError() const;

private:
	QString m_path;
	QString m_comment;
	bool m_hasError;
};

}

#endif
