#include "TransfersContentsWidget.h"
#include "../../../core/Utils.h"

#include "ui_TransfersContentsWidget.h"

#include <QtCore/QFileInfo>

namespace Otter
{

TransfersContentsWidget::TransfersContentsWidget(Window *window) : ContentsWidget(window),
	m_model(new QStandardItemModel(this)),
	m_ui(new Ui::TransfersContentsWidget)
{
	m_ui->setupUi(this);

	QStringList labels;
	labels << tr("Filename") << tr("Size") << tr("Progress") << tr("Time") << tr("Speed") << tr("Started") << tr("Finished");

	m_model->setHorizontalHeaderLabels(labels);

	m_ui->transfersView->setModel(m_model);
	m_ui->transfersView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

	const QList<TransferInformation*> transfers = TransfersManager::getTransfers();

	for (int i = 0; i < transfers.count(); ++i)
	{
		addTransfer(transfers.at(i));
	}

	connect(m_ui->transfersView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(updateActions()));
	connect(m_ui->stopResumeButton, SIGNAL(clicked()), this, SLOT(stopResumeTransfer()));
	connect(TransfersManager::getInstance(), SIGNAL(transferStarted(TransferInformation*)), this, SLOT(addTransfer(TransferInformation*)));
	connect(TransfersManager::getInstance(), SIGNAL(transferRemoved(TransferInformation*)), this, SLOT(removeTransfer(TransferInformation*)));
	connect(TransfersManager::getInstance(), SIGNAL(transferUpdated(TransferInformation*)), this, SLOT(updateTransfer(TransferInformation*)));
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

void TransfersContentsWidget::addTransfer(TransferInformation *transfer)
{
	QList<QStandardItem*> items;
	QStandardItem *item = new QStandardItem(QFileInfo(transfer->target).fileName());
	item->setData(qVariantFromValue((void*) transfer), Qt::UserRole);
	item->setToolTip(transfer->target);

	items.append(item);

	for (int i = 1; i < m_model->columnCount(); ++i)
	{
		items.append(new QStandardItem());
	}

	m_model->appendRow(items);

	updateTransfer(transfer);
}

void TransfersContentsWidget::removeTransfer(TransferInformation *transfer)
{
	const int row = findTransfer(transfer);

	if (row >= 0)
	{
		m_model->removeRow(row);
	}
}

void TransfersContentsWidget::updateTransfer(TransferInformation *transfer)
{
	const int row = findTransfer(transfer);

	if (row >= 0)
	{
		m_model->item(row, 1)->setText(Utils::formatUnit(transfer->bytesTotal, false, 1));
		m_model->item(row, 2)->setText((transfer->bytesTotal > 0) ? QString::number(((qreal) transfer->bytesReceived / transfer->bytesTotal) * 100) : QString());
		m_model->item(row, 4)->setText(Utils::formatUnit(transfer->speed, true, 1));
		m_model->item(row, 5)->setText(transfer->started.toString("yyyy-MM-dd HH:mm:ss"));
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

void TransfersContentsWidget::updateActions()
{
	TransferInformation *transfer = getTransfer(m_ui->transfersView->selectionModel()->hasSelection() ? m_ui->transfersView->selectionModel()->currentIndex() : QModelIndex());

	m_ui->stopResumeButton->setEnabled(transfer && (transfer->state == RunningTransfer || transfer->state == ErrorTransfer));
	m_ui->stopResumeButton->setText((transfer && transfer->state == ErrorTransfer) ? tr("Resume") : tr("Stop"));
}

void TransfersContentsWidget::print(QPrinter *printer)
{
	m_ui->transfersView->render(printer);
}

void TransfersContentsWidget::triggerAction(WindowAction action, bool checked)
{
	Q_UNUSED(action)
	Q_UNUSED(checked)
}

void TransfersContentsWidget::setHistory(const HistoryInformation &history)
{
	Q_UNUSED(history)
}

void TransfersContentsWidget::setZoom(int zoom)
{
	Q_UNUSED(zoom)
}

void TransfersContentsWidget::setUrl(const QUrl &url)
{
	Q_UNUSED(url)
}

TransferInformation* TransfersContentsWidget::getTransfer(const QModelIndex &index)
{
	if (index.isValid() && m_model->item(index.row(), 0))
	{
		return static_cast<TransferInformation*>(m_model->item(index.row(), 0)->data(Qt::UserRole).value<void*>());
	}

	return NULL;
}

ContentsWidget* TransfersContentsWidget::clone(Window *window)
{
	Q_UNUSED(window)

	return NULL;
}

QAction* TransfersContentsWidget::getAction(WindowAction action)
{
	Q_UNUSED(action)

	return NULL;
}

QUndoStack* TransfersContentsWidget::getUndoStack()
{
	return NULL;
}

QString TransfersContentsWidget::getTitle() const
{
	return tr("Transfers Manager");
}

QString TransfersContentsWidget::getType() const
{
	return "transfers";
}

QUrl TransfersContentsWidget::getUrl() const
{
	return QUrl("about:transfers");
}

QIcon TransfersContentsWidget::getIcon() const
{
	return QIcon(":/icons/transfers.png");
}

QPixmap TransfersContentsWidget::getThumbnail() const
{
	return QPixmap();
}

HistoryInformation TransfersContentsWidget::getHistory() const
{
	HistoryEntry entry;
	entry.url = getUrl().toString();
	entry.title = getTitle();
	entry.position = QPoint(0, 0);
	entry.zoom = 100;

	HistoryInformation information;
	information.index = 0;
	information.entries.append(entry);

	return information;
}

int TransfersContentsWidget::getZoom() const
{
	return 100;
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

bool TransfersContentsWidget::canZoom() const
{
	return false;
}

bool TransfersContentsWidget::isClonable() const
{
	return false;
}

bool TransfersContentsWidget::isLoading() const
{
	return false;
}

bool TransfersContentsWidget::isPrivate() const
{
	return false;
}

}
