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

#ifndef OTTER_GESTURESMANAGER_H
#define OTTER_GESTURESMANAGER_H

#include <QtCore/QObject>
#include <QtGui/QMouseEvent>

class QjtMouseGesture;
class QjtMouseGestureFilter;

namespace Otter
{

class GesturesManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static void loadProfiles();
	static void startGesture(QObject *object, QMouseEvent *event);
	static bool endGesture(QObject *object, QMouseEvent *event);
	static GesturesManager* getInstance();

protected:
	explicit GesturesManager(QObject *parent);

protected slots:
	void optionChanged(const QString &option);
	void triggerGesture();

private:
	static GesturesManager *m_instance;
	static QjtMouseGestureFilter *m_filter;
	static QHash<QjtMouseGesture*, QString> m_gestures;
};

}

#endif
