/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "PreferencesContentPageWidget.h"
#include "../../core/SettingsManager.h"
#include "../../ui/OptionDelegate.h"
#include "../../ui/OptionWidget.h"

#include "ui_PreferencesContentPageWidget.h"

namespace Otter
{

PreferencesContentPageWidget::PreferencesContentPageWidget(QWidget *parent) :
	QWidget(parent),
	m_ui(new Ui::PreferencesContentPageWidget)
{
	m_ui->setupUi(this);
	m_ui->popupsComboBox->addItem(tr("Ask"), QLatin1String("ask"));
	m_ui->popupsComboBox->addItem(tr("Block all"), QLatin1String("blockAll"));
	m_ui->popupsComboBox->addItem(tr("Open all"), QLatin1String("openAll"));
	m_ui->popupsComboBox->addItem(tr("Open all in background"), QLatin1String("openAllInBackground"));
	m_ui->defaultZoomSpinBox->setValue(SettingsManager::getValue(QLatin1String("Content/DefaultZoom")).toInt());
	m_ui->zoomTextOnlyCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Content/ZoomTextOnly")).toBool());
	m_ui->proportionalFontSizeSpinBox->setValue(SettingsManager::getValue(QLatin1String("Content/DefaultFontSize")).toInt());
	m_ui->fixedFontSizeSpinBox->setValue(SettingsManager::getValue(QLatin1String("Content/DefaultFixedFontSize")).toInt());
	m_ui->minimumFontSizeSpinBox->setValue(SettingsManager::getValue(QLatin1String("Content/MinimumFontSize")).toInt());

	const int popupsPolicyIndex(m_ui->popupsComboBox->findData(SettingsManager::getValue(QLatin1String("Content/PopupsPolicy")).toString()));

	m_ui->popupsComboBox->setCurrentIndex((popupsPolicyIndex < 0) ? 0 : popupsPolicyIndex);

	QStandardItemModel *fontsModel(new QStandardItemModel(this));
	fontsModel->setHorizontalHeaderLabels(QStringList({tr("Style"), tr("Font"), tr("Preview")}));

	const QList<QLatin1String> fonts({QLatin1String("StandardFont"), QLatin1String("FixedFont"), QLatin1String("SerifFont"), QLatin1String("SansSerifFont"), QLatin1String("CursiveFont"), QLatin1String("FantasyFont")});
	const QStringList fontCategories({tr("Standard font"), tr("Fixed-width font"), tr("Serif font"), tr("Sans-serif font"), tr("Cursive font"), tr("Fantasy font")});
	OptionDelegate *fontsDelegate(new OptionDelegate(true, this));

	for (int i = 0; i < fonts.count(); ++i)
	{
		const QString family(SettingsManager::getValue(QLatin1String("Content/") + fonts.at(i)).toString());
		QList<QStandardItem*> items({new QStandardItem(fontCategories.at(i)), new QStandardItem(family), new QStandardItem(tr("The quick brown fox jumps over the lazy dog"))});
		items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		items[1]->setData(QLatin1String("Content/") + fonts.at(i), Qt::UserRole);
		items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		items[2]->setData(QFont(family), Qt::FontRole);
		items[2]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		fontsModel->appendRow(items);
	}

	m_ui->fontsViewWidget->setModel(fontsModel);
	m_ui->fontsViewWidget->setItemDelegateForColumn(1, fontsDelegate);

	QStandardItemModel *colorsModel(new QStandardItemModel(this));
	colorsModel->setHorizontalHeaderLabels(QStringList({tr("Type"), tr("Preview")}));

	const QList<QLatin1String> colors({QLatin1String("BackgroundColor"), QLatin1String("TextColor"), QLatin1String("LinkColor"), QLatin1String("VisitedLinkColor")});
	const QStringList colorTypes({tr("Background Color"), tr("Text Color"), tr("Link Color"), tr("Visited Link Color")});
	OptionDelegate *colorsDelegate(new OptionDelegate(true, this));

	for (int i = 0; i < colors.count(); ++i)
	{
		const QString color(SettingsManager::getValue(QLatin1String("Content/") + colors.at(i)).toString());
		QList<QStandardItem*> items({new QStandardItem(colorTypes.at(i)), new QStandardItem(color)});
		items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		items[1]->setData(QColor(color), Qt::BackgroundRole);
		items[1]->setData(QLatin1String("Content/") + colors.at(i), Qt::UserRole);
		items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		colorsModel->appendRow(items);
	}

	m_ui->colorsViewWidget->setModel(colorsModel);
	m_ui->colorsViewWidget->setItemDelegateForColumn(1, colorsDelegate);

	connect(m_ui->fontsViewWidget->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(currentFontChanged(QModelIndex,QModelIndex)));
	connect(fontsDelegate, SIGNAL(commitData(QWidget*)), this, SLOT(fontChanged(QWidget*)));
	connect(m_ui->colorsViewWidget->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(currentColorChanged(QModelIndex,QModelIndex)));
	connect(colorsDelegate, SIGNAL(commitData(QWidget*)), this, SLOT(colorChanged(QWidget*)));
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

void PreferencesContentPageWidget::currentFontChanged(const QModelIndex &currentIndex, const QModelIndex &previousIndex)
{
	m_ui->fontsViewWidget->closePersistentEditor(previousIndex.sibling(previousIndex.row(), 1));

	if (currentIndex.isValid())
	{
		m_ui->fontsViewWidget->openPersistentEditor(currentIndex.sibling(currentIndex.row(), 1));
	}
}

void PreferencesContentPageWidget::fontChanged(QWidget *editor)
{
	OptionWidget *widget(qobject_cast<OptionWidget*>(editor));
	const QModelIndex index(widget ? widget->getIndex() : QModelIndex());

	if (index.isValid())
	{
		m_ui->fontsViewWidget->model()->setData(index.sibling(index.row(), 2), QFont(index.data(Qt::EditRole).toString()), Qt::FontRole);
	}

	emit settingsModified();
}

void PreferencesContentPageWidget::currentColorChanged(const QModelIndex &currentIndex, const QModelIndex &previousIndex)
{
	m_ui->colorsViewWidget->closePersistentEditor(previousIndex.sibling(previousIndex.row(), 1));

	if (currentIndex.isValid())
	{
		m_ui->colorsViewWidget->openPersistentEditor(currentIndex.sibling(currentIndex.row(), 1));
	}
}

void PreferencesContentPageWidget::colorChanged(QWidget *editor)
{
	OptionWidget *widget(qobject_cast<OptionWidget*>(editor));
	const QModelIndex index(widget ? widget->getIndex() : QModelIndex());

	if (index.isValid())
	{
		m_ui->colorsViewWidget->model()->setData(index.sibling(index.row(), 1), QColor(index.data(Qt::EditRole).toString()), Qt::BackgroundRole);
	}

	emit settingsModified();
}

void PreferencesContentPageWidget::save()
{
	SettingsManager::setValue(QLatin1String("Content/PopupsPolicy"), m_ui->popupsComboBox->currentData().toString());
	SettingsManager::setValue(QLatin1String("Content/DefaultZoom"), m_ui->defaultZoomSpinBox->value());
	SettingsManager::setValue(QLatin1String("Content/ZoomTextOnly"), m_ui->zoomTextOnlyCheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("Content/DefaultFontSize"), m_ui->proportionalFontSizeSpinBox->value());
	SettingsManager::setValue(QLatin1String("Content/DefaultFixedFontSize"), m_ui->fixedFontSizeSpinBox->value());
	SettingsManager::setValue(QLatin1String("Content/MinimumFontSize"), m_ui->minimumFontSizeSpinBox->value());

	for (int i = 0; i < m_ui->fontsViewWidget->getRowCount(); ++i)
	{
		SettingsManager::setValue(m_ui->fontsViewWidget->getIndex(i, 1).data(Qt::UserRole).toString(), m_ui->fontsViewWidget->getIndex(i, 1).data(Qt::DisplayRole));
	}

	for (int i = 0; i < m_ui->colorsViewWidget->getRowCount(); ++i)
	{
		SettingsManager::setValue(m_ui->colorsViewWidget->getIndex(i, 1).data(Qt::UserRole).toString(), m_ui->colorsViewWidget->getIndex(i, 1).data(Qt::EditRole));
	}
}

}
