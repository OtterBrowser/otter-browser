/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_ANIMATION_H
#define OTTER_ANIMATION_H

#include <QtGui/QMovie>
#include <QtSvg/QSvgRenderer>

namespace Otter
{

class Animation final : public QObject
{
	Q_OBJECT

public:
	enum AnimationFormat
	{
		UnknownFormat = 0,
		GifFormat,
		SvgFormat,
		InvalidFormat
	};

	explicit Animation(const QString &path, QObject *parent = nullptr);

	QPixmap getCurrentPixmap() const;
	bool isRunning() const;
	void setColor(const QColor &color);

public slots:
	void start();
	void stop();

protected:
	void createSvgRenderer();

private:
	QMovie *m_gifMovie;
	QSvgRenderer *m_svgRenderer;
	QString m_path;
	QColor m_color;
	AnimationFormat m_format;

signals:
	void frameChanged();
};

}

#endif
