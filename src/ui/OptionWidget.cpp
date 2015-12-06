/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "OptionWidget.h"
#include "IconWidget.h"
#include "FilePathWidget.h"
#include "../core/SettingsManager.h"

#include <QtWidgets/QColorDialog>
#include <QtWidgets/QCompleter>
#include <QtWidgets/QHBoxLayout>

namespace Otter
{

OptionWidget::OptionWidget(bool isSimple, const QString &option, const QString &type, const QVariant &value, const QStringList &choices, const QModelIndex &index, QWidget *parent) : QWidget(parent),
	m_widget(NULL),
	m_iconWidget(NULL),
	m_colorButton(NULL),
	m_comboBox(NULL),
	m_fontComboBox(NULL),
	m_lineEdit(NULL),
	m_spinBox(NULL),
	m_resetButton(NULL),
	m_saveButton(NULL),
	m_filePathWidget(NULL),
	m_option(option),
	m_index(index)
{
	const QVariant currentValue = (value.isNull() ? SettingsManager::getValue(option) : value);

	if (type == QLatin1String("color"))
	{
		m_widget = m_colorButton = new QPushButton(this);

		QPalette palette = m_colorButton->palette();
		palette.setColor(QPalette::Button, currentValue.value<QColor>());

		m_colorButton->setPalette(palette);

		connect(m_colorButton, SIGNAL(clicked()), this, SLOT(selectColor()));
	}
	else if (type == QLatin1String("font"))
	{
		m_widget = m_fontComboBox = new QFontComboBox(this);

		m_fontComboBox->setCurrentFont(QFont(currentValue.toString()));
		m_fontComboBox->lineEdit()->selectAll();

		connect(m_fontComboBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(markModified()));
	}
	else if (type == QLatin1String("icon"))
	{
		m_widget = m_iconWidget = new IconWidget(this);

		if (currentValue.type() == QVariant::String)
		{
			m_iconWidget->setIcon(currentValue.toString());
		}
		else
		{
			m_iconWidget->setIcon(currentValue.value<QIcon>());
		}

		connect(m_iconWidget, SIGNAL(iconChanged(QIcon)), this, SLOT(markModified()));
	}
	else if (type == QLatin1String("integer"))
	{
		m_widget = m_spinBox = new QSpinBox(this);

		m_spinBox->setMinimum(-999999999);
		m_spinBox->setMaximum(999999999);
		m_spinBox->setValue(currentValue.toInt());
		m_spinBox->selectAll();

		connect(m_spinBox, SIGNAL(valueChanged(int)), this, SLOT(markModified()));
	}
	else if (type == QLatin1String("path"))
	{
		m_widget = m_filePathWidget = new FilePathWidget(this);

		m_filePathWidget->setPath(currentValue.toString());
		m_filePathWidget->setSelectFile(false);

		connect(m_filePathWidget, SIGNAL(pathChanged(QString)), this, SLOT(markModified()));
	}
	else
	{
		if (type != QLatin1String("bool") && choices.isEmpty())
		{
			m_widget = m_lineEdit = new QLineEdit(currentValue.toString(), this);

			m_lineEdit->selectAll();

			connect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(markModified()));
		}
		else
		{
			m_widget = m_comboBox = new QComboBox(this);

			if (type == QLatin1String("bool"))
			{
				m_comboBox->addItem(QLatin1String("false"));
				m_comboBox->addItem(QLatin1String("true"));
			}
			else
			{
				for (int j = 0; j < choices.count(); ++j)
				{
					m_comboBox->addItem(choices.at(j));
				}
			}

			m_comboBox->setCurrentText(currentValue.toString());

			connect(m_comboBox, SIGNAL(currentTextChanged(QString)), this, SLOT(markModified()));
		}
	}

	setAutoFillBackground(false);

	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->setMargin(0);
	layout->addWidget(m_widget);

	if (!isSimple)
	{
		m_resetButton = new QPushButton(tr("Defaults"), this);
		m_resetButton->setEnabled(currentValue != SettingsManager::getDefaultValue(option));

		m_saveButton = new QPushButton(tr("Save"), this);

		layout->addWidget(m_resetButton);
		layout->addWidget(m_saveButton);

		connect(m_resetButton, SIGNAL(clicked()), this, SLOT(reset()));
		connect(m_saveButton, SIGNAL(clicked()), this, SLOT(save()));
	}

	setLayout(layout);

	m_widget->setAutoFillBackground(false);
	m_widget->setFocusPolicy(Qt::StrongFocus);
	m_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

void OptionWidget::focusInEvent(QFocusEvent *event)
{
	QWidget::focusInEvent(event);

	if (m_widget)
	{
		m_widget->setFocus();
	}
}

void OptionWidget::selectColor()
{
	QColorDialog dialog(this);
	dialog.setCurrentColor(m_colorButton->palette().color(QPalette::Button));

	if (dialog.exec() == QDialog::Accepted)
	{
		QPalette palette = m_colorButton->palette();
		palette.setColor(QPalette::Button, dialog.currentColor());

		m_colorButton->setPalette(palette);

		markModified();
	}
}

void OptionWidget::markModified()
{
	if (m_resetButton)
	{
		m_resetButton->setEnabled(getValue() != SettingsManager::getDefaultValue(m_option));
	}
	else
	{
		emit commitData(this);
	}
}

void OptionWidget::reset()
{
	const QVariant value = SettingsManager::getDefaultValue(m_option);

	if (m_colorButton)
	{
		QPalette palette = m_colorButton->palette();
		palette.setColor(QPalette::Button, value.value<QColor>());

		m_colorButton->setPalette(palette);
	}
	else if (m_comboBox)
	{
		m_comboBox->setCurrentText(value.toString());
	}
	else if (m_filePathWidget)
	{
		m_filePathWidget->setPath(value.toString());
	}
	else if (m_fontComboBox)
	{
		m_fontComboBox->setCurrentFont(QFont(value.toString()));
	}
	else if (m_iconWidget)
	{
		if (value.type() == QVariant::String)
		{
			m_iconWidget->setIcon(value.toString());
		}
		else
		{
			m_iconWidget->setIcon(value.value<QIcon>());
		}
	}
	else if (m_lineEdit)
	{
		m_lineEdit->setText(value.toString());
	}
	else if (m_spinBox)
	{
		m_spinBox->setValue(value.toInt());
	}

	m_resetButton->setEnabled(false);
}

void OptionWidget::save()
{
	SettingsManager::setValue(m_option, getValue());
}

QString OptionWidget::getOption() const
{
	return m_option;
}

QVariant OptionWidget::getValue() const
{
	if (m_colorButton)
	{
		return m_colorButton->palette().color(QPalette::Button);
	}

	if (m_comboBox)
	{
		return m_comboBox->currentText();
	}

	if (m_filePathWidget)
	{
		return m_filePathWidget->getPath();
	}

	if (m_fontComboBox)
	{
		return m_fontComboBox->currentFont().family();
	}

	if (m_iconWidget)
	{
		return m_iconWidget->getIcon();
	}

	if (m_lineEdit)
	{
		return m_lineEdit->text();
	}

	if (m_spinBox)
	{
		return m_spinBox->value();
	}

	return QVariant();
}

QModelIndex OptionWidget::getIndex() const
{
	return m_index;
}

}
