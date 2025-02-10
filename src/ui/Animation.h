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

#ifndef OTTER_ANIMATION_H
#define OTTER_ANIMATION_H

#include <QtGui/QColor>
#include <QtGui/QMovie>
#include <QtSvg/QSvgRenderer>

namespace Otter
{

class Animation : public QObject
{
	Q_OBJECT

public:
	explicit Animation(QObject *parent = nullptr);

	virtual void paint(QPainter *painter, const QRect &rectangle = {}) const = 0;
	virtual QPixmap getCurrentPixmap() const = 0;
	virtual bool isRunning() const = 0;
	virtual void setColor(const QColor &color) = 0;
	virtual void setScaledSize(const QSize &size) = 0;

public slots:
	virtual void start() = 0;
	virtual void stop() = 0;

signals:
	void frameChanged();
};

class GenericAnimation final : public Animation
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

	explicit GenericAnimation(const QString &path, QObject *parent = nullptr);

	void paint(QPainter *painter, const QRect &rectangle = {}) const override;
	QPixmap getCurrentPixmap() const override;
	bool isRunning() const override;
	void setColor(const QColor &color) override;
	void setScaledSize(const QSize &size) override;

public slots:
	void start() override;
	void stop() override;

protected:
	void createSvgRenderer();

private:
	QMovie *m_gifMovie;
	QSvgRenderer *m_svgRenderer;
	QString m_path;
	QColor m_color;
	QSize m_scaledSize;
	AnimationFormat m_format;
};

class SpinnerAnimation final : public Animation
{
	Q_OBJECT

public:
	explicit SpinnerAnimation(QObject *parent = nullptr);

	void paint(QPainter *painter, const QRect &rectangle = {}) const override;
	QPixmap getCurrentPixmap() const override;
	bool isRunning() const override;
	void setColor(const QColor &color) override;
	void setScaledSize(const QSize &size) override;

public slots:
	void start() override;
	void stop() override;

protected:
	void timerEvent(QTimerEvent *event) override;

private:
	QColor m_color;
	QSize m_scaledSize;
	int m_step;
	int m_updateTimer;
};

}

#endif
