/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "TransfersWidget.h"
#include "../../../core/Application.h"
#include "../../../core/ThemesManager.h"
#include "../../../core/TransfersManager.h"
#include "../../../core/Utils.h"
#include "../../../ui/Action.h"

#include <QtCore/QFileInfo>
#include <QtWidgets/QMenu>

namespace Otter
{

TransfersWidget::TransfersWidget(const ToolBarsManager::ToolBarDefinition::Entry &definition, QWidget *parent) : ToolButtonWidget(definition, parent),
	m_icon(ThemesManager::createIcon(QLatin1String("transfers")))
{
	setMenu(new QMenu(this));
	setPopupMode(QToolButton::InstantPopup);
	updateState();

	connect(TransfersManager::getInstance(), &TransfersManager::transferChanged, this, &TransfersWidget::updateState);
	connect(TransfersManager::getInstance(), &TransfersManager::transferStarted, this, &TransfersWidget::updateState);
	connect(TransfersManager::getInstance(), &TransfersManager::transferFinished, this, &TransfersWidget::updateState);
	connect(TransfersManager::getInstance(), &TransfersManager::transferRemoved, this, &TransfersWidget::updateState);
	connect(TransfersManager::getInstance(), &TransfersManager::transferStopped, this, &TransfersWidget::updateState);
}

void TransfersWidget::updateState()
{
	menu()->clear();

	const QVector<Transfer*> transfers(TransfersManager::getInstance()->getTransfers());
	qint64 bytesTotal(0);
	qint64 bytesReceived(0);
	qint64 transferAmount(0);

	for (int i = 0; i < transfers.count(); ++i)
	{
		const Transfer *transfer(transfers.at(i));

		if (transfer->getState() == Transfer::RunningState && transfer->getBytesTotal() > 0)
		{
			++transferAmount;

			bytesTotal += transfer->getBytesTotal();
			bytesReceived += transfer->getBytesReceived();
		}

		if (!transfer->isArchived() || transfer->getState() == Transfer::RunningState)
		{
			menu()->addAction(Utils::elideText(QFileInfo(transfer->getTarget()).fileName(), menu()));
		}
	}

	menu()->addSeparator();
	menu()->addAction(new Action(ActionsManager::TransfersAction, {}, ActionExecutor::Object(Application::getInstance(), Application::getInstance()), this));
	setIcon(getIcon());
}

QIcon TransfersWidget::getIcon() const
{
	return m_icon;
}

}
