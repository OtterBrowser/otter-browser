/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Piotr Wójcik <chocimier@tlen.pl>
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
#include "ColorWidget.h"
#include "IconWidget.h"
#include "FilePathWidget.h"

#include <QtWidgets/QCompleter>
#include <QtWidgets/QHBoxLayout>

namespace Otter
{

OptionWidget::OptionWidget(const QString &option, const QVariant &value, SettingsManager::OptionType type, QWidget *parent) : QWidget(parent),
	m_widget(NULL),
	m_colorWidget(NULL),
	m_filePathWidget(NULL),
	m_iconWidget(NULL),
	m_comboBox(NULL),
	m_fontComboBox(NULL),
	m_lineEdit(NULL),
	m_spinBox(NULL),
	m_resetButton(NULL),
	m_saveButton(NULL),
	m_option(option),
	m_value(value)
{
	switch (type)
	{
		case SettingsManager::BooleanType:
			m_widget = m_comboBox = new QComboBox(this);

			m_comboBox->addItem(tr("No"), QLatin1String("false"));
			m_comboBox->addItem(tr("Yes"), QLatin1String("true"));
			m_comboBox->setCurrentIndex(m_value.toBool() ? 1 : 0);

			connect(m_comboBox, SIGNAL(currentTextChanged(QString)), this, SLOT(markModified()));

			break;
		case SettingsManager::ColorType:
			{
				m_widget = m_colorWidget = new ColorWidget(this);

				m_colorWidget->setColor(m_value.value<QColor>());

				connect(m_colorWidget, SIGNAL(colorChanged(QColor)), this, SLOT(markModified()));
			}

			break;
		case SettingsManager::EnumerationType:
			m_widget = m_comboBox = new QComboBox(this);

			connect(m_comboBox, SIGNAL(currentTextChanged(QString)), this, SLOT(markModified()));

			break;
		case SettingsManager::FontType:
			m_widget = m_fontComboBox = new QFontComboBox(this);

			m_fontComboBox->setCurrentFont(QFont(m_value.toString()));
			m_fontComboBox->lineEdit()->selectAll();

			connect(m_fontComboBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(markModified()));

			break;
		case SettingsManager::IconType:
			m_widget = m_iconWidget = new IconWidget(this);

			if (m_value.type() == QVariant::String)
			{
				m_iconWidget->setIcon(m_value.toString());
			}
			else
			{
				m_iconWidget->setIcon(m_value.value<QIcon>());
			}

			connect(m_iconWidget, SIGNAL(iconChanged(QIcon)), this, SLOT(markModified()));

			break;
		case SettingsManager::IntegerType:
			m_widget = m_spinBox = new QSpinBox(this);

			m_spinBox->setMinimum(-999999999);
			m_spinBox->setMaximum(999999999);
			m_spinBox->setValue(m_value.toInt());
			m_spinBox->selectAll();

			connect(m_spinBox, SIGNAL(valueChanged(int)), this, SLOT(markModified()));

			break;
		case SettingsManager::PathType:
			m_widget = m_filePathWidget = new FilePathWidget(this);

			m_filePathWidget->setPath(m_value.toString());
			m_filePathWidget->setSelectFile(false);

			connect(m_filePathWidget, SIGNAL(pathChanged(QString)), this, SLOT(markModified()));

			break;
		default:
			m_widget = m_lineEdit = new QLineEdit(m_value.toString(), this);

			m_lineEdit->setClearButtonEnabled(true);
			m_lineEdit->selectAll();

			connect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(markModified()));

			break;
	}

	setAutoFillBackground(false);

	QHBoxLayout *layout(new QHBoxLayout(this));
	layout->setMargin(0);
	layout->addWidget(m_widget);

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

void OptionWidget::markModified()
{
	if (m_resetButton)
	{
		m_resetButton->setEnabled(getValue() != SettingsManager::getOptionDefinition(SettingsManager::getOptionIdentifier(m_option)).defaultValue);
	}
	else
	{
		emit commitData(this);
	}
}

void OptionWidget::reset()
{
	const QVariant value(SettingsManager::getOptionDefinition(SettingsManager::getOptionIdentifier(m_option)).defaultValue);

	setValue(value);

	m_resetButton->setEnabled(false);
}

void OptionWidget::save()
{
	SettingsManager::setValue(SettingsManager::getOptionIdentifier(m_option), getValue());
}

void OptionWidget::setIndex(const QModelIndex &index)
{
	m_index = index;
}

void OptionWidget::setValue(const QVariant &value)
{
	m_value = value;

	if (m_colorWidget)
	{
		m_colorWidget->setColor(value.value<QColor>());
	}
	else if (m_comboBox)
	{
		m_comboBox->setCurrentIndex(qMax(0, m_comboBox->findData(value)));
	}
	else if (m_filePathWidget)
	{
		m_filePathWidget->setPath(value.toString());
	}
	else if (m_fontComboBox)
	{
		m_fontComboBox->setCurrentText(value.toString());
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
}

void OptionWidget::setChoices(const QStringList &choices)
{
	if (!m_comboBox)
	{
		return;
	}

	m_comboBox->clear();

	for (int i = 0; i < choices.count(); ++i)
	{
		m_comboBox->addItem(choices.at(i), choices.at(i));
	}

	m_comboBox->setCurrentText(m_value.toString());
}

void OptionWidget::setChoices(const QList<OptionWidget::EnumerationChoice> &choices)
{
	if (!m_comboBox)
	{
		return;
	}

	m_comboBox->clear();

	for (int i = 0; i < choices.count(); ++i)
	{
		m_comboBox->addItem(choices.at(i).icon, choices.at(i).text, choices.at(i).value);
	}

	m_comboBox->setCurrentIndex(qMax(0, m_comboBox->findData(m_value)));
}

void OptionWidget::setControlsVisible(bool isVisible)
{
	if (isVisible && !m_resetButton)
	{
		m_resetButton = new QPushButton(tr("Defaults"), this);
		m_resetButton->setEnabled(getValue() != SettingsManager::getOptionDefinition(SettingsManager::getOptionIdentifier(m_option)).defaultValue);

		m_saveButton = new QPushButton(tr("Save"), this);

		layout()->addWidget(m_resetButton);
		layout()->addWidget(m_saveButton);

		connect(m_resetButton, SIGNAL(clicked()), this, SLOT(reset()));
		connect(m_saveButton, SIGNAL(clicked()), this, SLOT(save()));
	}
	else if (!isVisible && m_resetButton)
	{
		m_resetButton->deleteLater();
		m_resetButton = NULL;

		m_saveButton->deleteLater();
		m_saveButton = NULL;
	}
}

void OptionWidget::setSizePolicy(QSizePolicy::Policy horizontal, QSizePolicy::Policy vertical)
{
	QWidget::setSizePolicy(horizontal, vertical);

	m_widget->setSizePolicy(horizontal, vertical);
}

void OptionWidget::setSizePolicy(QSizePolicy policy)
{
	QWidget::setSizePolicy(policy);

	m_widget->setSizePolicy(policy);
}

QString OptionWidget::getOption() const
{
	return m_option;
}

QVariant OptionWidget::getValue() const
{
	if (m_colorWidget)
	{
		return m_colorWidget->getColor();
	}

	if (m_comboBox)
	{
		return m_comboBox->currentData();
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
		if (m_lineEdit->text().isEmpty())
		{
			return QVariant();
		}

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
