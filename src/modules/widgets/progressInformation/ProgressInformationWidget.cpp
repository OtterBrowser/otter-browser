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
#include "../../../core/Utils.h"
#include "../../../ui/ContentsWidget.h"
#include "../../../ui/ToolBarWidget.h"
#include "../../../ui/Window.h"

#include <QtWidgets/QHBoxLayout>

namespace Otter
{

ProgressInformationWidget::ProgressInformationWidget(Window *window, const ActionsManager::ActionEntryDefinition &definition, QWidget *parent) : QWidget(parent),
	m_window(nullptr),
	m_label(nullptr),
	m_progressBar(nullptr),
	m_type(UnknownType)
{
	QHBoxLayout *layout(new QHBoxLayout(this));
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	if (definition.action == QLatin1String("ProgressInformationDocumentProgressWidget"))
	{
		m_type = DocumentProgressType;
	}
	else if (definition.action == QLatin1String("ProgressInformationTotalSizeWidget"))
	{
		m_type = TotalSizeType;
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

	if (m_type == DocumentProgressType)
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

void ProgressInformationWidget::updateStatus(WebWidget::PageInformation key, const QVariant &value)
{
	switch (m_type)
	{
		case DocumentProgressType:
			if (key == WebWidget::DocumentLoadingProgressInformation)
			{
				m_progressBar->setValue(value.toInt());
			}

			break;
		case TotalSizeType:
			if (key == WebWidget::BytesReceivedInformation)
			{
				m_label->setText(tr("Total: %1").arg(Utils::formatUnit(value.toULongLong(), false, 1)));
			}

			break;
		case ElementsType:
			if (key == WebWidget::RequestsFinishedInformation)
			{
				m_label->setText(tr("Elements: %1/%2").arg(value.toInt()).arg(m_window ? m_window->getContentsWidget()->getPageInformation(WebWidget::RequestsStartedInformation).toInt() : 0));
			}

			break;
		case SpeedType:
			if (key == WebWidget::LoadingSpeedInformation)
			{
				m_label->setText(tr("Speed: %1").arg(Utils::formatUnit(value.toULongLong(), true, 1)));
			}

			break;
		case ElapsedTimeType:
			if (key == WebWidget::LoadingTimeInformation)
			{
				int minutes(value.toInt() / 60);
				int seconds(value.toInt() - (minutes * 60));

				m_label->setText(tr("Time: %1").arg(QStringLiteral("%1:%2").arg(minutes).arg(seconds, 2, 10, QLatin1Char('0'))));
			}

			break;
		case MessageType:
			if (key == WebWidget::LoadingMessageInformation)
			{
				m_label->setText(value.toString());
			}

			break;
		default:
			break;
	}
}

void ProgressInformationWidget::setWindow(Window *window)
{
	WebWidget::PageInformation type(WebWidget::UnknownInformation);

	switch (m_type)
	{
		case DocumentProgressType:
			type = WebWidget::DocumentLoadingProgressInformation;

			break;
		case TotalSizeType:
			type = WebWidget::BytesReceivedInformation;

			break;
		case ElementsType:
			type = WebWidget::RequestsFinishedInformation;

			break;
		case SpeedType:
			type = WebWidget::LoadingSpeedInformation;

			break;
		case ElapsedTimeType:
			type = WebWidget::LoadingTimeInformation;

			break;
		case MessageType:
			type = WebWidget::LoadingMessageInformation;

			break;
		default:
			break;
	}

	if (m_window && !m_window->isAboutToClose())
	{
		disconnect(m_window->getContentsWidget(), SIGNAL(pageInformationChanged(WebWidget::PageInformation,QVariant)), this, SLOT(updateStatus(WebWidget::PageInformation,QVariant)));

		updateStatus(type);
	}

	m_window = window;

	if (window)
	{
		updateStatus(type, window->getContentsWidget()->getPageInformation(type));

		connect(m_window->getContentsWidget(), SIGNAL(pageInformationChanged(WebWidget::PageInformation,QVariant)), this, SLOT(updateStatus(WebWidget::PageInformation,QVariant)));
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
