/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_OPTIONWIDGET_H
#define OTTER_OPTIONWIDGET_H

#include "../core/SettingsManager.h"

#include <QtWidgets/QComboBox>
#include <QtWidgets/QFontComboBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>

namespace Otter
{

class ColorWidget;
class FilePathWidget;
class IconWidget;

class OptionWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit OptionWidget(const QVariant &value, SettingsManager::OptionType type, QWidget *parent = nullptr);

	void setDefaultValue(const QVariant &value);
	void setValue(const QVariant &value);
	void setChoices(const QStringList &choices);
	void setChoices(const QVector<SettingsManager::OptionDefinition::ChoiceDefinition> &choices);
	void setSizePolicy(QSizePolicy::Policy horizontal, QSizePolicy::Policy vertical);
	void setSizePolicy(QSizePolicy policy);
	QVariant getDefaultValue() const;
	QVariant getValue() const;

protected:
	void focusInEvent(QFocusEvent *event) override;

protected slots:
	void markAsModified();
	void reset();

private:
	QWidget *m_widget;
	ColorWidget *m_colorWidget;
	FilePathWidget *m_filePathWidget;
	IconWidget *m_iconWidget;
	QComboBox *m_comboBox;
	QFontComboBox *m_fontComboBox;
	QLineEdit *m_lineEdit;
	QSpinBox *m_spinBox;
	QPushButton *m_resetButton;
	QVariant m_defaultValue;
	SettingsManager::OptionType m_type;

signals:
	void commitData(QWidget *editor);
};

}

#endif
