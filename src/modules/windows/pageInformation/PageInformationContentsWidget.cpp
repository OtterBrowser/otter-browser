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
#include "../../../ui/MainWindow.h"
#include "../../../ui/Window.h"

#include "ui_PageInformationContentsWidget.h"

namespace Otter
{

PageInformationContentsWidget::PageInformationContentsWidget(const QVariantMap &parameters, QWidget *parent) : ContentsWidget(parameters, nullptr, parent),
	m_window(nullptr),
	m_ui(new Ui::PageInformationContentsWidget)
{
	m_ui->setupUi(this);

	const QVector<SectionName> sections({GeneralSection, SecuritySection, PermissionsSection, MetaSection, HeadersSection});
	QStandardItemModel *model(new QStandardItemModel(this));

	for (int i = 0; i < sections.count(); ++i)
	{
		QStandardItem *item(new QStandardItem());
		item->setData(sections.at(i), Qt::UserRole);

		model->appendRow({item, new QStandardItem()});
	}

	m_ui->informationViewWidget->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->informationViewWidget->setModel(model);
	m_ui->informationViewWidget->expandAll();

	const MainWindow *mainWindow(MainWindow::findMainWindow(parentWidget()));

	if (mainWindow)
	{
		m_window = mainWindow->getActiveWindow();

		connect(mainWindow, &MainWindow::currentWindowChanged, this, [=]()
		{
			m_window = mainWindow->getActiveWindow();

			updateSections();
		});
	}

	updateSections();
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

		updateSections();
	}
}

void PageInformationContentsWidget::updateSections()
{
	for (int i = 0; i < m_ui->informationViewWidget->getRowCount(); ++i)
	{
		const QModelIndex index(m_ui->informationViewWidget->getIndex(i));
		QStandardItem *sectionItem(m_ui->informationViewWidget->getItem(index));
		QStandardItemModel *model(m_ui->informationViewWidget->getSourceModel());
		model->removeRows(0, model->rowCount(index), index);

		switch (static_cast<SectionName>(index.data(Qt::UserRole).toInt()))
		{
			case GeneralSection:
				m_ui->informationViewWidget->setData(index, tr("General"), Qt::DisplayRole);

				if (sectionItem)
				{
					QList<QStandardItem*> items({new QStandardItem(tr("Title:")), new QStandardItem(m_window ? m_window->getTitle() : tr("<empty>"))});
					items[0]->setFlags(items[0]->flags() | Qt::ItemNeverHasChildren);
					items[1]->setFlags(items[1]->flags() | Qt::ItemNeverHasChildren);

					sectionItem->appendRow(items);
				}

				break;
			case HeadersSection:
				m_ui->informationViewWidget->setData(index, tr("Headers"), Qt::DisplayRole);

				if (sectionItem && m_window && m_window->getWebWidget())
				{
					const QMap<QByteArray, QByteArray> headers(m_window->getWebWidget()->getHeaders());
					QMap<QByteArray, QByteArray>::const_iterator iterator;

					for (iterator = headers.constBegin(); iterator != headers.constEnd(); ++iterator)
					{
						QList<QStandardItem*> items({new QStandardItem(QString(iterator.key())), new QStandardItem(QString(iterator.value()))});
						items[0]->setFlags(items[0]->flags() | Qt::ItemNeverHasChildren);
						items[1]->setFlags(items[1]->flags() | Qt::ItemNeverHasChildren);

						sectionItem->appendRow(items);
					}
				}

				break;
			case MetaSection:
				m_ui->informationViewWidget->setData(index, tr("Meta"), Qt::DisplayRole);

				if (sectionItem && m_window && m_window->getWebWidget())
				{
					const QMultiMap<QString, QString> metaData(m_window->getWebWidget()->getMetaData());
					QMultiMap<QString, QString>::const_iterator iterator;

					for (iterator = metaData.constBegin(); iterator != metaData.constEnd(); ++iterator)
					{
						if (!iterator.key().isEmpty())
						{
							QList<QStandardItem*> items({new QStandardItem(iterator.key()), new QStandardItem(iterator.value())});
							items[0]->setFlags(items[0]->flags() | Qt::ItemNeverHasChildren);
							items[1]->setFlags(items[1]->flags() | Qt::ItemNeverHasChildren);

							sectionItem->appendRow(items);
						}
					}
				}

				break;
			case PermissionsSection:
				m_ui->informationViewWidget->setData(index, tr("Permissions"), Qt::DisplayRole);

				break;
			case SecuritySection:
				m_ui->informationViewWidget->setData(index, tr("Security"), Qt::DisplayRole);

				break;
			default:
				break;
		}

		m_ui->informationViewWidget->setRowHidden(i, model->invisibleRootItem()->index(), (model->rowCount(index) == 0));
	}
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

}
