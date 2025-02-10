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

#include "Animation.h"
#include "../core/Application.h"

#include <QtCore/QFile>
#include <QtGui/QPainter>

namespace Otter
{

Animation::Animation(QObject *parent) : QObject(parent)
{
}

GenericAnimation::GenericAnimation(const QString &path, QObject *parent) : Animation(parent),
	m_gifMovie(nullptr),
	m_svgRenderer(nullptr),
	m_path(path),
	m_format(UnknownFormat)
{
	if (path.isEmpty())
	{
		return;
	}

	m_gifMovie = new QMovie(path, QByteArrayLiteral("gif"), this);

	if (m_gifMovie->isValid())
	{
		m_format = GifFormat;

		connect(m_gifMovie, &QMovie::frameChanged, m_gifMovie, [&]()
		{
			emit frameChanged();
		});
	}
	else
	{
		m_gifMovie->deleteLater();
		m_gifMovie = nullptr;
	}
}

void GenericAnimation::createSvgRenderer()
{
	if (m_format == GifFormat || m_format == InvalidFormat || m_path.isEmpty())
	{
		return;
	}

	if (m_svgRenderer)
	{
		m_svgRenderer->deleteLater();
		m_svgRenderer = nullptr;
	}

	if (m_color.isValid())
	{
		QFile file(m_path);

		if (file.open(QIODevice::ReadOnly))
		{
			QByteArray data(file.readAll());

			m_svgRenderer = new QSvgRenderer(data.replace(QByteArrayLiteral("{color}"), m_color.name().toLatin1()), this);

			file.close();
		}
		else
		{
			m_format = InvalidFormat;

			return;
		}
	}
	else
	{
		m_svgRenderer = new QSvgRenderer(m_path, this);
	}

	if (m_svgRenderer->isValid())
	{
		m_format = SvgFormat;

		connect(m_svgRenderer, &QSvgRenderer::repaintNeeded, this, &GenericAnimation::frameChanged);
	}
	else
	{
		m_format = InvalidFormat;

		m_svgRenderer->deleteLater();
		m_svgRenderer = nullptr;
	}
}

void GenericAnimation::paint(QPainter *painter, const QRect &rectangle) const
{
	if (m_gifMovie)
	{
		const QSize size(m_scaledSize.isValid() ? m_scaledSize : m_gifMovie->frameRect().size());

		painter->drawPixmap((rectangle.isValid() ? rectangle : QRect(0, 0, size.width(), size.height())), m_gifMovie->currentPixmap());
	}
	else if (m_svgRenderer)
	{
		const QSize size(m_scaledSize.isValid() ? m_scaledSize : m_svgRenderer->defaultSize());

		m_svgRenderer->render(painter, (rectangle.isValid() ? rectangle : QRect(0, 0, size.width(), size.height())));
	}
}

void GenericAnimation::start()
{
	if (!isRunning() && !m_path.isEmpty() && m_format != InvalidFormat)
	{
		if (m_gifMovie)
		{
			m_gifMovie->start();
		}
		else
		{
			createSvgRenderer();
		}

		emit frameChanged();
	}
}

void GenericAnimation::stop()
{
	if (m_gifMovie)
	{
		m_gifMovie->stop();
	}
	else if (m_svgRenderer)
	{
		m_svgRenderer->deleteLater();
		m_svgRenderer = nullptr;
	}
}

void GenericAnimation::setColor(const QColor &color)
{
	if (color != m_color && m_svgRenderer)
	{
		createSvgRenderer();
	}
}

void GenericAnimation::setScaledSize(const QSize &size)
{
	if (size != m_scaledSize)
	{
		m_scaledSize = size;

		if (m_gifMovie)
		{
			m_gifMovie->setScaledSize(size);
		}
	}
}

QPixmap GenericAnimation::getCurrentPixmap() const
{
	if (m_gifMovie)
	{
		return m_gifMovie->currentPixmap();
	}

	if (m_svgRenderer)
	{
		QPixmap pixmap(m_scaledSize.isValid() ? m_scaledSize : m_svgRenderer->defaultSize());
		pixmap.setDevicePixelRatio(Application::getInstance()->devicePixelRatio());
		pixmap.fill(Qt::transparent);

		QPainter painter(&pixmap);

		m_svgRenderer->render(&painter, QRect(0, 0, pixmap.width(), pixmap.height()));

		return pixmap;
	}

	return {};
}

bool GenericAnimation::isRunning() const
{
	return (m_gifMovie ? (m_gifMovie->state() == QMovie::Running) : (m_svgRenderer != nullptr));
}

SpinnerAnimation::SpinnerAnimation(QObject *parent) : Animation(parent),
	m_color(0, 0, 0, 200),
	m_scaledSize(16, 16),
	m_step(0),
	m_updateTimer(0)
{
}

void SpinnerAnimation::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_updateTimer)
	{
		m_step -= 6;

		if (m_step == -360)
		{
			m_step = 0;
		}

		emit frameChanged();
	}
}

void SpinnerAnimation::paint(QPainter *painter, const QRect &rectangle) const
{
	const QSize size(rectangle.isValid() ? rectangle.size() : m_scaledSize);
	const qreal offset(size.width() / 8.0);
	const QRectF targetRectangle((rectangle.x() + offset), (rectangle.y() + offset), (size.width() - (offset * 2)), (size.height() - (offset * 2)));
	QConicalGradient gradient(targetRectangle.center(), m_step);
	gradient.setColorAt(0, m_color);
	gradient.setColorAt(1, Qt::transparent);

	painter->save();
	painter->setRenderHint(QPainter::Antialiasing);
	painter->setPen(QPen(gradient, (offset * 1.5)));
	painter->drawArc(targetRectangle, 0, 5760);
	painter->restore();
}

void SpinnerAnimation::start()
{
	if (m_updateTimer == 0)
	{
		m_updateTimer = startTimer(15);
	}

	m_step = 0;
}

void SpinnerAnimation::stop()
{
	if (m_updateTimer != 0)
	{
		killTimer(m_updateTimer);

		m_updateTimer = 0;
	}
}

QPixmap SpinnerAnimation::getCurrentPixmap() const
{
	QPixmap pixmap(m_scaledSize);
	pixmap.setDevicePixelRatio(Application::getInstance()->devicePixelRatio());
	pixmap.fill(Qt::transparent);

	QPainter painter(&pixmap);

	paint(&painter, pixmap.rect());

	return pixmap;
}

bool SpinnerAnimation::isRunning() const
{
	return (m_updateTimer != 0);
}

void SpinnerAnimation::setColor(const QColor &color)
{
	m_color = color;
}

void SpinnerAnimation::setScaledSize(const QSize &size)
{
	m_scaledSize = size;
}

}
