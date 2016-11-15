/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 - 2016 Piotr Wójcik <chocimier@tlen.pl>
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

#include "../core/Utils.h"
#include "../../3rdparty/mousegestures/MouseGestures.h"

namespace Otter
{

class GesturesManager : public QObject
{
	Q_OBJECT

public:
	enum GesturesContext
	{
		UnknownGesturesContext = 0,
		GenericGesturesContext,
		LinkGesturesContext,
		ContentEditableGesturesContext,
		ToolBarGesturesContext,
		TabHandleGesturesContext,
		ActiveTabHandleGesturesContext,
		NoTabHandleGesturesContext,
		OtherGesturesContext
	};

	static void createInstance(QObject *parent = nullptr);
	static void loadProfiles();
	static GesturesManager* getInstance();
	static bool startGesture(QObject *object, QEvent *event, QList<GesturesContext> contexts = QList<GesturesContext>({GenericGesturesContext}), const QVariantMap &parameters = QVariantMap());
	static bool isTracking();

protected:
	struct GestureStep
	{
		QEvent::Type type;
		Qt::MouseButton button;
		Qt::KeyboardModifiers modifiers;
		MouseGestures::MouseAction direction;

		GestureStep();
		GestureStep(QEvent::Type type, MouseGestures::MouseAction direction, Qt::KeyboardModifiers modifiers = Qt::NoModifier);
		GestureStep(QEvent::Type type, Qt::MouseButton button, Qt::KeyboardModifiers modifiers = Qt::NoModifier);
		explicit GestureStep(const QInputEvent *event);

		bool operator ==(const GestureStep &other);
		bool operator !=(const GestureStep &other);
	};

	struct MouseGesture
	{
		QList<GestureStep> steps;
		int action = 0;
	};

	explicit GesturesManager(QObject *parent);

	void timerEvent(QTimerEvent *event);
	static void releaseObject();
	static GestureStep deserializeStep(const QString &string);
	static QList<GestureStep> recognizeMoveStep(QInputEvent *event);
	static int matchGesture();
	static int getLastMoveDistance(bool measureFinished = false);
	static int gesturesDifference(QList<GestureStep> defined);
	static bool triggerAction(int gestureIdentifier);
	bool eventFilter(QObject *object, QEvent *event);

protected slots:
	void optionChanged(int identifier);
	void endGesture();

private:
	int m_reloadTimer;

	static GesturesManager *m_instance;
	static MouseGestures::Recognizer *m_recognizer;
	static QPointer<QObject> m_trackedObject;
	static QPoint m_lastClick;
	static QPoint m_lastPosition;
	static QVariantMap m_paramaters;
	static QHash<GesturesContext, QVector<MouseGesture> > m_gestures;
	static QHash<GesturesContext, QList<QList<GestureStep> > > m_nativeGestures;
	static QList<GestureStep> m_steps;
	static QList<QInputEvent*> m_events;
	static QList<GesturesContext> m_contexts;
	static bool m_isReleasing;
	static bool m_afterScroll;
};

}

#endif
