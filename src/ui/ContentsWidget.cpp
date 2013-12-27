#include "ContentsWidget.h"
#include "ContentsDialog.h"
#include "../core/HistoryManager.h"

namespace Otter
{

ContentsWidget::ContentsWidget(Window *window) : QWidget(window),
	m_layer(NULL)
{
	if (window)
	{
		connect(window, SIGNAL(aboutToClose()), this, SLOT(close()));
	}
}

void ContentsWidget::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);

	if (m_layer)
	{
		m_layer->resize(size());

		for (int i = 0; i < m_dialogs.count(); ++i)
		{
			if (m_dialogs.at(i))
			{
				m_dialogs.at(i)->parentWidget()->move(geometry().center() - QRect(QPoint(0, 0), m_dialogs.at(i)->parentWidget()->size()).center());
			}
		}
	}
}

void ContentsWidget::close()
{
	for (int i = 0; i < m_dialogs.count(); ++i)
	{
		if (m_dialogs.at(i))
		{
			m_dialogs.at(i)->close();
		}
	}
}

void ContentsWidget::showDialog(QWidget *dialog)
{
	if (!dialog)
	{
		return;
	}

	if (!m_layer)
	{
		QPalette palette = this->palette();
		palette.setColor(QPalette::Window, QColor(0, 0, 0, 70));

		m_layer = new QWidget(this);
		m_layer->setAutoFillBackground(true);
		m_layer->setPalette(palette);
		m_layer->resize(size());
		m_layer->show();
		m_layer->raise();
	}

	ContentsDialog *contentsDialog = new ContentsDialog(dialog, this);
	contentsDialog->show();
	contentsDialog->adjustSize();
	contentsDialog->raise();
	contentsDialog->move(geometry().center() - QRect(QPoint(0, 0), contentsDialog->size()).center());

	dialog->setFocus();

	m_dialogs.append(dialog);
}

void ContentsWidget::hideDialog(QWidget *dialog)
{
	m_dialogs.removeAll(dialog);

	if (m_dialogs.isEmpty() && m_layer)
	{
		m_layer->hide();
		m_layer->deleteLater();
		m_layer = NULL;
	}
}

void ContentsWidget::triggerAction(WindowAction action, bool checked)
{
	Q_UNUSED(action)
	Q_UNUSED(checked)
}

void ContentsWidget::setHistory(const WindowHistoryInformation &history)
{
	Q_UNUSED(history)
}

void ContentsWidget::setZoom(int zoom)
{
	Q_UNUSED(zoom)
}

void ContentsWidget::setUrl(const QUrl &url, bool typed)
{
	HistoryManager::addEntry(url, getTitle(), getIcon(), typed);
}

void ContentsWidget::setParent(Window *window)
{
	if (window)
	{
		connect(window, SIGNAL(aboutToClose()), this, SLOT(close()));
	}

	QWidget::setParent(window);
}

ContentsWidget* ContentsWidget::clone(Window *window)
{
	Q_UNUSED(window)

	return NULL;
}

QAction* ContentsWidget::getAction(WindowAction action)
{
	Q_UNUSED(action)

	return NULL;
}

QUndoStack* ContentsWidget::getUndoStack()
{
	return NULL;
}

QPixmap ContentsWidget::getThumbnail() const
{
	return QPixmap();
}

WindowHistoryInformation ContentsWidget::getHistory() const
{
	WindowHistoryEntry entry;
	entry.url = getUrl().toString();
	entry.title = getTitle();
	entry.position = QPoint(0, 0);
	entry.zoom = 100;

	WindowHistoryInformation information;
	information.index = 0;
	information.entries.append(entry);

	return information;
}

int ContentsWidget::getZoom() const
{
	return 100;
}

bool ContentsWidget::canClone() const
{
	return false;
}

bool ContentsWidget::canZoom() const
{
	return false;
}

bool ContentsWidget::isLoading() const
{
	return false;
}

bool ContentsWidget::isPrivate() const
{
	return false;
}

}
