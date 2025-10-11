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

#include "ContentsWidget.h"
#include "BookmarkPropertiesDialog.h"
#include "ContentsDialog.h"
#include "MainWindow.h"
#include "Window.h"
#include "../core/Application.h"

#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrintPreviewDialog>
#include <QtWidgets/QMessageBox>

namespace Otter
{

ContentsWidget::ContentsWidget(const QVariantMap &parameters, Window *window, QWidget *parent) : QWidget(parent),
	m_window(window),
	m_layer(nullptr),
	m_layerTimer(0),
	m_sidebar(parameters.value(QLatin1String("sidebar"), -1).toInt()),
	m_isModified(false)
{
	if (window)
	{
		connect(window, &Window::aboutToClose, this, &ContentsWidget::handleAboutToClose);
	}
}

void ContentsWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_layerTimer)
	{
		killTimer(m_layerTimer);

		m_layerTimer = 0;

		if (m_layer)
		{
			m_layer->hide();
			m_layer->deleteLater();
			m_layer = nullptr;
		}
	}
}

void ContentsWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		emit titleChanged(getTitle());
	}
}

void ContentsWidget::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);

	if (m_layer)
	{
		m_layer->resize(size());

		for (int i = 0; i < m_dialogs.count(); ++i)
		{
			ContentsDialog *dialog(m_dialogs.at(i));

			if (dialog)
			{
				dialog->raise();
				dialog->adjustSize();
				dialog->move(geometry().center() - QRect({0, 0}, dialog->size()).center());
			}
		}
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
			ContentsDialog *dialog(m_dialogs.at(i));

			if (dialog)
			{
				dialog->updateSize();
				dialog->raise();
				dialog->move(geometry().center() - QRect({0, 0}, dialog->size()).center());
			}
		}
	}
}

void ContentsWidget::closeEvent(QCloseEvent *event)
{
	if (!m_isModified || canClose())
	{
		event->accept();
	}
	else
	{
		event->ignore();
	}
}

void ContentsWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::MiddleButton)
	{
		event->accept();
	}
	else
	{
		QWidget::mousePressEvent(event);
	}
}

void ContentsWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::MiddleButton)
	{
		event->accept();
	}
	else
	{
		QWidget::mouseReleaseEvent(event);
	}
}

void ContentsWidget::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
{
	Q_UNUSED(trigger)

	switch (identifier)
	{
		case ActionsManager::PrintAction:
			{
				QPrinter printer;

				setupPrinter(&printer);

				QPrintDialog printDialog(&printer, this);
				printDialog.setWindowTitle(tr("Print Page"));

				if (printDialog.exec() == QDialog::Accepted)
				{
					print(&printer);
				}
			}

			break;
		case ActionsManager::PrintPreviewAction:
			{
				QPrintPreviewDialog printPreviewDialog(this);

				setupPrinter(printPreviewDialog.printer());

				printPreviewDialog.setWindowFlags(printPreviewDialog.windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);
				printPreviewDialog.setWindowTitle(tr("Print Preview"));

				if (Application::getActiveWindow())
				{
					printPreviewDialog.resize(printPreviewDialog.size().expandedTo(Application::getActiveWindow()->size()));
				}

				connect(&printPreviewDialog, &QPrintPreviewDialog::paintRequested, this, &ContentsWidget::print);

				printPreviewDialog.exec();
			}

			break;
		case ActionsManager::BookmarkPageAction:
			{
				const QUrl url(parameters.value(QLatin1String("url"), getUrl()).toUrl().adjusted(QUrl::RemovePassword));

				if (Utils::isUrlEmpty(url))
				{
					break;
				}

				if (BookmarksManager::hasBookmark(url))
				{
					BookmarkPropertiesDialog dialog(BookmarksManager::getBookmark(url), this);
					dialog.exec();
				}
				else if (parameters.value(QLatin1String("showDialog"), true).toBool())
				{
					BookmarkPropertiesDialog dialog(url, parameters.value(QLatin1String("title"), getTitle()).toString(), parameters.value(QLatin1String("description"), getDescription()).toString(), nullptr, -1, true, this);
					dialog.exec();
				}
				else
				{
					BookmarksManager::addBookmark(BookmarksModel::UrlBookmark, {{BookmarksModel::UrlRole, url}, {BookmarksModel::TitleRole, parameters.value(QLatin1String("title"), getTitle()).toString()}, {BookmarksModel::DescriptionRole, parameters.value(QLatin1String("description"), getDescription()).toString()}}, BookmarksManager::getLastUsedFolder());
				}
			}

			break;
		default:
			break;
	}
}

void ContentsWidget::print(QPrinter *printer)
{
	Q_UNUSED(printer)
}

void ContentsWidget::setupPrinter(QPrinter *printer)
{
	printer->setCreator(QStringLiteral("Otter Browser %1").arg(Application::getFullVersion()));
	printer->setDocName(getTitle());
}

void ContentsWidget::handleAboutToClose()
{
	for (int i = 0; i < m_dialogs.count(); ++i)
	{
		ContentsDialog *dialog(m_dialogs.at(i));

		if (dialog)
		{
			dialog->close();
		}
	}
}

void ContentsWidget::showDialog(ContentsDialog *dialog, bool lockEventLoop)
{
	if (!dialog)
	{
		return;
	}

	const Window *window(qobject_cast<Window*>(parent()));

	if (window && window->isAboutToClose())
	{
		dialog->deleteLater();

		return;
	}

	if (m_layerTimer != 0)
	{
		killTimer(m_layerTimer);

		m_layerTimer = 0;
	}

	m_dialogs.append(dialog);

	if (!m_layer)
	{
		QPalette palette(this->palette());
		palette.setColor(QPalette::Window, QColor(0, 0, 0, 70));

		m_layer = new QWidget(this);
		m_layer->setAutoFillBackground(true);
		m_layer->setPalette(palette);
		m_layer->resize(size());
		m_layer->show();
		m_layer->raise();
	}

	connect(dialog, &ContentsDialog::finished, this, [=]()
	{
		m_dialogs.removeAll(dialog);

		dialog->hide();
		dialog->deleteLater();

		if (m_dialogs.isEmpty() && m_layer && m_layerTimer == 0)
		{
			m_layerTimer = startTimer(100);
		}
	});

	dialog->setParent(m_layer);
	dialog->show();

	if (m_dialogs.count() > 1)
	{
		dialog->stackUnder(m_dialogs.value(m_dialogs.count() - 2));
	}
	else
	{
		dialog->raise();
		dialog->setFocus();
	}

	dialog->move(geometry().center() - QRect({0, 0}, dialog->size()).center());

	emit needsAttention();

	if (lockEventLoop)
	{
		QEventLoop eventLoop;

		connect(dialog, &ContentsDialog::finished, &eventLoop, &QEventLoop::quit);
		connect(this, &ContentsWidget::destroyed, &eventLoop, &QEventLoop::quit);

		eventLoop.exec();
	}
}

void ContentsWidget::markAsModified()
{
	setModified(true);
}

void ContentsWidget::setModified(bool isModified)
{
	if (m_isModified != isModified)
	{
		m_isModified = isModified;

		emit isModifiedChanged(isModified);
	}
}

void ContentsWidget::setOption(int identifier, const QVariant &value)
{
	Q_UNUSED(identifier)
	Q_UNUSED(value)
}

void ContentsWidget::setHistory(const Session::Window::History &history)
{
	Q_UNUSED(history)
}

void ContentsWidget::setZoom(int zoom)
{
	Q_UNUSED(zoom)
}

void ContentsWidget::setUrl(const QUrl &url, bool isTypedIn)
{
	Q_UNUSED(url)
	Q_UNUSED(isTypedIn)
}

void ContentsWidget::setParent(Window *window)
{
	if (m_window)
	{
		disconnect(m_window, &Window::aboutToClose, this, &ContentsWidget::handleAboutToClose);
	}

	m_window = window;

	if (window)
	{
		connect(window, &Window::aboutToClose, this, &ContentsWidget::handleAboutToClose);
	}

	QWidget::setParent(window);

	emit windowChanged();
}

ContentsWidget* ContentsWidget::clone(bool cloneHistory) const
{
	Q_UNUSED(cloneHistory)

	return nullptr;
}

WebWidget* ContentsWidget::getWebWidget() const
{
	return nullptr;
}

Window* ContentsWidget::getWindow() const
{
	return m_window;
}

QString ContentsWidget::parseQuery(const QString &query) const
{
	return query;
}

QString ContentsWidget::getDescription() const
{
	return {};
}

QString ContentsWidget::getVersion() const
{
	return QCoreApplication::applicationVersion();
}

QString ContentsWidget::getStatusMessage() const
{
	return {};
}

QVariant ContentsWidget::getOption(int identifier) const
{
	Q_UNUSED(identifier)

	return {};
}

QPixmap ContentsWidget::createThumbnail()
{
	return {};
}

ActionsManager::ActionDefinition::State ContentsWidget::getActionState(int identifier, const QVariantMap &parameters) const
{
	Q_UNUSED(parameters)

	ActionsManager::ActionDefinition::State state(ActionsManager::getActionDefinition(identifier).getDefaultState());
	state.isEnabled = false;

	return state;
}

Session::Window::History ContentsWidget::getHistory() const
{
	Session::Window::History::Entry entry;
	entry.url = getUrl().toString(QUrl::RemovePassword);
	entry.title = getTitle();
	entry.position = {0, 0};
	entry.zoom = 100;

	Session::Window::History history;
	history.entries = {entry};
	history.index = 0;

	return history;
}

WebWidget::ContentStates ContentsWidget::getContentState() const
{
	return WebWidget::ApplicationContentState;
}

WebWidget::LoadingState ContentsWidget::getLoadingState() const
{
	return WebWidget::FinishedLoadingState;
}

int ContentsWidget::getSidebar() const
{
	return m_sidebar;
}

int ContentsWidget::getZoom() const
{
	return 100;
}

bool ContentsWidget::canClose()
{
	QMessageBox::warning(this, tr("Warning"), tr("Page contents have been changed."), QMessageBox::Close);

	return false;
}

bool ContentsWidget::canClone() const
{
	return false;
}

bool ContentsWidget::canZoom() const
{
	return false;
}

bool ContentsWidget::isModified() const
{
	return m_isModified;
}

bool ContentsWidget::isPrivate() const
{
	return false;
}

bool ContentsWidget::isSidebarPanel() const
{
	return (m_sidebar >= 0);
}

ActiveWindowObserverContentsWidget::ActiveWindowObserverContentsWidget(const QVariantMap &parameters, Window *window, QWidget *parent) : ContentsWidget(parameters, window, parent),
	m_mainWindow(nullptr),
	m_activeWindow(nullptr)
{
	if (isSidebarPanel())
	{
		m_mainWindow = ((window && window->getMainWindow()) ? window->getMainWindow() : MainWindow::findMainWindow(parent));

		if (m_mainWindow)
		{
			setActiveWindow(m_mainWindow ? m_mainWindow->getActiveWindow() : nullptr);

			connect(m_mainWindow, &MainWindow::activeWindowChanged, this, &ActiveWindowObserverContentsWidget::handleCurrentWindowChanged);
		}

		connect(this, &ActiveWindowObserverContentsWidget::windowChanged, this, &ActiveWindowObserverContentsWidget::handleParentWindowChanged);
	}
}

void ActiveWindowObserverContentsWidget::handleCurrentWindowChanged()
{
	if (m_mainWindow)
	{
		setActiveWindow(m_mainWindow->getActiveWindow());
	}
}

void ActiveWindowObserverContentsWidget::handleParentWindowChanged()
{
	Window *window(getWindow());
	MainWindow *mainWindow((window && window->getMainWindow()) ? window->getMainWindow() : MainWindow::findMainWindow(parentWidget()));

	if (mainWindow == m_mainWindow)
	{
		return;
	}

	if (m_mainWindow)
	{
		disconnect(m_mainWindow, &MainWindow::activeWindowChanged, this, &ActiveWindowObserverContentsWidget::handleCurrentWindowChanged);
	}

	m_mainWindow = mainWindow;

	if (m_mainWindow)
	{
		connect(m_mainWindow, &MainWindow::activeWindowChanged, this, &ActiveWindowObserverContentsWidget::handleCurrentWindowChanged);
	}

	setActiveWindow(m_mainWindow ? m_mainWindow->getActiveWindow() : nullptr);
}

void ActiveWindowObserverContentsWidget::setActiveWindow(Window *window)
{
	if (window != m_activeWindow)
	{
		Window *previousWindow = m_activeWindow;

		m_activeWindow = window;

		emit activeWindowChanged(m_activeWindow, previousWindow);
	}
}

Window* ActiveWindowObserverContentsWidget::getActiveWindow() const
{
	return m_activeWindow;
}

QUrl ActiveWindowObserverContentsWidget::getUrl() const
{
	return {};
}

}
