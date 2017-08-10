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

#include "Animation.h"

#include <QtCore/QFile>
#include <QtGui/QPainter>

namespace Otter
{

Animation::Animation(const QString &path, QObject *parent) : QObject(parent),
	m_gifMovie(nullptr),
	m_svgRenderer(nullptr),
	m_path(path),
	m_format(UnknownFormat)
{
	if (!path.isEmpty())
	{
		m_gifMovie = new QMovie(path, QByteArrayLiteral("gif"), this);

		if (m_gifMovie->isValid())
		{
			m_format = GifFormat;

			connect(m_gifMovie, &QMovie::frameChanged, [&](int frame)
			{
				Q_UNUSED(frame)

				emit frameChanged();
			});
		}
		else
		{
			m_gifMovie->deleteLater();
			m_gifMovie = nullptr;
		}
	}
}

void Animation::createSvgRenderer()
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

	if (!m_color.isValid())
	{
		m_svgRenderer = new QSvgRenderer(m_path, this);
	}
	else
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

	if (m_svgRenderer->isValid())
	{
		m_format = SvgFormat;

		connect(m_svgRenderer, &QSvgRenderer::repaintNeeded, this, &Animation::frameChanged);
	}
	else
	{
		m_format = InvalidFormat;

		m_svgRenderer->deleteLater();
		m_svgRenderer = nullptr;
	}
}

void Animation::start()
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

void Animation::stop()
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

void Animation::setColor(const QColor &color)
{
	if (color != m_color && m_svgRenderer)
	{
		createSvgRenderer();
	}
}

QPixmap Animation::getCurrentPixmap() const
{
	if (m_gifMovie)
	{
		return m_gifMovie->currentPixmap();
	}

	if (m_svgRenderer)
	{
		QPixmap pixmap(m_svgRenderer->defaultSize());
		pixmap.fill(Qt::transparent);

		QPainter painter(&pixmap);

		m_svgRenderer->render(&painter, QRectF(0, 0, pixmap.width(), pixmap.height()));

		return pixmap;
	}

	return QPixmap();
}

bool Animation::isRunning() const
{
	return (m_gifMovie ? (m_gifMovie->state() == QMovie::Running) : (m_svgRenderer != nullptr));
}

}
