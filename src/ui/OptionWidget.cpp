/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../core/FileSystemCompleterModel.h"
#include "../core/SettingsManager.h"

#include <QtWidgets/QColorDialog>
#include <QtWidgets/QCompleter>
#include <QtWidgets/QHBoxLayout>

namespace Otter
{

OptionWidget::OptionWidget(bool simple, const QString &option, const QString &type, const QVariant &value, const QStringList &choices, const QModelIndex &index, QWidget *parent) : QWidget(parent),
	m_option(option),
	m_index(index),
	m_resetButton(NULL),
	m_saveButton(NULL),
	m_colorButton(NULL),
	m_comboBox(NULL),
	m_fontComboBox(NULL),
	m_lineEdit(NULL),
	m_spinBox(NULL)
{
	const QVariant currentValue = (value.isNull() ? SettingsManager::getValue(option) : value);
	QWidget *widget = NULL;

	if (type == QLatin1String("color"))
	{
		widget = m_colorButton = new QPushButton(this);

		QPalette palette = m_colorButton->palette();
		palette.setColor(QPalette::Button, currentValue.value<QColor>());

		m_colorButton->setPalette(palette);

		connect(m_colorButton, SIGNAL(clicked()), this, SLOT(selectColor()));
	}
	else if (type == QLatin1String("font"))
	{
		widget = m_fontComboBox = new QFontComboBox(this);

		m_fontComboBox->setCurrentFont(QFont(currentValue.toString()));
		m_fontComboBox->lineEdit()->selectAll();

		connect(m_fontComboBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(modified()));
	}
	else if (type == QLatin1String("integer"))
	{
		widget = m_spinBox = new QSpinBox(this);

		m_spinBox->setMinimum(-99999);
		m_spinBox->setMaximum(99999);
		m_spinBox->setValue(currentValue.toInt());
		m_spinBox->selectAll();

		connect(m_spinBox, SIGNAL(valueChanged(int)), this, SLOT(modified()));
	}
	else
	{
		if (type != QLatin1String("bool") && choices.isEmpty())
		{
			widget = m_lineEdit = new QLineEdit(currentValue.toString(), this);

			if (type == QLatin1String("path"))
			{
				m_lineEdit->setCompleter(new QCompleter(new FileSystemCompleterModel(this), this));
			}

			m_lineEdit->selectAll();

			connect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(modified()));
		}
		else
		{
			widget = m_comboBox = new QComboBox(this);

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

			connect(m_comboBox, SIGNAL(currentTextChanged(QString)), this, SLOT(modified()));
		}
	}

	setAutoFillBackground(false);
	setFocus();

	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->setMargin(0);
	layout->addWidget(widget);

	if (!simple)
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

	widget->setAutoFillBackground(false);
	widget->setFocusPolicy(Qt::StrongFocus);
	widget->setFocus();
	widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
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

		modified();
	}
}

void OptionWidget::modified()
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
	else if (m_fontComboBox)
	{
		m_fontComboBox->setCurrentFont(QFont(value.toString()));
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

QVariant OptionWidget::getValue() const
{
	if (m_colorButton)
	{
		return m_colorButton->palette().color(QPalette::Button);
	}
	else if (m_comboBox)
	{
		return m_comboBox->currentText();
	}
	else if (m_fontComboBox)
	{
		return m_fontComboBox->currentFont().family();
	}
	else if (m_lineEdit)
	{
		return m_lineEdit->text();
	}
	else if (m_spinBox)
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
