/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "TransfersContentsWidget.h"
#include "../../../core/ThemesManager.h"
#include "../../../core/TransfersManager.h"
#include "../../../core/Utils.h"
#include "../../../ui/Menu.h"
#include "../../../ui/ProgressBarWidget.h"

#include "ui_TransfersContentsWidget.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QtMath>
#include <QtGui/QClipboard>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>

namespace Otter
{

ProgressBarDelegate::ProgressBarDelegate(QObject *parent) : ItemDelegate(parent)
{
}

void ProgressBarDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	ProgressBarWidget *progressBar(editor->findChild<ProgressBarWidget*>());

	if (!progressBar)
	{
		return;
	}

	const Transfer::TransferState state(static_cast<Transfer::TransferState>(index.data(TransfersContentsWidget::StateRole).toInt()));
	const bool isIndeterminate(index.data(TransfersContentsWidget::BytesTotalRole).toLongLong() <= 0);
	const bool hasError(state == Transfer::UnknownState || state == Transfer::ErrorState);

	progressBar->setHasError(hasError);
	progressBar->setRange(0, ((isIndeterminate && !hasError) ? 0 : 100));
	progressBar->setValue(isIndeterminate ? (hasError ? 0 : -1) : index.data(TransfersContentsWidget::ProgressRole).toInt());
	progressBar->setFormat(isIndeterminate ? tr("Unknown") : QLatin1String("%p%"));
}

QWidget* ProgressBarDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	QWidget *widget(new QWidget(parent));
	QHBoxLayout *layout(new QHBoxLayout(widget));
	layout->setContentsMargins(0, 3, 0, 3);

	widget->setLayout(layout);

	ProgressBarWidget *editor(new ProgressBarWidget(widget));
	editor->setAlignment(Qt::AlignCenter);

	layout->addWidget(editor);

	setEditorData(editor, index);

	return widget;
}

TransfersContentsWidget::TransfersContentsWidget(const QVariantMap &parameters, Window *window, QWidget *parent) : ContentsWidget(parameters, window, parent),
	m_model(new QStandardItemModel(this)),
	m_isLoading(false),
	m_ui(new Ui::TransfersContentsWidget)
{
	m_ui->setupUi(this);

	m_model->setHorizontalHeaderLabels({tr("Status"), tr("Filename"), tr("Size"), tr("Progress"), tr("Time"), tr("Speed"), tr("Started"), tr("Finished")});
	m_model->setHeaderData(0, Qt::Horizontal, 28, HeaderViewWidget::WidthRole);
	m_model->setHeaderData(1, Qt::Horizontal, 500, HeaderViewWidget::WidthRole);
	m_model->setHeaderData(2, Qt::Horizontal, 150, HeaderViewWidget::WidthRole);
	m_model->setHeaderData(3, Qt::Horizontal, 250, HeaderViewWidget::WidthRole);
	m_model->setHeaderData(4, Qt::Horizontal, 150, HeaderViewWidget::WidthRole);
	m_model->setHeaderData(5, Qt::Horizontal, 150, HeaderViewWidget::WidthRole);

	m_ui->transfersViewWidget->setModel(m_model);
	m_ui->transfersViewWidget->setItemDelegateForColumn(3, new ProgressBarDelegate(this));
	m_ui->transfersViewWidget->setSortRoleMapping({{0, StateRole}, {2, BytesTotalRole}, {3, ProgressRole}, {6, TimeStartedRole}, {7, TimeFinishedRole}});
	m_ui->transfersViewWidget->installEventFilter(this);
	m_ui->stopResumeButton->setIcon(ThemesManager::createIcon(QLatin1String("task-reject")));
	m_ui->redownloadButton->setIcon(ThemesManager::createIcon(QLatin1String("view-refresh")));
	m_ui->downloadLineEditWidget->installEventFilter(this);

	const QVector<Transfer*> transfers(TransfersManager::getTransfers());

	for (int i = 0; i < transfers.count(); ++i)
	{
		handleTransferAdded(transfers.at(i));
	}

	if (isSidebarPanel())
	{
		m_ui->detailsWidget->hide();
	}

	connect(TransfersManager::getInstance(), &TransfersManager::transferStarted, this, &TransfersContentsWidget::handleTransferAdded);
	connect(TransfersManager::getInstance(), &TransfersManager::transferRemoved, this, [&](Transfer *transfer)
	{
		const int row(findTransferRow(transfer));

		if (row >= 0)
		{
			m_model->removeRow(row);
		}
	});
	connect(TransfersManager::getInstance(), &TransfersManager::transferChanged, this, &TransfersContentsWidget::handleTransferChanged);
	connect(m_model, &QStandardItemModel::modelReset, this, &TransfersContentsWidget::updateActions);
	connect(m_ui->transfersViewWidget, &ItemViewWidget::doubleClicked, this, &TransfersContentsWidget::openTransfer);
	connect(m_ui->transfersViewWidget, &ItemViewWidget::customContextMenuRequested, this, &TransfersContentsWidget::showContextMenu);
	connect(m_ui->transfersViewWidget, &ItemViewWidget::needsActionsUpdate, this, &TransfersContentsWidget::updateActions);
	connect(m_ui->downloadLineEditWidget, &LineEditWidget::returnPressed, this, [&]()
	{
		TransfersManager::startTransfer(m_ui->downloadLineEditWidget->text(), {}, (Transfer::CanNotifyOption | Transfer::IsQuickTransferOption | (SessionsManager::isPrivate() ? Transfer::IsPrivateOption : Transfer::NoOption)));

		m_ui->downloadLineEditWidget->clear();
	});
	connect(m_ui->stopResumeButton, &QPushButton::clicked, this, &TransfersContentsWidget::stopResumeTransfer);
	connect(m_ui->redownloadButton, &QPushButton::clicked, this, &TransfersContentsWidget::redownloadTransfer);
}

TransfersContentsWidget::~TransfersContentsWidget()
{
	delete m_ui;
}

void TransfersContentsWidget::changeEvent(QEvent *event)
{
	ContentsWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);

		m_model->setHorizontalHeaderLabels({tr("Status"), tr("Filename"), tr("Size"), tr("Progress"), tr("Time"), tr("Speed"), tr("Started"), tr("Finished")});

		updateActions();
	}
}

void TransfersContentsWidget::removeTransfer()
{
	Transfer *transfer(getTransfer(m_ui->transfersViewWidget->currentIndex()));

	if (!transfer || (transfer->getState() == Transfer::RunningState && QMessageBox::warning(this, tr("Warning"), tr("This file is still being downloaded.\nDo you really want to remove it?"), (QMessageBox::Yes | QMessageBox::Cancel)) == QMessageBox::Cancel))
	{
		return;
	}

	m_model->removeRow(m_ui->transfersViewWidget->currentIndex().row());

	TransfersManager::removeTransfer(transfer);
}

void TransfersContentsWidget::openTransfer()
{
	const Transfer *transfer(getTransfer(m_ui->transfersViewWidget->currentIndex()));

	if (transfer)
	{
		transfer->openTarget();
	}
}

void TransfersContentsWidget::copyTransferInformation()
{
	const QModelIndex index(m_ui->transfersViewWidget->currentIndex());

	if (index.isValid() && index.data(Qt::ToolTipRole).isValid())
	{
		QGuiApplication::clipboard()->setText(index.data(Qt::ToolTipRole).toString().remove(QRegularExpression(QLatin1String("<[^>]*>"))));
	}
}

void TransfersContentsWidget::stopResumeTransfer()
{
	Transfer *transfer(getTransfer(m_ui->transfersViewWidget->getCurrentIndex()));

	if (!transfer)
	{
		return;
	}

	switch (transfer->getState())
	{
		case Transfer::RunningState:
			transfer->stop();

			break;
		case Transfer::ErrorState:
			transfer->resume();

			break;
		default:
			break;
	}

	updateActions();
}

void TransfersContentsWidget::redownloadTransfer()
{
	Transfer *transfer(getTransfer(m_ui->transfersViewWidget->getCurrentIndex()));

	if (transfer)
	{
		transfer->restart();
	}
}

void TransfersContentsWidget::handleTransferAdded(Transfer *transfer)
{
	QList<QStandardItem*> items({new QStandardItem(), new QStandardItem(QFileInfo(transfer->getTarget()).fileName())});
	items[0]->setData(QVariant::fromValue(static_cast<void*>(transfer)), InstanceRole);
	items[0]->setFlags(items[0]->flags() | Qt::ItemNeverHasChildren);
	items[1]->setFlags(items[1]->flags() | Qt::ItemNeverHasChildren);
	items.reserve(m_model->columnCount());

	for (int i = 2; i < m_model->columnCount(); ++i)
	{
		QStandardItem *item(new QStandardItem());
		item->setFlags(item->flags() | Qt::ItemNeverHasChildren);

		items.append(item);
	}

	m_model->appendRow(items);

	m_ui->transfersViewWidget->openPersistentEditor(items[ProgressColumn]->index());

	handleTransferChanged(transfer);
}

void TransfersContentsWidget::handleTransferChanged(Transfer *transfer)
{
	const int row(findTransferRow(transfer));

	if (row < 0)
	{
		return;
	}

	QString iconName(QLatin1String("task-reject"));
	const bool isIndeterminate(transfer->getBytesTotal() <= 0);

	switch (transfer->getState())
	{
		case Transfer::RunningState:
			iconName = QLatin1String("task-ongoing");

			break;
		case Transfer::FinishedState:
			iconName = QLatin1String("task-complete");

			break;
		default:
			break;
	}

	const QString toolTip(tr("<div style=\"white-space:pre;\">Source: %1\nTarget: %2\nSize: %3\nDownloaded: %4\nProgress: %5</div>").arg(transfer->getSource().toDisplayString().toHtmlEscaped(), transfer->getTarget().toHtmlEscaped(), (isIndeterminate ? tr("Unknown") : Utils::formatUnit(transfer->getBytesTotal(), false, 1, true)), Utils::formatUnit(transfer->getBytesReceived(), false, 1, true), (isIndeterminate ? tr("Unknown") : QStringLiteral("%1%").arg(Utils::calculatePercent(transfer->getBytesReceived(), transfer->getBytesTotal()), 0, 'f', 1))));

	for (int i = 0; i < m_model->columnCount(); ++i)
	{
		const QModelIndex &index(m_model->index(row, i));

		m_model->setData(index, toolTip, Qt::ToolTipRole);

		switch (i)
		{
			case DecorationColumn:
				m_model->setData(index, ThemesManager::createIcon(iconName), Qt::DecorationRole);
				m_model->setData(index, transfer->getState(), StateRole);

				break;
			case FilenameColumn:
				m_model->setData(index, QFileInfo(transfer->getTarget()).fileName(), Qt::DisplayRole);

				break;
			case SizeColumn:
				m_model->setData(index, Utils::formatUnit(transfer->getBytesTotal(), false, 1), Qt::DisplayRole);
				m_model->setData(index, transfer->getBytesTotal(), BytesTotalRole);

				break;
			case ProgressColumn:
				m_model->setData(index, transfer->getBytesReceived(), BytesReceivedRole);
				m_model->setData(index, transfer->getBytesTotal(), BytesTotalRole);
				m_model->setData(index, ((transfer->getBytesTotal() > 0) ? qFloor(Utils::calculatePercent(transfer->getBytesReceived(), transfer->getBytesTotal())) : -1), ProgressRole);
				m_model->setData(index, transfer->getState(), StateRole);

				break;
			case TimeElapsedColumn:
				m_model->setData(index, ((isIndeterminate || transfer->getRemainingTime() <= 0) ? QString() : Utils::formatElapsedTime(transfer->getRemainingTime())), Qt::DisplayRole);

				break;
			case SpeedColumn:
				m_model->setData(index, ((transfer->getState() == Transfer::RunningState) ? Utils::formatUnit(transfer->getSpeed(), true, 1) : QString()), Qt::DisplayRole);

				break;
			case TimeStartedColumn:
				m_model->setData(index, Utils::formatDateTime(transfer->getTimeStarted()), Qt::DisplayRole);
				m_model->setData(index, transfer->getTimeStarted(), TimeStartedRole);

				break;
			case TimeFinishedColumn:
				m_model->setData(index, Utils::formatDateTime(transfer->getTimeFinished()), Qt::DisplayRole);
				m_model->setData(index, transfer->getTimeFinished(), TimeFinishedRole);

				break;
			default:
				break;
		}
	}

	if (m_ui->transfersViewWidget->hasSelection())
	{
		updateActions();
	}

	const bool isRunning(transfer && transfer->getState() == Transfer::RunningState);

	if (isRunning == m_isLoading)
	{
		return;
	}

	if (isRunning)
	{
		m_isLoading = true;

		emit loadingStateChanged(WebWidget::OngoingLoadingState);
	}
	else if (!TransfersManager::hasRunningTransfers())
	{
		m_isLoading = false;

		emit loadingStateChanged(WebWidget::FinishedLoadingState);
	}
}

void TransfersContentsWidget::showContextMenu(const QPoint &position)
{
	const Transfer *transfer(getTransfer(m_ui->transfersViewWidget->indexAt(position)));
	QMenu menu(this);

	if (transfer)
	{
		const bool canOpen(QFile::exists(transfer->getTarget()));

		menu.addAction(ThemesManager::createIcon(QLatin1String("document-open")), QCoreApplication::translate("actions", "Open"), this, &TransfersContentsWidget::openTransfer)->setEnabled(canOpen);

		Menu *openWithMenu(new Menu(Menu::OpenInApplicationMenu, this));
		openWithMenu->setEnabled(canOpen);
		openWithMenu->setExecutor(ActionExecutor::Object(this, this));
		openWithMenu->setActionParameters({{QLatin1String("url"), transfer->getTarget()}});
		openWithMenu->setMenuOptions({{QLatin1String("mimeType"), transfer->getMimeType().name()}});

		menu.addMenu(openWithMenu);
		menu.addAction(tr("Open Folder"), this, [&]()
		{
			const Transfer *transfer(getTransfer(m_ui->transfersViewWidget->currentIndex()));

			if (transfer)
			{
				Utils::runApplication({}, QUrl::fromLocalFile(QFileInfo(transfer->getTarget()).dir().canonicalPath()));
			}
		})->setEnabled(canOpen || QFileInfo(transfer->getTarget()).dir().exists());
		menu.addSeparator();
		menu.addAction(((transfer->getState() == Transfer::ErrorState) ? tr("Resume") : tr("Stop")), this, &TransfersContentsWidget::stopResumeTransfer)->setEnabled(transfer->getState() == Transfer::RunningState || transfer->getState() == Transfer::ErrorState);
		menu.addAction(tr("Redownload"), this, &TransfersContentsWidget::redownloadTransfer);
		menu.addSeparator();
		menu.addAction(tr("Copy Transfer Information"), this, &TransfersContentsWidget::copyTransferInformation);
		menu.addSeparator();
		menu.addAction(ThemesManager::createIcon(QLatin1String("edit-delete")), tr("Remove"), this, &TransfersContentsWidget::removeTransfer);
		menu.addSeparator();
	}

	const QVector<Transfer*> transfers(TransfersManager::getTransfers());
	int finishedTransfers(0);

	for (int i = 0; i < transfers.count(); ++i)
	{
		if (transfers.at(i)->getState() == Transfer::FinishedState)
		{
			++finishedTransfers;
		}
	}

	menu.addAction(ThemesManager::createIcon(QLatin1String("edit-clear-history")), tr("Clear Finished Transfers"), &TransfersManager::clearTransfers)->setEnabled(finishedTransfers > 0);
	menu.exec(m_ui->transfersViewWidget->mapToGlobal(position));
}

void TransfersContentsWidget::updateActions()
{
	const Transfer *transfer(getTransfer(m_ui->transfersViewWidget->getCurrentIndex()));

	if (transfer && transfer->getState() == Transfer::ErrorState)
	{
		m_ui->stopResumeButton->setText(tr("Resume"));
		m_ui->stopResumeButton->setIcon(ThemesManager::createIcon(QLatin1String("task-ongoing")));
	}
	else
	{
		m_ui->stopResumeButton->setText(tr("Stop"));
		m_ui->stopResumeButton->setIcon(ThemesManager::createIcon(QLatin1String("task-reject")));
	}

	m_ui->stopResumeButton->setEnabled(transfer && (transfer->getState() == Transfer::RunningState || transfer->getState() == Transfer::ErrorState));
	m_ui->redownloadButton->setEnabled(transfer);

	if (transfer)
	{
		const bool isIndeterminate(transfer->getBytesTotal() <= 0);

		m_ui->sourceLabelWidget->setText(transfer->getSource().toDisplayString());
		m_ui->sourceLabelWidget->setUrl(transfer->getSource());
		m_ui->targetLabelWidget->setText(transfer->getTarget());
		m_ui->targetLabelWidget->setUrl(QUrl(transfer->getTarget()));
		m_ui->sizeLabelWidget->setText(isIndeterminate ? tr("Unknown") : Utils::formatUnit(transfer->getBytesTotal(), false, 1, true));
		m_ui->downloadedLabelWidget->setText(Utils::formatUnit(transfer->getBytesReceived(), false, 1, true));
		m_ui->progressLabelWidget->setText(isIndeterminate ? tr("Unknown") : QStringLiteral("%1%").arg(Utils::calculatePercent(transfer->getBytesReceived(), transfer->getBytesTotal()), 0, 'f', 1));
	}
	else
	{
		m_ui->sourceLabelWidget->clear();
		m_ui->targetLabelWidget->clear();
		m_ui->sizeLabelWidget->clear();
		m_ui->downloadedLabelWidget->clear();
		m_ui->progressLabelWidget->clear();
	}

	emit categorizedActionsStateChanged({ActionsManager::ActionDefinition::EditingCategory});
}

void TransfersContentsWidget::print(QPrinter *printer)
{
	m_ui->transfersViewWidget->render(printer);
}

void TransfersContentsWidget::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
{
	switch (identifier)
	{
		case ActionsManager::CopyAction:
			if (m_ui->transfersViewWidget->hasFocus() && m_ui->transfersViewWidget->currentIndex().isValid())
			{
				copyTransferInformation();
			}
			else
			{
				const QWidget *widget(focusWidget());

				if (widget->metaObject()->className() == QLatin1String("Otter::TextLabelWidget"))
				{
					const TextLabelWidget *label(qobject_cast<const TextLabelWidget*>(widget));

					if (label)
					{
						label->copy();
					}
				}
			}

			break;
		case ActionsManager::DeleteAction:
			removeTransfer();

			break;
		case ActionsManager::FindAction:
		case ActionsManager::QuickFindAction:
			m_ui->downloadLineEditWidget->setFocus();
			m_ui->downloadLineEditWidget->selectAll();

			break;
		case ActionsManager::ActivateContentAction:
			m_ui->transfersViewWidget->setFocus();

			break;
		default:
			ContentsWidget::triggerAction(identifier, parameters, trigger);

			break;
	}
}

Transfer* TransfersContentsWidget::getTransfer(const QModelIndex &index) const
{
	if (index.isValid())
	{
		return static_cast<Transfer*>(index.sibling(index.row(), 0).data(InstanceRole).value<void*>());
	}

	return nullptr;
}

QString TransfersContentsWidget::getTitle() const
{
	return tr("Downloads");
}

QLatin1String TransfersContentsWidget::getType() const
{
	return QLatin1String("transfers");
}

QUrl TransfersContentsWidget::getUrl() const
{
	return {QLatin1String("about:transfers")};
}

QIcon TransfersContentsWidget::getIcon() const
{
	return ThemesManager::createIcon(QLatin1String("transfers"), false);
}

ActionsManager::ActionDefinition::State TransfersContentsWidget::getActionState(int identifier, const QVariantMap &parameters) const
{
	switch (identifier)
	{
		case ActionsManager::CopyAction:
		case ActionsManager::DeleteAction:
			{
				ActionsManager::ActionDefinition::State state(ActionsManager::getActionDefinition(identifier).getDefaultState());
				state.isEnabled = (getTransfer(m_ui->transfersViewWidget->getCurrentIndex()) != nullptr);

				return state;
			}
		default:
			break;
	}

	return ContentsWidget::getActionState(identifier, parameters);
}

WebWidget::LoadingState TransfersContentsWidget::getLoadingState() const
{
	return (m_isLoading ? WebWidget::OngoingLoadingState : WebWidget::FinishedLoadingState);
}

int TransfersContentsWidget::findTransferRow(Transfer *transfer) const
{
	if (!transfer)
	{
		return -1;
	}

	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		if (transfer == getTransfer(m_model->index(i, 0)))
		{
			return i;
		}
	}

	return -1;
}

bool TransfersContentsWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_ui->transfersViewWidget && event->type() == QEvent::KeyPress)
	{
		switch (static_cast<QKeyEvent*>(event)->key())
		{
			case Qt::Key_Delete:
				removeTransfer();

				return true;
			case Qt::Key_Enter:
			case Qt::Key_Return:
				openTransfer();

				return true;
			default:
				break;
		}
	}
	else if (object == m_ui->downloadLineEditWidget && event->type() == QEvent::KeyPress && static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape)
	{
		m_ui->downloadLineEditWidget->clear();
	}

	return ContentsWidget::eventFilter(object, event);
}

}
