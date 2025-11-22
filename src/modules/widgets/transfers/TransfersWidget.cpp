/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../../../ui/ProgressBarWidget.h"
#include "../../../ui/Style.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QtMath>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolTip>
#include <QtWidgets/QWidgetAction>

namespace Otter
{

TransfersWidget::TransfersWidget(const ToolBarsManager::ToolBarDefinition::Entry &definition, QWidget *parent) : ToolButtonWidget(definition, parent),
	m_icon(ThemesManager::createIcon(QLatin1String("transfers")))
{
	QMenu *menu(new QMenu(this));

	setMenu(menu);
	setPopupMode(QToolButton::InstantPopup);
	setToolTip(tr("Downloads"));
	updateState();

	connect(TransfersManager::getInstance(), &TransfersManager::transferChanged, this, &TransfersWidget::updateState);
	connect(TransfersManager::getInstance(), &TransfersManager::transferStarted, this, [&](Transfer *transfer)
	{
		if ((!transfer->isArchived() || transfer->getState() == Transfer::RunningState) && menu->isVisible())
		{
			QAction *firstAction(menu->actions().value(0));
			QWidgetAction *widgetAction(new QWidgetAction(menu));
			widgetAction->setDefaultWidget(new TransferActionWidget(transfer, menu));

			menu->insertAction(firstAction, widgetAction);
			menu->insertSeparator(firstAction);
		}

		updateState();
	});
	connect(TransfersManager::getInstance(), &TransfersManager::transferFinished, this, [&](const Transfer *transfer)
	{
		const QList<QAction*> actions(menu->actions());

		for (int i = 0; i < actions.count(); ++i)
		{
			const QWidgetAction *widgetAction(qobject_cast<QWidgetAction*>(actions.at(i)));

			if (!widgetAction || !widgetAction->defaultWidget())
			{
				continue;
			}

			const TransferActionWidget *transferActionWidget(qobject_cast<TransferActionWidget*>(widgetAction->defaultWidget()));

			if (transferActionWidget && transferActionWidget->getTransfer() == transfer)
			{
				menu->removeAction(actions.at(i));
				menu->removeAction(actions.value(i + 1));

				break;
			}
		}

		updateState();
	});
	connect(TransfersManager::getInstance(), &TransfersManager::transferRemoved, this, &TransfersWidget::updateState);
	connect(TransfersManager::getInstance(), &TransfersManager::transferStopped, this, &TransfersWidget::updateState);
	connect(menu, &QMenu::aboutToShow, this, &TransfersWidget::populateMenu);
	connect(menu, &QMenu::aboutToHide, menu, &QMenu::clear);
}

void TransfersWidget::changeEvent(QEvent *event)
{
	ToolButtonWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		setToolTip(tr("Downloads"));
	}
}

void TransfersWidget::populateMenu()
{
	const QVector<Transfer*> transfers(TransfersManager::getTransfers());

	for (int i = 0; i < transfers.count(); ++i)
	{
		Transfer *transfer(transfers.at(i));

		if (!transfer->isArchived() || transfer->getState() == Transfer::RunningState)
		{
			QWidgetAction *widgetAction(new QWidgetAction(menu()));
			widgetAction->setDefaultWidget(new TransferActionWidget(transfer, menu()));

			menu()->addAction(widgetAction);
			menu()->addSeparator();
		}
	}

	Action *transfersAction(new Action(ActionsManager::TransfersAction, {}, ActionExecutor::Object(Application::getInstance(), Application::getInstance()), this));
	transfersAction->setTextOverride(QT_TRANSLATE_NOOP("actions", "Show all Downloads"));

	menu()->addAction(transfersAction);
}

void TransfersWidget::updateState()
{
	const TransfersManager::ActiveTransfersInformation information(TransfersManager::getActiveTransfersInformation());

	m_icon = ThemesManager::createIcon(QLatin1String("transfers"));

	if (information.activeTransfersAmount - information.unknownProgressTransfersAmount > 0)
	{
		const int iconSize(this->iconSize().width());
		QPixmap pixmap(m_icon.pixmap(iconSize, iconSize));
		QPainter painter(&pixmap);
		QStyleOptionProgressBar progressBarOption;
		progressBarOption.palette = palette();
		progressBarOption.minimum = 0;
		progressBarOption.maximum = 100;
		progressBarOption.progress = static_cast<int>(Utils::calculatePercent(information.bytesReceived, information.bytesTotal));
		progressBarOption.rect = pixmap.rect();
		progressBarOption.rect.setTop(progressBarOption.rect.bottom() - (iconSize / 8));

		ThemesManager::getStyle()->drawThinProgressBar(&progressBarOption, &painter);

		m_icon = QIcon(pixmap);
	}

	setIcon(m_icon);
}

QString TransfersWidget::getToolTip() const
{
	return tr("Downloads");
}

QIcon TransfersWidget::getIcon() const
{
	return m_icon;
}

TransferActionWidget::TransferActionWidget(Transfer *transfer, QWidget *parent) : QWidget(parent),
	m_transfer(transfer),
	m_detailsLabel(new QLabel(this)),
	m_fileNameLabel(new QLabel(this)),
	m_iconLabel(new QLabel(this)),
	m_progressBar(new ProgressBarWidget(this)),
	m_toolButton(new QToolButton(this)),
	m_centralWidget(new QWidget(this))
{
	QVBoxLayout *centralLayout(new QVBoxLayout(m_centralWidget));
	centralLayout->setContentsMargins(0, 0, 0, 0);
	centralLayout->addWidget(m_fileNameLabel);
	centralLayout->addWidget(m_progressBar);
	centralLayout->addWidget(m_detailsLabel);

	QFrame *leftSeparatorFrame(new QFrame(this));
	leftSeparatorFrame->setFrameShape(QFrame::VLine);

	QFrame *rightSeparatorFrame(new QFrame(this));
	rightSeparatorFrame->setFrameShape(QFrame::VLine);

	QHBoxLayout *mainLayout(new QHBoxLayout(this));
	mainLayout->addWidget(m_iconLabel);
	mainLayout->addWidget(leftSeparatorFrame);
	mainLayout->addWidget(m_centralWidget);
	mainLayout->addWidget(rightSeparatorFrame);
	mainLayout->addWidget(m_toolButton);

	setLayout(mainLayout);
	updateState();

	m_iconLabel->setFixedSize(32, 32);
	m_progressBar->setMode(ProgressBarWidget::ThinMode);
	m_toolButton->setIconSize({16, 16});
	m_toolButton->setAutoRaise(true);

	connect(transfer, &Transfer::changed, this, &TransferActionWidget::updateState);
	connect(transfer, &Transfer::finished, this, &TransferActionWidget::updateState);
	connect(transfer, &Transfer::stopped, this, &TransferActionWidget::updateState);
	connect(transfer, &Transfer::progressChanged, this, &TransferActionWidget::updateState);
	connect(m_toolButton, &QToolButton::clicked, this, [&]()
	{
		switch (m_transfer->getState())
		{
			case Transfer::CancelledState:
			case Transfer::ErrorState:
				m_transfer->restart();

				break;
			case Transfer::FinishedState:
				Utils::runApplication({}, QUrl::fromLocalFile(QFileInfo(m_transfer->getTarget()).dir().canonicalPath()));

				break;
			default:
				m_transfer->stop();

				break;
		}
	});
}

void TransferActionWidget::mousePressEvent(QMouseEvent *event)
{
	event->accept();
}

void TransferActionWidget::mouseReleaseEvent(QMouseEvent *event)
{
	event->accept();

	if (event->button() == Qt::LeftButton)
	{
		m_transfer->openTarget();
	}
}

void TransferActionWidget::updateState()
{
	QString details;
	QVector<QPair<QString, QString> > detailsValues({{tr("From:"), Utils::extractHost(m_transfer->getSource())}});
	const bool isIndeterminate(m_transfer->getBytesTotal() <= 0);
	const bool hasError(m_transfer->getState() == Transfer::UnknownState || m_transfer->getState() == Transfer::ErrorState);

	if (m_transfer->getState() == Transfer::FinishedState)
	{
		detailsValues.append({tr("Size:"), tr("%1 (download completed)").arg(Utils::formatUnit(m_transfer->getBytesTotal()))});
	}
	else
	{
		detailsValues.append({tr("Size:"), tr("%1 (%2% downloaded)").arg(Utils::formatUnit(m_transfer->getBytesTotal())).arg(Utils::calculatePercent(m_transfer->getBytesReceived(), m_transfer->getBytesTotal()), 0, 'f', 1)});
	}

	for (int i = 0; i < detailsValues.count(); ++i)
	{
		const QPair<QString, QString> detailsValue(detailsValues.at(i));

		details.append(detailsValue.first + QLatin1Char(' ') + detailsValue.second);

		if (i < (detailsValues.count() - 1))
		{
			details.append(QLatin1String("<br>"));
		}
	}

	m_fileNameLabel->setText(Utils::elideText(QFileInfo(m_transfer->getTarget()).fileName(), m_fileNameLabel->fontMetrics(), nullptr, 300));
	m_detailsLabel->setText(QLatin1String("<small>") + details + QLatin1String("</small>"));
	m_iconLabel->setPixmap(m_transfer->getIcon().pixmap(32, 32));
	m_progressBar->setHasError(hasError);
	m_progressBar->setRange(0, ((isIndeterminate && !hasError) ? 0 : 100));
	m_progressBar->setValue(isIndeterminate ? (hasError ? 0 : -1) : ((m_transfer->getBytesTotal() > 0) ? qFloor(Utils::calculatePercent(m_transfer->getBytesReceived(), m_transfer->getBytesTotal())) : -1));
	m_progressBar->setFormat(isIndeterminate ? tr("Unknown") : QLatin1String("%p%"));

	switch (m_transfer->getState())
	{
		case Transfer::CancelledState:
		case Transfer::ErrorState:
			m_toolButton->setIcon(ThemesManager::createIcon(QLatin1String("view-refresh")));
			m_toolButton->setToolTip(tr("Redownload"));

			break;
		case Transfer::FinishedState:
			m_toolButton->setIcon(ThemesManager::createIcon(QLatin1String("document-open-folder")));
			m_toolButton->setToolTip(tr("Open Folder"));

			break;
		default:
			m_toolButton->setIcon(ThemesManager::createIcon(QLatin1String("task-reject")));
			m_toolButton->setToolTip(tr("Stop"));

			break;
	}
}

Transfer* TransferActionWidget::getTransfer() const
{
	return m_transfer;
}

bool TransferActionWidget::event(QEvent *event)
{
	if (event->type() == QEvent::ToolTip)
	{
		const bool isIndeterminate(m_transfer->getBytesTotal() <= 0);

		QToolTip::showText(static_cast<QHelpEvent*>(event)->globalPos(), tr("<div style=\"white-space:pre;\">Source: %1\nTarget: %2\nSize: %3\nDownloaded: %4\nProgress: %5</div>").arg(m_transfer->getSource().toDisplayString().toHtmlEscaped(), m_transfer->getTarget().toHtmlEscaped(), (isIndeterminate ? tr("Unknown") : Utils::formatUnit(m_transfer->getBytesTotal(), false, 1, true)), Utils::formatUnit(m_transfer->getBytesReceived(), false, 1, true), (isIndeterminate ? tr("Unknown") : QStringLiteral("%1%")).arg(Utils::calculatePercent(m_transfer->getBytesReceived(), m_transfer->getBytesTotal()), 0, 'f', 1)));

		return true;
	}

	return QWidget::event(event);
}

}
