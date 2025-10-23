/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "AddonsContentsWidget.h"
#include "DictionariesPage.h"
#include "UserScriptsPage.h"
#include "../../../core/ThemesManager.h"

#include "ui_AddonsContentsWidget.h"

#include <QtWidgets/QTabBar>

namespace Otter
{

AddonsContentsWidget::AddonsContentsWidget(const QVariantMap &parameters, Window *window, QWidget *parent) : ContentsWidget(parameters, window, parent),
	m_currentPage(nullptr),
	m_tabIndexEnumerator(metaObject()->indexOfEnumerator(QLatin1String("TabIndex").data())),
	m_ui(new Ui::AddonsContentsWidget)
{
	m_ui->setupUi(this);

	const bool needsDetails(!isSidebarPanel());

	addPage(new UserScriptsPage(needsDetails, this));

	m_ui->categoriesTabWidget->addPage(tr("User Styles"));

	addPage(new DictionariesPage(needsDetails, this));

	m_ui->categoriesTabWidget->addPage(tr("Translations"));

	m_currentPage = qobject_cast<AddonsPage*>(m_ui->categoriesTabWidget->getPage(0));

	connect(m_ui->categoriesTabWidget, &CategoriesTabWidget::currentChanged, this, [&](int index)
	{
		m_currentPage = qobject_cast<AddonsPage*>(m_ui->categoriesTabWidget->getPage(index));

		emit titleChanged(getTitle());
		emit urlChanged(getUrl());
	});
}

AddonsContentsWidget::~AddonsContentsWidget()
{
	delete m_ui;
}

void AddonsContentsWidget::changeEvent(QEvent *event)
{
	ContentsWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);

		QTabBar *tabBar(m_ui->categoriesTabWidget->tabBar());
		tabBar->setTabText(1, tr("User Styles"));
		tabBar->setTabText(3, tr("Translations"));
	}
}

void AddonsContentsWidget::print(QPrinter *printer)
{
	m_currentPage->print(printer);
}

void AddonsContentsWidget::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
{
	switch (identifier)
	{
		case ActionsManager::SelectAllAction:
		case ActionsManager::DeleteAction:
		case ActionsManager::FindAction:
		case ActionsManager::QuickFindAction:
		case ActionsManager::ActivateContentAction:
			m_currentPage->triggerAction(identifier, parameters, trigger);

			break;
		default:
			ContentsWidget::triggerAction(identifier, parameters, trigger);

			break;
	}
}

void AddonsContentsWidget::addPage(AddonsPage *page)
{
	m_ui->categoriesTabWidget->addPage(page);

	connect(page, &AddonsPage::needsActionsUpdate, this, [&]()
	{
		emit arbitraryActionsStateChanged({ActionsManager::DeleteAction});
	});
}

void AddonsContentsWidget::setUrl(const QUrl &url, bool isTypedIn)
{
	Q_UNUSED(isTypedIn)

	const QString section(url.fragment());

	if (!section.isEmpty())
	{
		m_ui->categoriesTabWidget->setCurrentIndex(qMax(0, EnumeratorMapper(staticMetaObject.enumerator(m_tabIndexEnumerator), QLatin1String("Tab")).mapToValue(section)));
	}
}

QString AddonsContentsWidget::getTitle() const
{
	CategoryPage *page(m_ui->categoriesTabWidget->getPage(m_ui->categoriesTabWidget->currentIndex()));

	if (page)
	{
		return QStringLiteral("%1 / %2").arg(tr("Addons"), page->getTitle());
	}

	return tr("Addons");
}

QLatin1String AddonsContentsWidget::getType() const
{
	return QLatin1String("addons");
}

QUrl AddonsContentsWidget::getUrl() const
{
	QUrl url(QLatin1String("about:addons"));

	if (m_ui->categoriesTabWidget->currentIndex() != 0)
	{
		url.setFragment(EnumeratorMapper(staticMetaObject.enumerator(m_tabIndexEnumerator), QLatin1String("Tab")).mapToName(m_ui->categoriesTabWidget->currentIndex()));
	}

	return url;
}

QIcon AddonsContentsWidget::getIcon() const
{
	return ThemesManager::createIcon(QLatin1String("preferences-plugin"), false);
}

ActionsManager::ActionDefinition::State AddonsContentsWidget::getActionState(int identifier, const QVariantMap &parameters) const
{
	return m_currentPage->getActionState(identifier, parameters);
}

WebWidget::LoadingState AddonsContentsWidget::getLoadingState() const
{
	return m_currentPage->getLoadingState();
}

}
