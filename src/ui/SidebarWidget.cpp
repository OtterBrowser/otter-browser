/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2015 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "SidebarWidget.h"
#include "MainWindow.h"
#include "toolbars/PanelChooserWidget.h"
#include "../core/ActionsManager.h"
#include "../core/SettingsManager.h"
#include "../core/Utils.h"
#include "../core/WindowsManager.h"
#include "../modules/windows/bookmarks/BookmarksContentsWidget.h"
#include "../modules/windows/cache/CacheContentsWidget.h"
#include "../modules/windows/configuration/ConfigurationContentsWidget.h"
#include "../modules/windows/cookies/CookiesContentsWidget.h"
#include "../modules/windows/history/HistoryContentsWidget.h"
#include "../modules/windows/notes/NotesContentsWidget.h"
#include "../modules/windows/transfers/TransfersContentsWidget.h"
#include "../modules/windows/web/WebContentsWidget.h"

#include "ui_SidebarWidget.h"

#include <QtGui/QIcon>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QToolBar>

namespace Otter
{

SidebarWidget::SidebarWidget(QWidget *parent) : QWidget(parent),
	m_currentWidget(NULL),
	m_resizeTimer(0),
	m_ui(new Ui::SidebarWidget)
{
	m_ui->setupUi(this);

	QToolBar *toolbar = new QToolBar(this);
	toolbar->setIconSize(QSize(16, 16));
	toolbar->addWidget(new PanelChooserWidget(this));
	toolbar->addAction(ActionsManager::getAction(ActionsManager::OpenPanelAction, this));

	QWidget *spacer = new QWidget(toolbar);
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	toolbar->addWidget(spacer);
	toolbar->addAction(ActionsManager::getAction(ActionsManager::ClosePanelAction, this));

	m_ui->panelLayout->addWidget(toolbar);

	m_ui->panelsButton->setPopupMode(QToolButton::InstantPopup);
	m_ui->panelsButton->setIcon(Utils::getIcon(QLatin1String("list-add")));

	optionChanged(QLatin1String("Sidebar/CurrentPanel"), SettingsManager::getValue(QLatin1String("Sidebar/CurrentPanel")));
	optionChanged(QLatin1String("Sidebar/Panels"), SettingsManager::getValue(QLatin1String("Sidebar/Panels")));

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
}

SidebarWidget::~SidebarWidget()
{
	delete m_ui;
}

void SidebarWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_resizeTimer)
	{
		SettingsManager::setValue(QString("Sidebar/Width"), width());

		killTimer(m_resizeTimer);

		m_resizeTimer = 0;
	}
}

void SidebarWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);

		for (QHash<QString, QToolButton*>::iterator iterator = m_buttons.begin(); iterator != m_buttons.end(); ++iterator)
		{
			iterator.value()->setToolTip(getPanelTitle(iterator.key()));
		}

		if (m_ui->panelsButton->menu())
		{
			QList<QAction*> actions = m_ui->panelsButton->menu()->actions();

			for (int i = 0; i < actions.count(); ++i)
			{
				if (!actions[i]->data().toString().isEmpty())
				{
					actions[i]->setText(getPanelTitle(actions[i]->data().toString()));
				}
			}
		}
	}
}

void SidebarWidget::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("Sidebar/CurrentPanel"))
	{
		selectPanel(value.toString());
	}
	else if (option == QLatin1String("Sidebar/Panels"))
	{
		qDeleteAll(m_buttons.begin(), m_buttons.end());

		m_buttons.clear();

		QMenu *menu = new QMenu(m_ui->panelsButton);
		const QStringList chosenPanels = value.toStringList();
		QStringList allPanels;
		allPanels << QLatin1String("bookmarks") << QLatin1String("cache") << QLatin1String("cookies") << QLatin1String("config") << QLatin1String("history") << QLatin1String("notes") << QLatin1String("transfers");

		for (int i = 0; i < allPanels.count(); ++i)
		{
			QAction *action = new QAction(menu);
			action->setCheckable(true);
			action->setChecked(chosenPanels.contains(allPanels[i]));
			action->setData(allPanels[i]);
			action->setText(getPanelTitle(allPanels[i]));

			connect(action, SIGNAL(toggled(bool)), this, SLOT(choosePanel(bool)));

			menu->addAction(action);
		}

		menu->addSeparator();

		for (int i = 0; i < chosenPanels.count(); ++i)
		{
			QToolButton *button = new QToolButton(this);
			button->setDefaultAction(new QAction(button));
			button->setToolTip(getPanelTitle(chosenPanels.at(i)));
			button->setCheckable(true);
			button->setAutoRaise(true);
			button->defaultAction()->setData(chosenPanels.at(i));

			if (chosenPanels.at(i) == QLatin1String("bookmarks"))
			{
				button->setIcon(Utils::getIcon(QLatin1String("bookmarks")));
			}
			else if (chosenPanels.at(i) == QLatin1String("cache"))
			{
				button->setIcon(Utils::getIcon(QLatin1String("cache")));
			}
			else if (chosenPanels.at(i) == QLatin1String("config"))
			{
				button->setIcon(Utils::getIcon(QLatin1String("configuration")));
			}
			else if (chosenPanels.at(i) == QLatin1String("cookies"))
			{
				button->setIcon(Utils::getIcon(QLatin1String("cookies")));
			}
			else if (chosenPanels.at(i) == QLatin1String("history"))
			{
				button->setIcon(Utils::getIcon(QLatin1String("view-history")));
			}
			else if (chosenPanels.at(i) == QLatin1String("notes"))
			{
				button->setIcon(Utils::getIcon(QLatin1String("notes")));
			}
			else if (chosenPanels.at(i) == QLatin1String("transfers"))
			{
				button->setIcon(Utils::getIcon(QLatin1String("transfers")));
			}
			else if (chosenPanels.at(i).startsWith(QLatin1String("web:")))
			{
				button->setIcon(Utils::getIcon(QLatin1String("text-html")));

				QAction *action = new QAction(menu);
				action->setCheckable(true);
				action->setChecked(true);
				action->setData(chosenPanels.at(i));
				action->setText(getPanelTitle(chosenPanels.at(i)));

				connect(action, SIGNAL(toggled(bool)), this, SLOT(choosePanel(bool)));

				menu->addAction(action);
			}
			else
			{
				continue;
			}

			m_ui->buttonsLayout->insertWidget(qMax(0, (m_ui->buttonsLayout->count() - 2)), button);

			m_buttons.insert(chosenPanels.at(i), button);

			connect(button->defaultAction(), SIGNAL(triggered()), this, SLOT(selectPanel()));
		}

		QAction *addWebPanelAction = new QAction(menu);
		addWebPanelAction->setText(tr("Add Web Panel…"));

		connect(addWebPanelAction, SIGNAL(triggered()), this, SLOT(addWebPanel()));

		menu->addSeparator();
		menu->addAction(addWebPanelAction);

		if (m_ui->panelsButton->menu())
		{
			m_ui->panelsButton->menu()->deleteLater();
		}

		m_ui->panelsButton->setMenu(menu);
	}
	else if (option == QLatin1String("Sidebar/Reverse"))
	{
		const bool isReversed = value.toBool();

		qobject_cast<QBoxLayout*>(layout())->setDirection(isReversed ? QBoxLayout::RightToLeft : QBoxLayout::LeftToRight);

		QToolBar *toolbar = findChild<QToolBar*>();

		if (toolbar)
		{
			toolbar->setLayoutDirection(isReversed ? Qt::RightToLeft : Qt::LeftToRight);

			QList<QWidget*> widgets = toolbar->findChildren<QWidget*>();

			for (int i = 0; i < widgets.count(); ++i)
			{
				widgets[i]->setLayoutDirection(Qt::LeftToRight);
			}
		}

		ActionsManager::getAction(ActionsManager::OpenPanelAction, this)->setIcon(Utils::getIcon(isReversed ? QLatin1String("arrow-left") : QLatin1String("arrow-right")));
	}
}

void SidebarWidget::scheduleSizeSave()
{
	if (isVisible() && !SettingsManager::getValue(QLatin1String("Sidebar/CurrentPanel")).toString().isEmpty())
	{
		if (m_resizeTimer > 0)
		{
			killTimer(m_resizeTimer);
		}

		m_resizeTimer = startTimer(500);
	}
}

void SidebarWidget::addWebPanel()
{
	WindowsManager *manager = SessionsManager::getWindowsManager();
	QString url;

	if (manager)
	{
		url = manager->getUrl().toString(QUrl::RemovePassword);
	}

	url = QInputDialog::getText(this, tr("Add web panel"), tr("Input address of web page to be shown in panel:"), QLineEdit::Normal, url);

	if (!url.isEmpty())
	{
		url = QLatin1String("web:") + url;

		SettingsManager::setValue(QLatin1String("Sidebar/Panels"), SettingsManager::getValue(QLatin1String("Sidebar/Panels")).toStringList() << url);
	}
}

void SidebarWidget::choosePanel(bool checked)
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (!action)
	{
		return;
	}

	QStringList chosenPanels = SettingsManager::getValue(QLatin1String("Sidebar/Panels")).toStringList();

	if (checked)
	{
		chosenPanels.append(action->data().toString());
	}
	else
	{
		chosenPanels.removeAll(action->data().toString());
	}

	SettingsManager::setValue(QLatin1String("Sidebar/Panels"), chosenPanels);
}

void SidebarWidget::selectPanel()
{
	QAction *action = qobject_cast<QAction*>(sender());

	if (action)
	{
		selectPanel((action->data().toString() == m_currentPanel) ? QString() : action->data().toString());
	}
}

void SidebarWidget::selectPanel(const QString &identifier)
{
	if (!identifier.isEmpty() && identifier == m_currentPanel)
	{
		return;
	}

	ContentsWidget *widget = NULL;

	if (identifier == QLatin1String("bookmarks"))
	{
		widget = new BookmarksContentsWidget(NULL);
	}
	else if (identifier == QLatin1String("cache"))
	{
		widget = new CacheContentsWidget(NULL);
	}
	else if (identifier == QLatin1String("config"))
	{
		widget = new ConfigurationContentsWidget(NULL);
	}
	else if (identifier == QLatin1String("cookies"))
	{
		widget = new CookiesContentsWidget(NULL);
	}
	else if (identifier == QLatin1String("history"))
	{
		widget = new HistoryContentsWidget(NULL);
	}
	else if (identifier == QLatin1String("notes"))
	{
		widget = new NotesContentsWidget(NULL);
	}
	else if (identifier == QLatin1String("transfers"))
	{
		widget = new TransfersContentsWidget(NULL);
	}
	else if (identifier.startsWith(QLatin1String("web:")))
	{
		WebContentsWidget *webWidget = new WebContentsWidget(true, NULL, NULL);
		webWidget->setUrl(identifier.section(QLatin1Char(':'), 1, -1), false);

		widget = webWidget;
	}

	if (widget)
	{
		MainWindow *window = MainWindow::findMainWindow(parent());

		if (window)
		{
			connect(widget, SIGNAL(requestedOpenUrl(QUrl,OpenHints)), window->getWindowsManager(), SLOT(open(QUrl,OpenHints)));
			connect(widget, SIGNAL(requestedSearch(QString,QString,OpenHints)), window->getWindowsManager(), SLOT(search(QString,QString,OpenHints)));
			connect(widget, SIGNAL(requestedAddBookmark(QUrl,QString,QString)), window->getWindowsManager(), SIGNAL(requestedAddBookmark(QUrl,QString,QString)));
			connect(widget, SIGNAL(requestedNewWindow(ContentsWidget*,OpenHints)), window->getWindowsManager(), SLOT(openWindow(ContentsWidget*,OpenHints)));
		}
	}

	if (m_currentWidget)
	{
		m_ui->panelLayout->removeWidget(m_currentWidget);

		m_currentWidget->deleteLater();
		m_currentWidget = NULL;
	}

	if (m_buttons.contains(m_currentPanel) && m_buttons[m_currentPanel])
	{
		m_buttons[m_currentPanel]->setChecked(false);
	}

	if (widget)
	{
		m_ui->panelLayout->addWidget(widget);

		if (m_buttons.contains(identifier) && m_buttons[identifier])
		{
			m_buttons[identifier]->setChecked(true);
		}

		setMinimumWidth(0);
		setMaximumWidth(QWIDGETSIZE_MAX);
	}
	else
	{
		setFixedWidth(m_ui->buttonsLayout->sizeHint().width());
	}

	m_ui->containerWidget->setVisible(widget != NULL);

	m_currentPanel = identifier;
	m_currentWidget = widget;

	SettingsManager::setValue(QLatin1String("Sidebar/CurrentPanel"), identifier);
}

ContentsWidget* SidebarWidget::getCurrentPanel()
{
	return m_currentWidget;
}

QString SidebarWidget::getPanelTitle(const QString &identifier)
{
	if (identifier == QLatin1String("bookmarks"))
	{
		return tr("Bookmarks");
	}

	if (identifier == QLatin1String("cache"))
	{
		return tr("Cache");
	}

	if (identifier == QLatin1String("cookies"))
	{
		return tr("Cookies");
	}

	if (identifier == QLatin1String("config"))
	{
		return tr("Configuration");
	}

	if (identifier == QLatin1String("history"))
	{
		return tr("History");
	}

	if (identifier == QLatin1String("notes"))
	{
		return tr("Notes");
	}

	if (identifier == QLatin1String("transfers"))
	{
		return tr("Transfers");
	}

	if (identifier.startsWith(QLatin1String("web:")))
	{
		return identifier.mid(4);
	}

	return identifier;
}

QSize SidebarWidget::sizeHint() const
{
	if (SettingsManager::getValue(QLatin1String("Sidebar/CurrentPanel")).toString().isEmpty())
	{
		return m_ui->buttonsLayout->sizeHint();
	}

	return QSize(SettingsManager::getValue(QLatin1String("Sidebar/Width")).toInt(), m_ui->buttonsLayout->sizeHint().height());
}

}
