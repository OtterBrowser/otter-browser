/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ProgressInformationWidget.h"
#include "../ContentsWidget.h"
#include "../ToolBarWidget.h"
#include "../Window.h"
#include "../../core/Utils.h"

#include <QtWidgets/QHBoxLayout>

namespace Otter
{

ProgressInformationWidget::ProgressInformationWidget(Window *window, const ActionsManager::ActionEntryDefinition &definition, QWidget *parent) : QWidget(parent),
	m_window(window),
	m_label(NULL),
	m_progressBar(NULL),
	m_type(UnknownType)
{
	QHBoxLayout *layout(new QHBoxLayout(this));
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	if (definition.action == QLatin1String("ProgressInformationDocumentPercentWidget"))
	{
		m_type = TotalPercentType;
	}
	else if (definition.action == QLatin1String("ProgressInformationTotalSizeWidget"))
	{
		m_type = TotalBytesType;
	}
	else if (definition.action == QLatin1String("ProgressInformationElementsWidget"))
	{
		m_type = ElementsType;
	}
	else if (definition.action == QLatin1String("ProgressInformationSpeedWidget"))
	{
		m_type = SpeedType;
	}
	else if (definition.action == QLatin1String("ProgressInformationElapsedTimeWidget"))
	{
		m_type = ElapsedTimeType;
	}
	else if (definition.action == QLatin1String("ProgressInformationMessageWidget"))
	{
		m_type = MessageType;
	}

	if (m_type == TotalPercentType)
	{
		m_progressBar = new QProgressBar(this);
		m_progressBar->setFormat(tr("Document: %p%"));
		m_progressBar->setAlignment(Qt::AlignCenter);

		layout->addWidget(m_progressBar);
	}
	else
	{
		m_label = new QLabel(this);

		if (m_type != MessageType)
		{
			m_label->setAlignment(Qt::AlignCenter);
		}

		layout->addWidget(m_label);
	}

	setSizePolicy(((m_type == MessageType) ? QSizePolicy::Expanding : QSizePolicy::Fixed), QSizePolicy::Preferred);
	setWindow(window);

	ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent));

	if (toolBar && toolBar->getIdentifier() != ToolBarsManager::NavigationBar)
	{
		connect(toolBar, SIGNAL(windowChanged(Window*)), this, SLOT(setWindow(Window*)));
	}
}

void ProgressInformationWidget::updateLoadStatus(int elapsedTime, int finishedRequests, int startedReuests, qint64 bytesReceived, qint64 bytesTotal, qint64 speed)
{
	switch (m_type)
	{
		case TotalBytesType:
			m_label->setText(tr("Total: %1").arg(Utils::formatUnit(bytesReceived, false, 1)));

			break;
		case ElementsType:
			m_label->setText(tr("Elements: %1/%2").arg(finishedRequests).arg(startedReuests));

			break;
		case SpeedType:
			m_label->setText(tr("Speed: %1").arg(Utils::formatUnit(speed, true, 1)));

			break;
		case ElapsedTimeType:
			{
				int minutes(elapsedTime / 60);
				int seconds(elapsedTime - (minutes * 60));

				m_label->setText(tr("Time: %1").arg(QStringLiteral("%1:%2").arg(minutes).arg(seconds, 2, 10, QLatin1Char('0'))));

				break;
			}
		default:
			break;
	}
}

void ProgressInformationWidget::setWindow(Window *window)
{
	if (m_window)
	{
		if (m_type == TotalPercentType)
		{
			disconnect(m_window->getContentsWidget(), SIGNAL(loadProgress(int)), m_progressBar, SLOT(setValue(int)));

			m_progressBar->setValue(0);
		}
		else if (m_type == MessageType)
		{
			disconnect(m_window->getContentsWidget(), SIGNAL(loadMessageChanged(QString)), m_label, SLOT(setText(QString)));

			m_label->setText(QString());
		}
		else
		{
			disconnect(m_window->getContentsWidget(), SIGNAL(loadStatusChanged(int,int,int,qint64,qint64,qint64)), this, SLOT(updateLoadStatus(int,int,int,qint64,qint64,qint64)));

			updateLoadStatus(0, 0, 0, 0, 0, 0);
		}
	}

	m_window = window;

	if (window)
	{
		if (m_type == TotalPercentType)
		{
			connect(window->getContentsWidget(), SIGNAL(loadProgress(int)), m_progressBar, SLOT(setValue(int)));
		}
		else if (m_type == MessageType)
		{
			connect(window->getContentsWidget(), SIGNAL(loadMessageChanged(QString)), m_label, SLOT(setText(QString)));
		}
		else
		{
			connect(window->getContentsWidget(), SIGNAL(loadStatusChanged(int,int,int,qint64,qint64,qint64)), this, SLOT(updateLoadStatus(int,int,int,qint64,qint64,qint64)));
		}
	}
}

QSize ProgressInformationWidget::sizeHint() const
{
	QSize size(QWidget::sizeHint());

	if (m_type == MessageType)
	{
		return QSize(250, size.height());
	}

	return QSize(150, size.height());
}

}
