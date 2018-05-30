/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "PreferencesContentPageWidget.h"
#include "../../../3rdparty/columnresizer/ColumnResizer.h"
#include "../../core/SettingsManager.h"
#include "../../ui/ColorWidget.h"
#include "../../ui/OptionWidget.h"

#include "ui_PreferencesContentPageWidget.h"

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

PreferencesContentPageWidget::PreferencesContentPageWidget(QWidget *parent) :
	QWidget(parent),
	m_ui(new Ui::PreferencesContentPageWidget)
{
	m_ui->setupUi(this);
	m_ui->popupsComboBox->addItem(tr("Ask"), QLatin1String("ask"));
	m_ui->popupsComboBox->addItem(tr("Block all"), QLatin1String("blockAll"));
	m_ui->popupsComboBox->addItem(tr("Open all"), QLatin1String("openAll"));
	m_ui->popupsComboBox->addItem(tr("Open all in background"), QLatin1String("openAllInBackground"));
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
		const QString family(SettingsManager::getOption(SettingsManager::getOptionIdentifier((QLatin1String("Content/") + fonts.at(i)))).toString());
		QList<QStandardItem*> items({new QStandardItem(fontCategories.at(i)), new QStandardItem(family), new QStandardItem(tr("The quick brown fox jumps over the lazy dog"))});
		items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
		items[1]->setData(QLatin1String("Content/") + fonts.at(i), Qt::UserRole);
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
	columnResizer->addWidgetsFromFormLayout(m_ui->blockingLayout, QFormLayout::LabelRole);
	columnResizer->addWidgetsFromFormLayout(m_ui->zoomLayout, QFormLayout::LabelRole);
	columnResizer->addWidgetsFromFormLayout(m_ui->fontsLayout, QFormLayout::LabelRole);

	connect(m_ui->fontsViewWidget->selectionModel(), &QItemSelectionModel::currentChanged, this, &PreferencesContentPageWidget::handleCurrentFontChanged);
	connect(m_ui->colorsViewWidget->selectionModel(), &QItemSelectionModel::currentChanged, this, &PreferencesContentPageWidget::handleCurrentColorChanged);
}

PreferencesContentPageWidget::~PreferencesContentPageWidget()
{
	delete m_ui;
}

void PreferencesContentPageWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void PreferencesContentPageWidget::handleCurrentFontChanged(const QModelIndex &currentIndex, const QModelIndex &previousIndex)
{
	m_ui->fontsViewWidget->closePersistentEditor(previousIndex.sibling(previousIndex.row(), 1));

	if (currentIndex.isValid())
	{
		m_ui->fontsViewWidget->openPersistentEditor(currentIndex.sibling(currentIndex.row(), 1));
	}
}

void PreferencesContentPageWidget::handleCurrentColorChanged(const QModelIndex &currentIndex, const QModelIndex &previousIndex)
{
	m_ui->colorsViewWidget->closePersistentEditor(previousIndex.sibling(previousIndex.row(), 1));

	if (currentIndex.isValid())
	{
		m_ui->colorsViewWidget->openPersistentEditor(currentIndex.sibling(currentIndex.row(), 1));
	}
}

void PreferencesContentPageWidget::save()
{
	SettingsManager::setOption(SettingsManager::Permissions_ScriptsCanOpenWindowsOption, m_ui->popupsComboBox->currentData().toString());
	SettingsManager::setOption(SettingsManager::Content_DefaultZoomOption, m_ui->defaultZoomSpinBox->value());
	SettingsManager::setOption(SettingsManager::Content_ZoomTextOnlyOption, m_ui->zoomTextOnlyCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::Content_DefaultFontSizeOption, m_ui->proportionalFontSizeSpinBox->value());
	SettingsManager::setOption(SettingsManager::Content_DefaultFixedFontSizeOption, m_ui->fixedFontSizeSpinBox->value());
	SettingsManager::setOption(SettingsManager::Content_MinimumFontSizeOption, m_ui->minimumFontSizeSpinBox->value());

	for (int i = 0; i < m_ui->fontsViewWidget->getRowCount(); ++i)
	{
		SettingsManager::setOption(SettingsManager::getOptionIdentifier(m_ui->fontsViewWidget->getIndex(i, 1).data(Qt::UserRole).toString()), m_ui->fontsViewWidget->getIndex(i, 1).data(Qt::DisplayRole));
	}

	for (int i = 0; i < m_ui->colorsViewWidget->getRowCount(); ++i)
	{
		SettingsManager::setOption(SettingsManager::getOptionIdentifier(m_ui->colorsViewWidget->getIndex(i, 1).data(Qt::UserRole).toString()), m_ui->colorsViewWidget->getIndex(i, 1).data(Qt::EditRole));
	}
}

}
