#include "OptionWidget.h"
#include "../core/SettingsManager.h"

#include <QtWidgets/QColorDialog>
#include <QtWidgets/QHBoxLayout>

namespace Otter
{

OptionWidget::OptionWidget(const QString &option, const QString &type, const QStringList &choices, QWidget *parent) : QWidget(parent),
	m_option(option),
	m_resetButton(new QPushButton(tr("Defaults"), this)),
	m_saveButton(new QPushButton(tr("Save"), this)),
	m_colorButton(NULL),
	m_comboBox(NULL),
	m_fontComboBox(NULL),
	m_lineEdit(NULL),
	m_spinBox(NULL)
{
	const QVariant value = SettingsManager::getValue(option);
	QWidget *widget = NULL;

	if (type == "color")
	{
		widget = m_colorButton = new QPushButton(this);

		QPalette palette = m_colorButton->palette();
		palette.setColor(QPalette::Button, value.value<QColor>());

		m_colorButton->setPalette(palette);

		connect(m_colorButton, SIGNAL(clicked()), this, SLOT(selectColor()));
	}
	else if (type == "font")
	{
		widget = m_fontComboBox = new QFontComboBox(this);

		m_fontComboBox->setCurrentFont(QFont(value.toString()));

		connect(m_fontComboBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(modified()));
	}
	else if (type == "integer")
	{
		widget = m_spinBox = new QSpinBox(this);

		m_spinBox->setMinimum(-99999);
		m_spinBox->setMaximum(99999);
		m_spinBox->setValue(value.toInt());

		connect(m_spinBox, SIGNAL(valueChanged(int)), this, SLOT(modified()));
	}
	else
	{
		if (type != "bool" && choices.isEmpty())
		{
			widget = m_lineEdit = new QLineEdit(value.toString(), this);

			connect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(modified()));
		}
		else
		{
			widget = m_comboBox = new QComboBox(this);

			if (type == "bool")
			{
				m_comboBox->addItem("false");
				m_comboBox->addItem("true");
			}
			else
			{
				for (int j = 0; j < choices.count(); ++j)
				{
					m_comboBox->addItem(choices.at(j));
				}
			}

			m_comboBox->setCurrentText(value.toString());

			connect(m_comboBox, SIGNAL(currentTextChanged(QString)), this, SLOT(modified()));
		}
	}

	widget->setAutoFillBackground(false);
	widget->setFocus();
	widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	setAutoFillBackground(false);

	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->setMargin(0);
	layout->addWidget(widget);
	layout->addWidget(m_resetButton);
	layout->addWidget(m_saveButton);

	setLayout(layout);

	m_resetButton->setEnabled(value != SettingsManager::getDefaultValue(option));

	connect(m_resetButton, SIGNAL(clicked()), this, SLOT(reset()));
	connect(m_saveButton, SIGNAL(clicked()), this, SLOT(save()));
}

void OptionWidget::selectColor()
{
	QColorDialog dialog(this);
	dialog.setCurrentColor(m_colorButton->palette().color(QPalette::Button));

	if (dialog.exec() == QDialog::Accepted)
	{
		m_resetButton->setEnabled(QVariant(dialog.currentColor()) != SettingsManager::getDefaultValue(m_option));

		QPalette palette = m_colorButton->palette();
		palette.setColor(QPalette::Button, dialog.currentColor());

		m_colorButton->setPalette(palette);
	}
}

void OptionWidget::modified()
{
	QVariant value;

	if (m_comboBox)
	{
		value = m_comboBox->currentText();
	}
	else if (m_fontComboBox)
	{
		value = m_fontComboBox->currentFont().family();
	}
	else if (m_lineEdit)
	{
		value = m_lineEdit->text();
	}
	else if (m_spinBox)
	{
		value = m_spinBox->value();
	}

	m_resetButton->setEnabled(value != SettingsManager::getDefaultValue(m_option));
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
	QVariant value;

	if (m_colorButton)
	{
		value = m_colorButton->palette().color(QPalette::Button);
	}
	else if (m_comboBox)
	{
		value = m_comboBox->currentText();
	}
	else if (m_fontComboBox)
	{
		value = m_fontComboBox->currentFont().family();
	}
	else if (m_lineEdit)
	{
		value = m_lineEdit->text();
	}
	else if (m_spinBox)
	{
		value = m_spinBox->value();
	}

	SettingsManager::setValue(m_option, value);
}

}
