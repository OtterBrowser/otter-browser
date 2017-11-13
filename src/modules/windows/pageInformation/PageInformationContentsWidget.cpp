/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "PageInformationContentsWidget.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/Window.h"

#include "ui_PageInformationContentsWidget.h"

namespace Otter
{

PageInformationContentsWidget::PageInformationContentsWidget(const QVariantMap &parameters, Window *window) : ContentsWidget(parameters, window),
	m_ui(new Ui::PageInformationContentsWidget)
{
	m_ui->setupUi(this);
	m_ui->informationViewWidget->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->informationViewWidget->expandAll();
}

PageInformationContentsWidget::~PageInformationContentsWidget()
{
	delete m_ui;
}

void PageInformationContentsWidget::changeEvent(QEvent *event)
{
	ContentsWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void PageInformationContentsWidget::print(QPrinter *printer)
{
	Q_UNUSED(printer)
}

QString PageInformationContentsWidget::getTitle() const
{
	return tr("Page Information");
}

QLatin1String PageInformationContentsWidget::getType() const
{
	return QLatin1String("pageInformation");
}

QUrl PageInformationContentsWidget::getUrl() const
{
	return {};
}

QIcon PageInformationContentsWidget::getIcon() const
{
	return ThemesManager::createIcon(QLatin1String("dialog-information"), false);
}

WebWidget::LoadingState PageInformationContentsWidget::getLoadingState() const
{
	return WebWidget::FinishedLoadingState;
}

}
