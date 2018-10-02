/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "Dialog.h"
#include "../core/JsonSettings.h"
#include "../core/SessionsManager.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

namespace Otter
{

Dialog::Dialog(QWidget *parent) : QDialog(parent),
	m_wasRestored(false)
{
}

void Dialog::showEvent(QShowEvent *event)
{
	if (!m_wasRestored)
	{
		QFile file(SessionsManager::getWritableDataPath(QLatin1String("dialogs.json")));

		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			adjustSize();
		}
		else
		{
			const QString name(Utils::normalizeObjectName(objectName(), QLatin1String("Dialog")));
			const QJsonObject settingsObject(QJsonDocument::fromJson(file.readAll()).object());

			file.close();

			if (settingsObject.contains(name))
			{
				const QJsonObject sizeObject(settingsObject.value(name).toObject().value(QLatin1String("size")).toObject());

				if (!sizeObject.isEmpty() && sizeObject.value(QLatin1String("width")).toInt() > 0 && sizeObject.value(QLatin1String("height")).toInt() > 0)
				{
					resize(sizeObject.value(QLatin1String("width")).toInt(), sizeObject.value(QLatin1String("height")).toInt());
				}
			}
		}

		m_wasRestored = true;
	}

	QDialog::showEvent(event);
}

void Dialog::resizeEvent(QResizeEvent *event)
{
	QDialog::resizeEvent(event);

	if (!m_wasRestored)
	{
		return;
	}

	JsonSettings settings(SessionsManager::getWritableDataPath(QLatin1String("dialogs.json")));
	QJsonObject settingsObject(settings.object());
	const QString name(Utils::normalizeObjectName(objectName(), QLatin1String("Dialog")));
	QJsonObject dialogObject(settingsObject.value(name).toObject());
	dialogObject.insert(QLatin1String("size"), QJsonObject({{QLatin1String("width"), width()}, {QLatin1String("height"), height()}}));

	settingsObject.insert(name, dialogObject);

	settings.setObject(settingsObject);
	settings.save();
}

}
