/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "PopupsBarWidget.h"
#include "../../../core/Utils.h"

#include "ui_PopupsBarWidget.h"

#include <QtWidgets/QMenu>

namespace Otter
{

PopupsBarWidget::PopupsBarWidget(const QUrl &parentUrl, QWidget *parent) : QWidget(parent),
	m_popupsMenu(NULL),
	m_popupsGroup(new QActionGroup(this)),
	m_parentUrl(parentUrl),
	m_ui(new Ui::PopupsBarWidget)
{
	m_ui->setupUi(this);

	QMenu *menu = new QMenu(this);

	m_ui->iconLabel->setPixmap(Utils::getIcon(QLatin1String("window-popup-block"), false).pixmap(m_ui->iconLabel->size()));
	m_ui->detailsButton->setMenu(menu);
	m_ui->detailsButton->setPopupMode(QToolButton::InstantPopup);

	QAction *openAllAction = menu->addAction(tr("Open All Pop-Ups from This Website"));
	openAllAction->setCheckable(true);
	openAllAction->setData(QLatin1String("openAll"));

	QAction *openAllInBackgroundAction = menu->addAction(tr("Open Pop-Ups from This Website in Background"));
	openAllInBackgroundAction->setCheckable(true);
	openAllInBackgroundAction->setData(QLatin1String("openAllInBackground"));

	QAction *blockAllAction = menu->addAction(tr("Block All Pop-Ups from This Website"));
	blockAllAction->setCheckable(true);
	blockAllAction->setData(QLatin1String("blockAll"));

	QAction *askAction = menu->addAction(tr("Always Ask What to Do for This Website"));
	askAction->setCheckable(true);
	askAction->setData(QLatin1String("ask"));

	m_popupsGroup->setExclusive(true);
	m_popupsGroup->addAction(openAllAction);
	m_popupsGroup->addAction(openAllInBackgroundAction);
	m_popupsGroup->addAction(blockAllAction);
	m_popupsGroup->addAction(askAction);

	menu->addSeparator();

	m_popupsMenu = menu->addMenu(tr("Blocked Pop-ups"));
	m_popupsMenu->addAction(tr("Open All"));
	m_popupsMenu->addSeparator();

	optionChanged(QLatin1String("Content/PopupsPolicy"));

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString, QVariant)), this, SLOT(optionChanged(QString)));
	connect(m_popupsGroup, SIGNAL(triggered(QAction*)), this, SLOT(setPolicy(QAction*)));
	connect(m_popupsMenu, SIGNAL(triggered(QAction*)), this, SLOT(openUrl(QAction*)));
	connect(m_ui->closeButton, SIGNAL(clicked()), this, SIGNAL(requestedClose()));
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
	}
}

void PopupsBarWidget::optionChanged(const QString &option)
{
	if (option == QLatin1String("Content/PopupsPolicy"))
	{
		const QString popupsPolicy = SettingsManager::getValue(QLatin1String("Content/PopupsPolicy"), m_parentUrl).toString();

		for (int i = 0; i < m_popupsGroup->actions().count(); ++i)
		{
			if (popupsPolicy == m_popupsGroup->actions().at(i)->data().toString())
			{
				m_popupsGroup->actions().at(i)->setChecked(true);

				break;
			}
		}
	}
}

void PopupsBarWidget::addPopup(const QUrl &url)
{
	QAction *action = m_popupsMenu->addAction(QString("%1").arg(fontMetrics().elidedText(url.url(), Qt::ElideMiddle, 256)));
	action->setData(url.url());

	m_ui->messageLabel->setText(tr("%1 wants to open %n pop-up window(s).", "", m_popupsMenu->actions().count() - 2).arg(m_parentUrl.host()));
}

void PopupsBarWidget::openUrl(QAction *action)
{
	if (!action)
	{
		return;
	}

	if (action->data().isNull())
	{
		for (int i = 0; i < m_popupsMenu->actions().count(); ++i)
		{
			const QUrl url = m_popupsMenu->actions().at(i)->data().toUrl();

			if (!url.isEmpty())
			{
				emit requestedNewWindow(url, WindowsManager::NewTabOpen);
			}
		}

		return;
	}

	emit requestedNewWindow(action->data().toUrl(), WindowsManager::NewTabOpen);
}

void PopupsBarWidget::setPolicy(QAction *action)
{
	if (action)
	{
		SettingsManager::setValue(QLatin1String("Content/PopupsPolicy"), action->data(), m_parentUrl);
	}
}

}
