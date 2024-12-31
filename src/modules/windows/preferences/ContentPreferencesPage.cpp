/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2024 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "ContentPreferencesPage.h"
#include "../../../core/Application.h"
#include "../../../core/SessionsManager.h"
#include "../../../core/SettingsManager.h"
#include "../../../ui/ColorWidget.h"
#include "../../../ui/OptionWidget.h"
#include "../../../../3rdparty/columnresizer/ColumnResizer.h"

#include "ui_ContentPreferencesPage.h"

#include <QtGui/QPainter>

namespace Otter
{

ColorItemDelegate::ColorItemDelegate(QObject *parent) : ItemDelegate(parent)
{
}

void ColorItemDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
	const QString color(index.data(Qt::EditRole).toString());
	QPixmap pixmap(option->fontMetrics.height(), option->fontMetrics.height());
	pixmap.fill(Qt::transparent);

	QPainter painter(&pixmap);
	painter.setRenderHints(QPainter::Antialiasing);

	ColorWidget::drawThumbnail(&painter, QColor(color), option->palette, pixmap.rect());

	painter.end();

	option->features |= QStyleOptionViewItem::HasDecoration;
	option->decorationSize = pixmap.size();
	option->icon = QIcon(pixmap);
	option->text = color.toUpper();
}

void ColorItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	OptionWidget *widget(qobject_cast<OptionWidget*>(editor));

	if (widget)
	{
		model->setData(index, widget->getValue(), Qt::EditRole);
	}
}

QWidget* ColorItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	OptionWidget *widget(new OptionWidget(index.data(Qt::EditRole), SettingsManager::ColorType, parent));
	widget->setFocus();

	connect(widget, &OptionWidget::commitData, this, &ColorItemDelegate::commitData);

	return widget;
}

FontItemDelegate::FontItemDelegate(QObject *parent) : ItemDelegate(parent)
{
}

void FontItemDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
	const QString fontFamily(index.data(Qt::EditRole).toString());

	option->font = QFont(fontFamily);
	option->text = fontFamily;
}

void FontItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	OptionWidget *widget(qobject_cast<OptionWidget*>(editor));

	if (widget)
	{
		model->setData(index, widget->getValue(), Qt::EditRole);
		model->setData(index.sibling(index.row(), 2), QFont(widget->getValue().toString()), Qt::FontRole);
	}
}

QWidget* FontItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	OptionWidget *widget(new OptionWidget(index.data(Qt::EditRole), SettingsManager::FontType, parent));
	widget->setFocus();

	connect(widget, &OptionWidget::commitData, this, &FontItemDelegate::commitData);

	return widget;
}

ContentPreferencesPage::ContentPreferencesPage(QWidget *parent) : CategoryPage(parent),
	m_ui(nullptr)
{
}

ContentPreferencesPage::~ContentPreferencesPage()
{
	if (wasLoaded())
	{
		delete m_ui;
	}
}

void ContentPreferencesPage::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (!wasLoaded())
	{
		return;
	}

	switch (event->type())
	{
		case QEvent::FontChange:
		case QEvent::StyleChange:
			updateStyle();

			break;
		case QEvent::LanguageChange:
			m_ui->retranslateUi(this);
			m_ui->enableImagesComboBox->setItemText(0, tr("All images"));
			m_ui->enableImagesComboBox->setItemText(1, tr("Cached images"));
			m_ui->enableImagesComboBox->setItemText(2, tr("No images"));
			m_ui->enablePluginsComboBox->setItemText(0, tr("Enabled"));
			m_ui->enablePluginsComboBox->setItemText(1, tr("On demand"));
			m_ui->enablePluginsComboBox->setItemText(2, tr("Disabled"));
			m_ui->userStyleSheetFilePathWidget->setFilters({tr("Style sheets (*.css)")});
			m_ui->popupsComboBox->setItemText(0, tr("Ask"));
			m_ui->popupsComboBox->setItemText(1, tr("Block all"));
			m_ui->popupsComboBox->setItemText(2, tr("Open all"));
			m_ui->popupsComboBox->setItemText(3, tr("Open all in background"));
			m_ui->fontsViewWidget->getSourceModel()->setHorizontalHeaderLabels({tr("Style"), tr("Font"), tr("Preview")});
			m_ui->colorsViewWidget->getSourceModel()->setHorizontalHeaderLabels({tr("Type"), tr("Preview")});

			for (int i = 0; i < m_ui->fontsViewWidget->getRowCount(); ++i)
			{
				m_ui->fontsViewWidget->setData(m_ui->fontsViewWidget->getIndex(i, 2), tr("The quick brown fox jumps over the lazy dog"), Qt::DisplayRole);
			}

			break;
		default:
			break;
	}
}

void ContentPreferencesPage::load()
{
	if (wasLoaded())
	{
		return;
	}

	m_ui = new Ui::ContentPreferencesPage();
	m_ui->setupUi(this);
	m_ui->enableImagesComboBox->addItem(tr("All images"), QLatin1String("enabled"));
	m_ui->enableImagesComboBox->addItem(tr("Cached images"), QLatin1String("onlyCached"));
	m_ui->enableImagesComboBox->addItem(tr("No images"), QLatin1String("disabled"));

	const int enableImagesIndex(m_ui->enableImagesComboBox->findData(SettingsManager::getOption(SettingsManager::Permissions_EnableImagesOption).toString()));

	m_ui->enableImagesComboBox->setCurrentIndex((enableImagesIndex < 0) ? 0 : enableImagesIndex);
	m_ui->enablePluginsComboBox->addItem(tr("Enabled"), QLatin1String("enabled"));
	m_ui->enablePluginsComboBox->addItem(tr("On demand"), QLatin1String("onDemand"));
	m_ui->enablePluginsComboBox->addItem(tr("Disabled"), QLatin1String("disabled"));

	const int enablePluginsIndex(m_ui->enablePluginsComboBox->findData(SettingsManager::getOption(SettingsManager::Permissions_EnablePluginsOption).toString()));

	m_ui->enablePluginsComboBox->setCurrentIndex((enablePluginsIndex < 0) ? 1 : enablePluginsIndex);
	m_ui->userStyleSheetFilePathWidget->setPath(SettingsManager::getOption(SettingsManager::Content_UserStyleSheetOption).toString());
	m_ui->userStyleSheetFilePathWidget->setFilters({tr("Style sheets (*.css)")});
	m_ui->popupsComboBox->addItem(tr("Ask"), QLatin1String("ask"));
	m_ui->popupsComboBox->addItem(tr("Block all"), QLatin1String("blockAll"));
	m_ui->popupsComboBox->addItem(tr("Open all"), QLatin1String("openAll"));
	m_ui->popupsComboBox->addItem(tr("Open all in background"), QLatin1String("openAllInBackground"));
	m_ui->enableContentBlockingCheckBox->setChecked(SettingsManager::getOption(SettingsManager::ContentBlocking_EnableContentBlockingOption).toBool());
	m_ui->defaultZoomSpinBox->setValue(SettingsManager::getOption(SettingsManager::Content_DefaultZoomOption).toInt());
	m_ui->zoomTextOnlyCheckBox->setChecked(SettingsManager::getOption(SettingsManager::Content_ZoomTextOnlyOption).toBool());
	m_ui->proportionalFontSizeSpinBox->setValue(SettingsManager::getOption(SettingsManager::Content_DefaultFontSizeOption).toInt());
	m_ui->fixedFontSizeSpinBox->setValue(SettingsManager::getOption(SettingsManager::Content_DefaultFixedFontSizeOption).toInt());
	m_ui->minimumFontSizeSpinBox->setValue(SettingsManager::getOption(SettingsManager::Content_MinimumFontSizeOption).toInt());

	const int popupsPolicyIndex(m_ui->popupsComboBox->findData(SettingsManager::getOption(SettingsManager::Permissions_ScriptsCanOpenWindowsOption).toString()));

	m_ui->popupsComboBox->setCurrentIndex((popupsPolicyIndex < 0) ? 0 : popupsPolicyIndex);

	QStandardItemModel *fontsModel(new QStandardItemModel(this));
	fontsModel->setHorizontalHeaderLabels({tr("Style"), tr("Font"), tr("Preview")});
	fontsModel->setHeaderData(2, Qt::Horizontal, 300, HeaderViewWidget::WidthRole);

	const QVector<QLatin1String> fonts({QLatin1String("StandardFont"), QLatin1String("FixedFont"), QLatin1String("SerifFont"), QLatin1String("SansSerifFont"), QLatin1String("CursiveFont"), QLatin1String("FantasyFont")});
	const QStringList fontCategories({tr("Standard font"), tr("Fixed-width font"), tr("Serif font"), tr("Sans-serif font"), tr("Cursive font"), tr("Fantasy font")});

	for (int i = 0; i < fonts.count(); ++i)
	{
		const QString optionName(QLatin1String("Content/") + fonts.at(i));
		const QString family(SettingsManager::getOption(SettingsManager::getOptionIdentifier(optionName)).toString());
		QList<QStandardItem*> items({new QStandardItem(fontCategories.at(i)), new QStandardItem(family), new QStandardItem(tr("The quick brown fox jumps over the lazy dog"))});
		items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
		items[1]->setData(optionName, Qt::UserRole);
		items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
		items[2]->setData(QFont(family), Qt::FontRole);
		items[2]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);

		fontsModel->appendRow(items);
	}

	m_ui->fontsViewWidget->setModel(fontsModel);
	m_ui->fontsViewWidget->setItemDelegateForColumn(1, new FontItemDelegate(this));

	QStandardItemModel *colorsModel(new QStandardItemModel(this));
	colorsModel->setHorizontalHeaderLabels({tr("Type"), tr("Preview")});
	colorsModel->setHeaderData(0, Qt::Horizontal, 300, HeaderViewWidget::WidthRole);

	const QVector<QLatin1String> colors({QLatin1String("BackgroundColor"), QLatin1String("TextColor"), QLatin1String("LinkColor"), QLatin1String("VisitedLinkColor")});
	const QStringList colorTypes({tr("Background Color"), tr("Text Color"), tr("Link Color"), tr("Visited Link Color")});

	for (int i = 0; i < colors.count(); ++i)
	{
		const QString optionName(QLatin1String("Content/") + colors.at(i));
		QList<QStandardItem*> items({new QStandardItem(colorTypes.at(i)), new QStandardItem(SettingsManager::getOption(SettingsManager::getOptionIdentifier(optionName)).toString())});
		items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
		items[1]->setData(optionName, Qt::UserRole);
		items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);

		colorsModel->appendRow(items);
	}

	m_ui->colorsViewWidget->setModel(colorsModel);
	m_ui->colorsViewWidget->setItemDelegateForColumn(1, new ColorItemDelegate(this));

	ColumnResizer *columnResizer(new ColumnResizer(this));
	columnResizer->addWidgetsFromFormLayout(m_ui->contentGeneralLayout, QFormLayout::LabelRole);
	columnResizer->addWidgetsFromFormLayout(m_ui->blockingLayout, QFormLayout::LabelRole);
	columnResizer->addWidgetsFromFormLayout(m_ui->zoomLayout, QFormLayout::LabelRole);
	columnResizer->addWidgetsFromFormLayout(m_ui->fontsLayout, QFormLayout::LabelRole);

	updateStyle();

	connect(m_ui->manageContentBlockingButton, &QPushButton::clicked, this,[&]()
	{
		const QUrl url(QLatin1String("about:content-filters"));

		if (!SessionsManager::hasUrl(url, true))
		{
			Application::triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), url}}, this);
		}
	});
	connect(m_ui->fontsViewWidget->selectionModel(), &QItemSelectionModel::currentChanged, this, [&](const QModelIndex &currentIndex, const QModelIndex &previousIndex)
	{
		m_ui->fontsViewWidget->closePersistentEditor(previousIndex.sibling(previousIndex.row(), 1));

		if (currentIndex.isValid())
		{
			m_ui->fontsViewWidget->openPersistentEditor(currentIndex.sibling(currentIndex.row(), 1));
		}
	});
	connect(m_ui->colorsViewWidget->selectionModel(), &QItemSelectionModel::currentChanged, this, [&](const QModelIndex &currentIndex, const QModelIndex &previousIndex)
	{
		m_ui->colorsViewWidget->closePersistentEditor(previousIndex.sibling(previousIndex.row(), 1));

		if (currentIndex.isValid())
		{
			m_ui->colorsViewWidget->openPersistentEditor(currentIndex.sibling(currentIndex.row(), 1));
		}
	});

	markAsLoaded();
}

void ContentPreferencesPage::save()
{
	SettingsManager::setOption(SettingsManager::Permissions_EnableImagesOption, m_ui->enableImagesComboBox->currentData(Qt::UserRole).toString());
	SettingsManager::setOption(SettingsManager::Permissions_EnablePluginsOption, m_ui->enablePluginsComboBox->currentData(Qt::UserRole).toString());
	SettingsManager::setOption(SettingsManager::Content_UserStyleSheetOption, m_ui->userStyleSheetFilePathWidget->getPath());
	SettingsManager::setOption(SettingsManager::Permissions_ScriptsCanOpenWindowsOption, m_ui->popupsComboBox->currentData().toString());
	SettingsManager::setOption(SettingsManager::ContentBlocking_EnableContentBlockingOption, m_ui->enableContentBlockingCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::Content_DefaultZoomOption, m_ui->defaultZoomSpinBox->value());
	SettingsManager::setOption(SettingsManager::Content_ZoomTextOnlyOption, m_ui->zoomTextOnlyCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::Content_DefaultFontSizeOption, m_ui->proportionalFontSizeSpinBox->value());
	SettingsManager::setOption(SettingsManager::Content_DefaultFixedFontSizeOption, m_ui->fixedFontSizeSpinBox->value());
	SettingsManager::setOption(SettingsManager::Content_MinimumFontSizeOption, m_ui->minimumFontSizeSpinBox->value());

	for (int i = 0; i < m_ui->fontsViewWidget->getRowCount(); ++i)
	{
		const QModelIndex index(m_ui->fontsViewWidget->getIndex(i, 1));

		SettingsManager::setOption(SettingsManager::getOptionIdentifier(index.data(Qt::UserRole).toString()), index.data(Qt::DisplayRole));
	}

	for (int i = 0; i < m_ui->colorsViewWidget->getRowCount(); ++i)
	{
		const QModelIndex index(m_ui->fontsViewWidget->getIndex(i, 1));

		SettingsManager::setOption(SettingsManager::getOptionIdentifier(index.data(Qt::UserRole).toString()), index.data(Qt::EditRole));
	}
}

void ContentPreferencesPage::updateStyle()
{
	m_ui->colorsViewWidget->setMaximumHeight(m_ui->colorsViewWidget->getContentsHeight());
	m_ui->fontsViewWidget->setMaximumHeight(m_ui->fontsViewWidget->getContentsHeight());
}

QString ContentPreferencesPage::getTitle() const
{
	return tr("Content");
}

}
