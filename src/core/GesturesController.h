/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2020 - 2021 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_GESTURESCONTROLLER_H
#define OTTER_GESTURESCONTROLLER_H

#include "GesturesManager.h"

namespace Otter
{

class GesturesController
{
public:
	struct GestureContext
	{
		QVariantMap parameters;
		QVector<GesturesManager::GesturesContext> contexts;
		bool canPropagateEvent = true;
	};

	explicit GesturesController();
	virtual ~GesturesController();

	virtual GestureContext getGestureContext(const QPoint &position = {}) const;

protected:
	void installGesturesFilter(QObject *object, GesturesController *target, const QVector<GesturesManager::GesturesContext> &gesturesContexts = {}, bool canPropagateEvents = true);

private:
	QVector<GesturesManager::GesturesContext> m_gesturesContexts;
	bool m_canPropagateEvents;
};

class GesturesFilter : public QObject
{
public:
	explicit GesturesFilter(QObject *object, GesturesController *controller);

	bool eventFilter(QObject *object, QEvent *event) override;

private:
	GesturesController *m_controller;
};

}

#endif
