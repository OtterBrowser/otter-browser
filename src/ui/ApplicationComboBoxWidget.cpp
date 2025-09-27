/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ApplicationComboBoxWidget.h"
#include "../core/ItemModel.h"
#include "../core/Utils.h"

#include <QtCore/QEvent>
#include <QtWidgets/QFileDialog>

namespace Otter
{

ApplicationComboBoxWidget::ApplicationComboBoxWidget(QWidget *parent) : QComboBox(parent),
	m_previousIndex(0),
	m_alwaysShowDefaultApplication(false)
{
	addItem(tr("Default Application"));
	insertSeparator(1);
	addItem(tr("Other…"));

	connect(this, static_cast<void(ApplicationComboBoxWidget::*)(int)>(&ApplicationComboBoxWidget::currentIndexChanged), this, &ApplicationComboBoxWidget::handleIndexChanged);
}

void ApplicationComboBoxWidget::changeEvent(QEvent *event)
{
	QComboBox::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		setItemData(0, tr("Default Application"), Qt::DisplayRole);
		setItemData((count() - 1), tr("Other…"), Qt::DisplayRole);
	}
}

void ApplicationComboBoxWidget::handleIndexChanged(int index)
{
	if (index == (count() - 1))
	{
		disconnect(this, static_cast<void(ApplicationComboBoxWidget::*)(int)>(&ApplicationComboBoxWidget::currentIndexChanged), this, &ApplicationComboBoxWidget::handleIndexChanged);

		setCurrentIndex(m_previousIndex);

		const QString path(QFileDialog::getOpenFileName(this, tr("Select Application"), Utils::getStandardLocation(QStandardPaths::ApplicationsLocation)));

		if (!path.isEmpty())
		{
			if (count() == 3)
			{
				insertSeparator(2);
			}

			m_previousIndex = (count() - 2);

			insertItem(m_previousIndex, QFileInfo(path).baseName(), path);
			setCurrentIndex(m_previousIndex);

			emit currentCommandChanged();
		}

		connect(this, static_cast<void(ApplicationComboBoxWidget::*)(int)>(&ApplicationComboBoxWidget::currentIndexChanged), this, &ApplicationComboBoxWidget::handleIndexChanged);
	}
	else
	{
		m_previousIndex = index;

		emit currentCommandChanged();
	}
}

void ApplicationComboBoxWidget::setCurrentCommand(const QString &command)
{
	if (command.isEmpty())
	{
		if (itemData(0, Qt::UserRole).toString().isEmpty())
		{
			setCurrentIndex(0);
		}

		return;
	}

	int index(findData(command));

	if (index < 0)
	{
		if (count() == 3)
		{
			insertSeparator(1);
		}

		index = (count() - 2);

		insertItem(index, command, command);
	}

	setCurrentIndex(index);
}

void ApplicationComboBoxWidget::setMimeType(const QMimeType &mimeType)
{
	disconnect(this, static_cast<void(ApplicationComboBoxWidget::*)(int)>(&ApplicationComboBoxWidget::currentIndexChanged), this, &ApplicationComboBoxWidget::handleIndexChanged);

	clear();

	const QVector<ApplicationInformation> applications(Utils::getApplicationsForMimeType(mimeType));
	const bool onlyDefaultApplication(applications.isEmpty());

	if (m_alwaysShowDefaultApplication || onlyDefaultApplication)
	{
		addItem(tr("Default Application"));
		insertSeparator(1);
	}

	if (!onlyDefaultApplication)
	{
		for (int i = 0; i < applications.count(); ++i)
		{
			const ApplicationInformation application(applications.at(i));

			addItem(application.icon, ((application.name.isEmpty()) ? tr("Unknown") : application.name), application.command);

			if (application.icon.isNull())
			{
				model()->setData(model()->index((count() - 1), 0), ItemModel::createDecoration(), Qt::DecorationRole);
			}
		}
	}

	if (count() > 2)
	{
		insertSeparator(count() + 1);
	}

	addItem(tr("Other…"));

	connect(this, static_cast<void(ApplicationComboBoxWidget::*)(int)>(&ApplicationComboBoxWidget::currentIndexChanged), this, &ApplicationComboBoxWidget::handleIndexChanged);
}

void ApplicationComboBoxWidget::setAlwaysShowDefaultApplication(bool show)
{
	m_alwaysShowDefaultApplication = show;
}

QString ApplicationComboBoxWidget::getCommand() const
{
	return currentData().toString();
}

}
