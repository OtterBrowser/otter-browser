/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 - 2016 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_GESTURESMANAGER_H
#define OTTER_GESTURESMANAGER_H

#include <QtCore/QObject>
#include <QtGui/QMouseEvent>

#include "../../3rdparty/mousegestures/MouseGestures.h"

namespace Otter
{

class GesturesManager : public QObject
{
	Q_OBJECT
	Q_ENUMS(GesturesContext)

public:
	enum GesturesContext
	{
		UnknownContext = 0,
		GenericContext,
		LinkContext,
		ContentEditableContext,
		ToolBarContext,
		TabHandleContext,
		ActiveTabHandleContext,
		NoTabHandleContext,
		OtherContext
	};

	static void createInstance(QObject *parent = nullptr);
	static void loadProfiles();
	static void cancelGesture();
	static GesturesManager* getInstance();
	static QObject* getTrackedObject();
	static bool startGesture(QObject *object, QEvent *event, QVector<GesturesContext> contexts = QVector<GesturesContext>({GenericContext}), const QVariantMap &parameters = QVariantMap());
	static bool continueGesture(QObject *object);
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

		QString toString() const;
		bool operator ==(const GestureStep &other) const;
		bool operator !=(const GestureStep &other) const;
	};

	struct MouseGesture
	{
		QVector<GestureStep> steps;
		int action = 0;
	};

	explicit GesturesManager(QObject *parent);

	void timerEvent(QTimerEvent *event) override;
	static void recognizeMoveStep(QInputEvent *event);
	static void releaseObject();
	static GestureStep deserializeStep(const QString &string);
	static int matchGesture();
	static int calculateLastMoveDistance(bool measureFinished = false);
	static int calculateGesturesDifference(const QVector<GestureStep> &steps);
	static bool triggerAction(int gestureIdentifier);
	bool eventFilter(QObject *object, QEvent *event) override;

protected slots:
	void endGesture();
	void handleOptionChanged(int identifier);

private:
	int m_reloadTimer;

	static GesturesManager *m_instance;
	static MouseGestures::Recognizer *m_recognizer;
	static QPointer<QObject> m_trackedObject;
	static QPoint m_lastClick;
	static QPoint m_lastPosition;
	static QVariantMap m_paramaters;
	static QHash<GesturesContext, QVector<MouseGesture> > m_gestures;
	static QHash<GesturesContext, QVector<QVector<GestureStep> > > m_nativeGestures;
	static QVector<GestureStep> m_steps;
	static QVector<QInputEvent*> m_events;
	static QVector<GesturesContext> m_contexts;
	static int m_gesturesContextEnumerator;
	static bool m_isReleasing;
	static bool m_afterScroll;
};

}

#endif
