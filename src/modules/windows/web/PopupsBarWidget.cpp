/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "PopupsBarWidget.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/MainWindow.h"

#include "ui_PopupsBarWidget.h"

#include <QtWidgets/QMenu>

namespace Otter
{

PopupsBarWidget::PopupsBarWidget(const QUrl &parentUrl, bool isPrivate, QWidget *parent) : QWidget(parent),
	m_actionGroup(nullptr),
	m_parentUrl(parentUrl),
	m_isPrivate(isPrivate),
	m_ui(new Ui::PopupsBarWidget)
{
	m_ui->setupUi(this);

	QMenu *menu(new QMenu(this));

	m_ui->iconLabel->setPixmap(ThemesManager::createIcon(QLatin1String("window-popup-block"), false).pixmap(m_ui->iconLabel->size()));
	m_ui->detailsButton->setMenu(menu);
	m_ui->detailsButton->setPopupMode(QToolButton::InstantPopup);

	connect(m_ui->closeButton, &QToolButton::clicked, this, &PopupsBarWidget::requestedClose);
	connect(menu, &QMenu::aboutToShow, this, &PopupsBarWidget::populateMenu);
}

PopupsBarWidget::~PopupsBarWidget()
{
	delete m_ui;
}

void PopupsBarWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
		m_ui->messageLabel->setText(tr("%1 wants to open %n pop-up window(s).", "", m_popupUrls.count()).arg(Utils::extractHost(m_parentUrl)));
	}
}

void PopupsBarWidget::addPopup(const QUrl &url)
{
	m_popupUrls.append(url);

	m_ui->messageLabel->setText(tr("%1 wants to open %n pop-up window(s).", "", m_popupUrls.count()).arg(Utils::extractHost(m_parentUrl)));
}

void PopupsBarWidget::openUrl(QAction *action)
{
	MainWindow *mainWindow(MainWindow::findMainWindow(this));

	if (!action || !mainWindow)
	{
		return;
	}

	const SessionsManager::OpenHints hints(m_isPrivate ? (SessionsManager::NewTabOpen | SessionsManager::PrivateOpen) : SessionsManager::NewTabOpen);

	if (action->data().isNull())
	{
		for (int i = 0; i < m_popupUrls.count(); ++i)
		{
			mainWindow->triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), m_popupUrls.at(i)}, {QLatin1String("hints"), QVariant(hints)}});
		}
	}
	else
	{
		mainWindow->triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), action->data().toUrl()}, {QLatin1String("hints"), QVariant(hints)}});
	}
}

void PopupsBarWidget::populateMenu()
{
	QMenu *menu(m_ui->detailsButton->menu());
	menu->clear();

	if (!m_actionGroup)
	{
		m_actionGroup = new QActionGroup(menu);
		m_actionGroup->setExclusive(true);

		connect(m_actionGroup, &QActionGroup::triggered, this, [&](QAction *action)
		{
			if (action)
			{
				SettingsManager::setOption(SettingsManager::Permissions_ScriptsCanOpenWindowsOption, action->data(), Utils::extractHost(m_parentUrl));
			}
		});
	}

	const QString popupsPolicy(SettingsManager::getOption(SettingsManager::Permissions_ScriptsCanOpenWindowsOption, Utils::extractHost(m_parentUrl)).toString());
	const QVector<QPair<QString, QString> > policies({{QLatin1String("openAll"), tr("Open All Pop-Ups from This Website")}, {QLatin1String("openAllInBackground"), tr("Open Pop-Ups from This Website in Background")}, {QLatin1String("blockAll"), tr("Block All Pop-Ups from This Website")}, {QLatin1String("ask"), tr("Always Ask What to Do for This Website")}});

	for (int i = 0; i < policies.count(); ++i)
	{
		const QPair<QString, QString> policy(policies.at(i));
		QAction *action(menu->addAction(policy.second));
		action->setCheckable(true);
		action->setChecked(popupsPolicy == policy.first);
		action->setData(policy.first);

		m_actionGroup->addAction(action);
	}

	menu->addSeparator();

	QMenu *popupsMenu(menu->addMenu(tr("Blocked Pop-ups")));
	popupsMenu->addAction(tr("Open All"));
	popupsMenu->addSeparator();

	for (int i = 0; i < m_popupUrls.count(); ++i)
	{
		const QString url(m_popupUrls.at(i).url());
		QAction *action(popupsMenu->addAction(Utils::elideText(url, popupsMenu->fontMetrics(), nullptr, 300)));
		action->setData(url);
	}

	connect(popupsMenu, &QMenu::triggered, this, &PopupsBarWidget::openUrl);
}

}
