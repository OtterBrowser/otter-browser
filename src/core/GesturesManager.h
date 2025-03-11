/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 - 2017 Piotr Wójcik <chocimier@tlen.pl>
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

#include "AddonsManager.h"
#include "../../3rdparty/mousegestures/MouseGestures.h"

#include <QtGui/QInputEvent>

namespace Otter
{

class MouseProfile final : public JsonAddon
{
public:
	enum LoadMode
	{
		StandardMode = 0,
		MetaDataOnlyMode,
		FullMode
	};

	struct Gesture final
	{
		struct Step final
		{
			QEvent::Type type = QEvent::None;
			Qt::MouseButton button = Qt::NoButton;
			Qt::KeyboardModifiers modifiers = Qt::NoModifier;
			MouseGestures::MouseAction direction = MouseGestures::UnknownMouseAction;

			Step();
			Step(QEvent::Type typeValue, MouseGestures::MouseAction directionValue, Qt::KeyboardModifiers modifiersValue = Qt::NoModifier);
			Step(QEvent::Type typeValue, Qt::MouseButton buttonValue, Qt::KeyboardModifiers modifiersValue = Qt::NoModifier);
			explicit Step(const QInputEvent *event);

			QString toString() const;
			static Step fromString(const QString &string);
			bool operator ==(const Step &other) const;
			bool operator !=(const Step &other) const;
		};

		QVector<Step> steps;
		QVariantMap parameters;
		int action = 0;

		bool operator ==(const Gesture &other) const;
	};

	explicit MouseProfile(const QString &identifier = {}, LoadMode mode = StandardMode);

	void setDefinitions(const QHash<int, QVector<Gesture> > &definitions);
	QString getName() const override;
	QHash<int, QVector<Gesture> > getDefinitions() const;
	AddonType getType() const override;
	bool isValid() const;
	bool save();

private:
	QString m_identifier;
	QHash<int, QVector<Gesture> > m_definitions;
};

class GesturesManager final : public QObject
{
	Q_OBJECT

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
		ItemViewContext,
		OtherContext
	};

	Q_ENUM(GesturesContext)

	static void createInstance();
	static void loadProfiles();
	static void cancelGesture();
	static GesturesManager* getInstance();
	static QObject* getTrackedObject();
	static QString getContextName(int identifier);
	static int getContextIdentifier(const QString &name);
	static bool startGesture(QObject *object, QEvent *event, const QVector<GesturesContext> &contexts = {GenericContext}, const QVariantMap &parameters = {});
	static bool continueGesture(QObject *object);
	static bool isTracking();

protected:
	explicit GesturesManager(QObject *parent);

	void timerEvent(QTimerEvent *event) override;
	static void recognizeMoveStep(const QInputEvent *event);
	static void releaseTrackedObject();
	static MouseProfile::Gesture matchGesture();
	static int calculateLastMoveDistance(bool measureFinished = false);
	static int calculateGesturesDifference(const QVector<MouseProfile::Gesture::Step> &steps);
	static bool triggerAction(const MouseProfile::Gesture &gesture);
	bool eventFilter(QObject *object, QEvent *event) override;

protected slots:
	void endGesture();

private:
	int m_reloadTimer;

	static GesturesManager *m_instance;
	static MouseGestures::Recognizer *m_recognizer;
	static QPointer<QObject> m_trackedObject;
	static QPoint m_lastClick;
	static QPoint m_lastLocalPosition;
	static QPoint m_lastGlobalPosition;
	static QVariantMap m_parameters;
	static QHash<GesturesContext, QVector<MouseProfile::Gesture> > m_gestures;
	static QHash<GesturesContext, QVector<QVector<MouseProfile::Gesture::Step> > > m_nativeGestures;
	static QVector<MouseProfile::Gesture::Step> m_steps;
	static QVector<QInputEvent*> m_events;
	static QVector<GesturesContext> m_contexts;
	static int m_gesturesContextEnumerator;
	static bool m_isReleasing;
	static bool m_isAfterScroll;
};

}

#endif
