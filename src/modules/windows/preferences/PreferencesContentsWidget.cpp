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

#include "PreferencesContentsWidget.h"
#include "AdvancedPreferencesPage.h"
#include "ContentPreferencesPage.h"
#include "GeneralPreferencesPage.h"
#include "InputPreferencesPage.h"
#include "PrivacyPreferencesPage.h"
#include "SearchPreferencesPage.h"
#include "WebsitesPreferencesPage.h"
#include "../../../core/Application.h"
#include "../../../core/ThemesManager.h"

#include "ui_PreferencesContentsWidget.h"

#include <QtWidgets/QMessageBox>

namespace Otter
{

PreferencesContentsWidget::PreferencesContentsWidget(const QVariantMap &parameters, Window *window, QWidget *parent) : ContentsWidget(parameters, window, parent),
	m_tabIndexEnumerator(metaObject()->indexOfEnumerator(QLatin1String("TabIndex").data())),
	m_ui(new Ui::PreferencesContentsWidget)
{
	m_ui->setupUi(this);

	addPage(new GeneralPreferencesPage(this));
	addPage(new ContentPreferencesPage(this));
	addPage(new PrivacyPreferencesPage(this));
	addPage(new SearchPreferencesPage(this));
	addPage(new InputPreferencesPage(this));
	addPage(new WebsitesPreferencesPage(this));
	addPage(new AdvancedPreferencesPage(this));

	connect(this, &PreferencesContentsWidget::isModifiedChanged, m_ui->saveButton, &QPushButton::setEnabled);
	connect(m_ui->categoriesTabWidget, &CategoriesTabWidget::currentChanged, this, [&]()
	{
		emit titleChanged(getTitle());
		emit urlChanged(getUrl());
	});
	connect(m_ui->saveButton, &QPushButton::clicked, this, [&]()
	{
		setModified(false);

		emit requestedSave();
	});
	connect(m_ui->allSettingsButton, &QPushButton::clicked, this,[&]()
	{
		const QUrl url(QLatin1String("about:config"));

		if (!SessionsManager::hasUrl(url, true))
		{
			Application::triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), url}}, this);
		}
	});
}

PreferencesContentsWidget::~PreferencesContentsWidget()
{
	delete m_ui;
}

void PreferencesContentsWidget::changeEvent(QEvent *event)
{
	ContentsWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void PreferencesContentsWidget::addPage(CategoryPage *page)
{
	m_ui->categoriesTabWidget->addPage(page);

	connect(page, &CategoryPage::settingsModified, this, &PreferencesContentsWidget::markAsModified);
}

void PreferencesContentsWidget::setUrl(const QUrl &url, bool isTypedIn)
{
	Q_UNUSED(isTypedIn)

	const QString section(url.fragment());

	if (!section.isEmpty())
	{
		m_ui->categoriesTabWidget->setCurrentIndex(qMax(0, EnumeratorMapper(staticMetaObject.enumerator(m_tabIndexEnumerator), QLatin1String("Tab")).mapToValue(section)));
	}
}

QString PreferencesContentsWidget::getTitle() const
{
	CategoryPage *page(m_ui->categoriesTabWidget->getPage(m_ui->categoriesTabWidget->currentIndex()));

	if (page)
	{
		return QStringLiteral("%1 / %2").arg(tr("Preferences"), page->getTitle());
	}

	return tr("Preferences");
}

QLatin1String PreferencesContentsWidget::getType() const
{
	return QLatin1String("preferences");
}

QUrl PreferencesContentsWidget::getUrl() const
{
	QUrl url(QLatin1String("about:preferences"));

	if (m_ui->categoriesTabWidget->currentIndex() != 0)
	{
		url.setFragment(EnumeratorMapper(staticMetaObject.enumerator(m_tabIndexEnumerator), QLatin1String("Tab")).mapToName(m_ui->categoriesTabWidget->currentIndex()));
	}

	return url;
}

QIcon PreferencesContentsWidget::getIcon() const
{
	return ThemesManager::createIcon(QLatin1String("configuration"), false);
}

bool PreferencesContentsWidget::canClose()
{
	const int result(QMessageBox::question(this, tr("Question"), tr("The settings have been changed.\nDo you want to save them?"), QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel));

	if (result == QMessageBox::Cancel)
	{
		return false;
	}

	if (result == QMessageBox::Yes)
	{
		emit requestedSave();
	}

	return true;
}

}
