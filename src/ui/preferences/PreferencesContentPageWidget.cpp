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

	const QList<QLatin1String> fonts({QLatin1String("StandardFont"), QLatin1String("FixedFont"), QLatin1String("SerifFont"), QLatin1String("SansSerifFont"), QLatin1String("CursiveFont"), QLatin1String("FantasyFont")});
	const QStringList fontCategories({tr("Standard font"), tr("Fixed-width font"), tr("Serif font"), tr("Sans-serif font"), tr("Cursive font"), tr("Fantasy font")});
	OptionDelegate *fontsDelegate(new OptionDelegate(true, this));

	m_ui->fontsWidget->setRowCount(fonts.count());
	m_ui->fontsWidget->setItemDelegateForColumn(1, fontsDelegate);

	for (int i = 0; i < fonts.count(); ++i)
	{
		const QString family(SettingsManager::getValue(QLatin1String("Content/") + fonts.at(i)).toString());
		QTableWidgetItem *familyItem(new QTableWidgetItem(family));
		familyItem->setData(Qt::UserRole, QLatin1String("Content/") + fonts.at(i));
		familyItem->setData((Qt::UserRole + 1), QLatin1String("font"));

		QTableWidgetItem *previewItem(new QTableWidgetItem(tr("The quick brown fox jumps over the lazy dog")));
		previewItem->setFont(QFont(family));

		m_ui->fontsWidget->setItem(i, 0, new QTableWidgetItem(fontCategories.at(i)));
		m_ui->fontsWidget->setItem(i, 1, familyItem);
		m_ui->fontsWidget->setItem(i, 2, previewItem);
	}

	const QList<QLatin1String> colors({QLatin1String("BackgroundColor"), QLatin1String("TextColor"), QLatin1String("LinkColor"), QLatin1String("VisitedLinkColor")});
	const QStringList colorTypes({tr("Background Color"), tr("Text Color"), tr("Link Color"), tr("Visited Link Color")});
	OptionDelegate *colorsDelegate(new OptionDelegate(true, this));

	m_ui->colorsWidget->setRowCount(colors.count());
	m_ui->colorsWidget->setItemDelegateForColumn(1, colorsDelegate);

	for (int i = 0; i < colors.count(); ++i)
	{
		const QString color(SettingsManager::getValue(QLatin1String("Content/") + colors.at(i)).toString());
		QTableWidgetItem *previewItem(new QTableWidgetItem(color));
		previewItem->setBackgroundColor(QColor(color));
		previewItem->setTextColor(Qt::transparent);
		previewItem->setData(Qt::UserRole, QLatin1String("Content/") + colors.at(i));
		previewItem->setData((Qt::UserRole + 1), QLatin1String("color"));

		m_ui->colorsWidget->setItem(i, 0, new QTableWidgetItem(colorTypes.at(i)));
		m_ui->colorsWidget->setItem(i, 1, previewItem);
	}

	connect(m_ui->fontsWidget, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(currentFontChanged(int,int,int,int)));
	connect(fontsDelegate, SIGNAL(commitData(QWidget*)), this, SLOT(fontChanged(QWidget*)));
	connect(m_ui->colorsWidget, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(currentColorChanged(int,int,int,int)));
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

void PreferencesContentPageWidget::currentFontChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
	Q_UNUSED(currentColumn)
	Q_UNUSED(previousColumn)

	QTableWidgetItem *previousItem(m_ui->fontsWidget->item(previousRow, 1));

	m_ui->fontsWidget->closePersistentEditor(previousItem);

	if (currentRow >= 0 && currentRow < m_ui->fontsWidget->rowCount())
	{
		m_ui->fontsWidget->openPersistentEditor(m_ui->fontsWidget->item(currentRow, 1));
	}
}

void PreferencesContentPageWidget::fontChanged(QWidget *editor)
{
	OptionWidget *widget(qobject_cast<OptionWidget*>(editor));

	if (widget && widget->getIndex().row() >= 0 && widget->getIndex().row() < m_ui->fontsWidget->rowCount())
	{
		m_ui->fontsWidget->item(widget->getIndex().row(), 1)->setText(m_ui->fontsWidget->item(widget->getIndex().row(), 1)->data(Qt::EditRole).toString());
		m_ui->fontsWidget->item(widget->getIndex().row(), 2)->setFont(QFont(widget->getValue().toString()));
	}

	emit settingsModified();
}

void PreferencesContentPageWidget::currentColorChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
	Q_UNUSED(currentColumn)
	Q_UNUSED(previousColumn)

	QTableWidgetItem *previousItem(m_ui->colorsWidget->item(previousRow, 1));

	m_ui->colorsWidget->closePersistentEditor(previousItem);

	if (currentRow >= 0 && currentRow < m_ui->colorsWidget->rowCount())
	{
		m_ui->colorsWidget->openPersistentEditor(m_ui->colorsWidget->item(currentRow, 1));
	}
}

void PreferencesContentPageWidget::colorChanged(QWidget *editor)
{
	OptionWidget *widget(qobject_cast<OptionWidget*>(editor));

	if (widget && widget->getIndex().row() >= 0 && widget->getIndex().row() < m_ui->colorsWidget->rowCount())
	{
		m_ui->colorsWidget->item(widget->getIndex().row(), 1)->setBackgroundColor(QColor(widget->getValue().toString()));
		m_ui->colorsWidget->item(widget->getIndex().row(), 1)->setData(Qt::EditRole, widget->getValue());
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

	for (int i = 0; i < m_ui->fontsWidget->rowCount(); ++i)
	{
		SettingsManager::setValue(m_ui->fontsWidget->item(i, 1)->data(Qt::UserRole).toString() , m_ui->fontsWidget->item(i, 1)->data(Qt::DisplayRole));
	}

	for (int i = 0; i < m_ui->colorsWidget->rowCount(); ++i)
	{
		SettingsManager::setValue(m_ui->colorsWidget->item(i, 1)->data(Qt::UserRole).toString() , m_ui->colorsWidget->item(i, 1)->data(Qt::EditRole));
	}
}

}
