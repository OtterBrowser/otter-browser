/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "TransfersContentsWidget.h"
#include "ProgressBarDelegate.h"
#include "../../../core/ActionsManager.h"
#include "../../../core/Utils.h"
#include "../../../ui/ItemDelegate.h"

#include "ui_TransfersContentsWidget.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QtMath>
#include <QtCore/QQueue>
#include <QtGui/QClipboard>
#include <QtGui/QDesktopServices>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>

namespace Otter
{

TransfersContentsWidget::TransfersContentsWidget(Window *window) : ContentsWidget(window),
	m_model(new QStandardItemModel(this)),
	m_isLoading(false),
	m_ui(new Ui::TransfersContentsWidget)
{
	m_ui->setupUi(this);

	QStringList labels;
	labels << QString() << tr("Filename") << tr("Size") << tr("Progress") << tr("Time") << tr("Speed") << tr("Started") << tr("Finished");

	m_model->setHorizontalHeaderLabels(labels);

	m_ui->transfersView->setModel(m_model);
	m_ui->transfersView->horizontalHeader()->setTextElideMode(Qt::ElideRight);
	m_ui->transfersView->horizontalHeader()->resizeSection(0, 30);
	m_ui->transfersView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
	m_ui->transfersView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	m_ui->transfersView->setItemDelegate(new ItemDelegate(this));
	m_ui->transfersView->setItemDelegateForColumn(3, new ProgressBarDelegate(this));
	m_ui->transfersView->installEventFilter(this);
	m_ui->stopResumeButton->setIcon(Utils::getIcon(QLatin1String("task-ongoing")));
	m_ui->redownloadButton->setIcon(Utils::getIcon(QLatin1String("view-refresh")));

	const QList<TransferInformation*> transfers = TransfersManager::getTransfers();

	for (int i = 0; i < transfers.count(); ++i)
	{
		addTransfer(transfers.at(i));
	}

	connect(TransfersManager::getInstance(), SIGNAL(transferStarted(TransferInformation*)), this, SLOT(addTransfer(TransferInformation*)));
	connect(TransfersManager::getInstance(), SIGNAL(transferRemoved(TransferInformation*)), this, SLOT(removeTransfer(TransferInformation*)));
	connect(TransfersManager::getInstance(), SIGNAL(transferUpdated(TransferInformation*)), this, SLOT(updateTransfer(TransferInformation*)));
	connect(m_model, SIGNAL(modelReset()), this, SLOT(updateActions()));
	connect(m_ui->transfersView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(updateActions()));
	connect(m_ui->transfersView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(openTransfer(QModelIndex)));
	connect(m_ui->transfersView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(m_ui->downloadLineEdit, SIGNAL(returnPressed()), this, SLOT(startQuickTransfer()));
	connect(m_ui->stopResumeButton, SIGNAL(clicked()), this, SLOT(stopResumeTransfer()));
	connect(m_ui->redownloadButton, SIGNAL(clicked()), this, SLOT(redownloadTransfer()));
}

TransfersContentsWidget::~TransfersContentsWidget()
{
	delete m_ui;
}

void TransfersContentsWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			m_ui->retranslateUi(this);

			break;
		default:
			break;
	}
}

void TransfersContentsWidget::triggerAction()
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (action)
	{
		triggerAction(static_cast<WindowAction>(action->data().toInt()));
	}
}

void TransfersContentsWidget::addTransfer(TransferInformation *transfer)
{
	QList<QStandardItem*> items;
	QStandardItem *item = new QStandardItem();
	item->setData(qVariantFromValue((void*) transfer), Qt::UserRole);

	items.append(item);

	item = new QStandardItem(QFileInfo(transfer->target).fileName());

	items.append(item);

	for (int i = 2; i < m_model->columnCount(); ++i)
	{
		items.append(new QStandardItem());
	}

	m_model->appendRow(items);

	if (transfer->state == RunningTransfer)
	{
		m_speeds[transfer] = QQueue<qint64>();
	}

	updateTransfer(transfer);
}

void TransfersContentsWidget::removeTransfer(TransferInformation *transfer)
{
	const int row = findTransfer(transfer);

	if (row >= 0)
	{
		m_model->removeRow(row);
	}

	m_speeds.remove(transfer);
}

void TransfersContentsWidget::removeTransfer()
{
	TransferInformation *transfer = getTransfer(m_ui->transfersView->currentIndex());

	if (transfer)
	{
		if (transfer->state == RunningTransfer && QMessageBox::warning(this, tr("Warning"), tr("This transfer is still running.\nDo you really want to remove it?"), (QMessageBox::Yes | QMessageBox::Cancel)) == QMessageBox::Cancel)
		{
			return;
		}

		TransfersManager::removeTransfer(transfer);
	}
}

void TransfersContentsWidget::updateTransfer(TransferInformation *transfer)
{
	const int row = findTransfer(transfer);

	if (row < 0)
	{
		return;
	}

	QString remainingTime;

	if (transfer->state == RunningTransfer)
	{
		if (!m_speeds.contains(transfer))
		{
			m_speeds[transfer] = QQueue<qint64>();
		}

		m_speeds[transfer].enqueue(transfer->speed);

		if (m_speeds[transfer].count() > 10)
		{
			m_speeds[transfer].dequeue();
		}

		if (transfer->bytesTotal > 0)
		{
			qint64 speedSum = 0;
			const QList<qint64> speeds = m_speeds[transfer];

			for (int i = 0; i < speeds.count(); ++i)
			{
				speedSum += speeds.at(i);
			}

			speedSum /= (speeds.count());

			remainingTime = Utils::formatTime(qreal(transfer->bytesTotal - transfer->bytesReceived) / speedSum);
		}
	}
	else
	{
		m_speeds.remove(transfer);
	}

	QIcon icon;

	switch (transfer->state)
	{
		case RunningTransfer:
			icon = Utils::getIcon(QLatin1String("task-ongoing"));

			break;
		case FinishedTransfer:
			icon = Utils::getIcon(QLatin1String("task-complete"));

			break;
		case ErrorTransfer:
			icon = Utils::getIcon(QLatin1String("task-reject"));

			break;
		default:
			break;
	}

	m_model->item(row, 0)->setIcon(icon);
	m_model->item(row, 2)->setText(Utils::formatUnit(transfer->bytesTotal, false, 1));
	m_model->item(row, 3)->setText((transfer->bytesTotal > 0) ? QString::number(qFloor(((qreal) transfer->bytesReceived / transfer->bytesTotal) * 100), 'f', 0) : QString());
	m_model->item(row, 4)->setText(remainingTime);
	m_model->item(row, 5)->setText((transfer->state == RunningTransfer) ? Utils::formatUnit(transfer->speed, true, 1) : QString());
	m_model->item(row, 6)->setText(transfer->started.toString(QLatin1String("yyyy-MM-dd HH:mm:ss")));
	m_model->item(row, 7)->setText(transfer->finished.toString(QLatin1String("yyyy-MM-dd HH:mm:ss")));

	const QString tooltip = tr("<pre style='font-family:auto;'>Source: %1\nTarget: %2\nSize: %3\nDownloaded: %4\nProgress: %5</pre>").arg(transfer->source.toHtmlEscaped()).arg(transfer->target.toHtmlEscaped()).arg((transfer->bytesTotal > 0) ? tr("%1 (%n B)", "", transfer->bytesTotal).arg(Utils::formatUnit(transfer->bytesTotal)) : QString('?')).arg(tr("%1 (%n B)", "", transfer->bytesReceived).arg(Utils::formatUnit(transfer->bytesReceived))).arg(QStringLiteral("%1%").arg(((transfer->bytesTotal > 0) ? (((qreal) transfer->bytesReceived / transfer->bytesTotal) * 100) : 0.0), 0, 'f', 1));

	for (int i = 0; i < m_model->columnCount(); ++i)
	{
		m_model->item(row, i)->setToolTip(tooltip);
	}

	if (m_ui->transfersView->selectionModel()->hasSelection())
	{
		updateActions();
	}

	const bool isRunning = (transfer && transfer->state == RunningTransfer);

	if (isRunning != m_isLoading)
	{
		if (isRunning)
		{
			m_isLoading = true;

			emit loadingChanged(true);
		}
		else
		{
			const QList<TransferInformation*> transfers = TransfersManager::getTransfers();
			bool hasRunning = false;

			for (int i = 0; i < transfers.count(); ++i)
			{
				if (transfers.at(i) && transfers.at(i)->state == RunningTransfer)
				{
					hasRunning = true;

					break;
				}
			}

			if (!hasRunning)
			{
				m_isLoading = false;

				emit loadingChanged(false);
			}
		}
	}
}

void TransfersContentsWidget::openTransfer(const QModelIndex &index)
{
	TransferInformation *transfer = getTransfer(index.isValid() ? index : m_ui->transfersView->currentIndex());

	if (transfer)
	{
		QDesktopServices::openUrl(QUrl::fromLocalFile(transfer->target));
	}
}

void TransfersContentsWidget::openTransferFolder(const QModelIndex &index)
{
	TransferInformation *transfer = getTransfer(index.isValid() ? index : m_ui->transfersView->currentIndex());

	if (transfer)
	{
		QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(transfer->target).dir().canonicalPath()));
	}
}

void TransfersContentsWidget::copyTransferInformation()
{
	QStandardItem *item = m_model->itemFromIndex(m_ui->transfersView->currentIndex());

	if (item)
	{
		QApplication::clipboard()->setText(item->toolTip().remove(QRegularExpression(QLatin1String("<[^>]*>"))));
	}
}

void TransfersContentsWidget::stopResumeTransfer()
{
	TransferInformation *transfer = getTransfer(m_ui->transfersView->selectionModel()->hasSelection() ? m_ui->transfersView->selectionModel()->currentIndex() : QModelIndex());

	if (transfer)
	{
		if (transfer->state == RunningTransfer)
		{
			TransfersManager::stopTransfer(transfer);
		}
		else if (transfer->state == ErrorTransfer)
		{
			TransfersManager::resumeTransfer(transfer);
		}

		updateActions();
	}
}

void TransfersContentsWidget::redownloadTransfer()
{
	TransferInformation *transfer = getTransfer(m_ui->transfersView->selectionModel()->hasSelection() ? m_ui->transfersView->selectionModel()->currentIndex() : QModelIndex());

	if (transfer)
	{
		TransfersManager::restartTransfer(transfer);
	}
}

void TransfersContentsWidget::startQuickTransfer()
{
	TransfersManager::startTransfer(m_ui->downloadLineEdit->text(), QString(), false, true);

	m_ui->downloadLineEdit->clear();
}

void TransfersContentsWidget::clearFinishedTransfers()
{
	TransfersManager::clearTransfers();
}

void TransfersContentsWidget::showContextMenu(const QPoint &point)
{
	TransferInformation *transfer = getTransfer(m_ui->transfersView->indexAt(point));
	QMenu menu(this);

	if (transfer)
	{
		menu.addAction(tr("Open"), this, SLOT(openTransfer()));
		menu.addAction(tr("Open Folder"), this, SLOT(openTransferFolder()));
		menu.addSeparator();
		menu.addAction(((transfer->state == ErrorTransfer) ? tr("Resume") : tr("Stop")), this, SLOT(stopResumeTransfer()))->setEnabled(transfer->state == RunningTransfer || transfer->state == ErrorTransfer);
		menu.addAction("Redownload", this, SLOT(redownloadTransfer()));
		menu.addSeparator();
		menu.addAction(tr("Copy Transfer Information"), this, SLOT(copyTransferInformation()));
		menu.addSeparator();
		menu.addAction(tr("Remove"), this, SLOT(removeTransfer()));
	}

	const QList<TransferInformation*> transfers = TransfersManager::getTransfers();
	int finishedTransfers = 0;

	for (int i = 0; i < transfers.count(); ++i)
	{
		if (transfers.at(i)->state == FinishedTransfer)
		{
			++finishedTransfers;
		}
	}

	menu.addAction(tr("Clear Finished Transfers"), this, SLOT(clearFinishedTransfers()))->setEnabled(finishedTransfers > 0);
	menu.exec(m_ui->transfersView->mapToGlobal(point));
}

void TransfersContentsWidget::updateActions()
{
	TransferInformation *transfer = getTransfer(m_ui->transfersView->selectionModel()->hasSelection() ? m_ui->transfersView->selectionModel()->currentIndex() : QModelIndex());

	if (transfer && transfer->state == ErrorTransfer)
	{
		m_ui->stopResumeButton->setText(tr("Resume"));
		m_ui->stopResumeButton->setIcon(Utils::getIcon(QLatin1String("task-ongoing")));
	}
	else
	{
		m_ui->stopResumeButton->setText(tr("Stop"));
		m_ui->stopResumeButton->setIcon(Utils::getIcon(QLatin1String("task-reject")));
	}

	m_ui->stopResumeButton->setEnabled(transfer && (transfer->state == RunningTransfer || transfer->state == ErrorTransfer));
	m_ui->redownloadButton->setEnabled(transfer);

	getAction(CopyAction)->setEnabled(transfer);
	getAction(DeleteAction)->setEnabled(transfer);

	if (transfer)
	{
		m_ui->sourceLabelWidget->setText(transfer->source);
		m_ui->targetLabelWidget->setText(transfer->target);
		m_ui->sizeLabelWidget->setText((transfer->bytesTotal > 0) ? tr("%1 (%n B)", "", transfer->bytesTotal).arg(Utils::formatUnit(transfer->bytesTotal)) : QString('?'));
		m_ui->downloadedLabelWidget->setText(tr("%1 (%n B)", "", transfer->bytesReceived).arg(Utils::formatUnit(transfer->bytesReceived)));
		m_ui->progressLabelWidget->setText(QStringLiteral("%1%").arg(((transfer->bytesTotal > 0) ? (((qreal) transfer->bytesReceived / transfer->bytesTotal) * 100) : 0.0), 0, 'f', 1));
	}
	else
	{
		m_ui->sourceLabelWidget->clear();
		m_ui->targetLabelWidget->clear();
		m_ui->sizeLabelWidget->clear();
		m_ui->downloadedLabelWidget->clear();
		m_ui->progressLabelWidget->clear();
	}

	emit actionsChanged();
}

void TransfersContentsWidget::print(QPrinter *printer)
{
	m_ui->transfersView->render(printer);
}

void TransfersContentsWidget::triggerAction(WindowAction action, bool checked)
{
	Q_UNUSED(checked)

	switch (action)
	{
		case CopyAction:
			copyTransferInformation();

			break;
		case DeleteAction:
			removeTransfer();

			break;
		default:
			break;
	}
}

TransferInformation* TransfersContentsWidget::getTransfer(const QModelIndex &index)
{
	if (index.isValid() && m_model->item(index.row(), 0))
	{
		return static_cast<TransferInformation*>(m_model->item(index.row(), 0)->data(Qt::UserRole).value<void*>());
	}

	return NULL;
}

QAction* TransfersContentsWidget::getAction(WindowAction action)
{
	if (m_actions.contains(action))
	{
		return m_actions[action];
	}

	QAction *actionObject = new QAction(this);
	actionObject->setData(action);

	connect(actionObject, SIGNAL(triggered()), this, SLOT(triggerAction()));

	switch (action)
	{
		case CopyAction:
			ActionsManager::setupLocalAction(actionObject, QLatin1String("Copy"));

			break;
		case DeleteAction:
			ActionsManager::setupLocalAction(actionObject, QLatin1String("Delete"));

			break;
		default:
			actionObject->deleteLater();
			actionObject = NULL;

			break;
	}

	if (actionObject)
	{
		m_actions[action] = actionObject;
	}

	return actionObject;
}

QString TransfersContentsWidget::getTitle() const
{
	return tr("Transfers Manager");
}

QLatin1String TransfersContentsWidget::getType() const
{
	return QLatin1String("transfers");
}

QUrl TransfersContentsWidget::getUrl() const
{
	return QUrl(QLatin1String("about:transfers"));
}

QIcon TransfersContentsWidget::getIcon() const
{
	return Utils::getIcon(QLatin1String("transfers"), false);
}

int TransfersContentsWidget::findTransfer(TransferInformation *transfer) const
{
	if (!transfer)
	{
		return -1;
	}

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		if (transfer == static_cast<TransferInformation*>(m_model->item(i, 0)->data(Qt::UserRole).value<void*>()))
		{
			return i;
		}
	}

	return -1;
}

bool TransfersContentsWidget::isLoading() const
{
	return m_isLoading;
}

bool TransfersContentsWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_ui->transfersView && event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

		if (keyEvent && (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return))
		{
			openTransfer();

			return true;
		}
	}

	return QWidget::eventFilter(object, event);
}

}
