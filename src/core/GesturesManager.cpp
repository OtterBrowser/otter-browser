/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "GesturesManager.h"
#include "Application.h"
#include "IniSettings.h"
#include "JsonSettings.h"
#include "SessionsManager.h"
#include "SettingsManager.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QMetaEnum>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QTextStream>

#include <limits>

#define UNKNOWN_GESTURE -1
#define NATIVE_GESTURE -2

namespace Otter
{

MouseProfile::Gesture::Step::Step()
{
}

MouseProfile::Gesture::Step::Step(QEvent::Type typeValue, MouseGestures::MouseAction directionValue, Qt::KeyboardModifiers modifiersValue) : type(typeValue),
	modifiers(modifiersValue),
	direction(directionValue)
{
}

MouseProfile::Gesture::Step::Step(QEvent::Type typeValue, Qt::MouseButton buttonValue, Qt::KeyboardModifiers modifiersValue) : type(typeValue),
	button(buttonValue),
	modifiers(modifiersValue)
{
}

MouseProfile::Gesture::Step::Step(const QInputEvent *event) : type(event->type()),
	modifiers(event->modifiers())
{
	switch (event->type())
	{
		case QEvent::MouseButtonPress:
		case QEvent::MouseButtonRelease:
		case QEvent::MouseButtonDblClick:
			button = static_cast<const QMouseEvent*>(event)->button();

			break;
		case QEvent::Wheel:
			{
				const QPoint delta(static_cast<const QWheelEvent*>(event)->angleDelta());

				if (qAbs(delta.x()) > qAbs(delta.y()))
				{
					direction = (delta.x() > 0) ? MouseGestures::MoveRightMouseAction : MouseGestures::MoveLeftMouseAction;
				}
				else if (qAbs(delta.y()) > 0)
				{
					direction = (delta.y() > 0) ? MouseGestures::MoveUpMouseAction : MouseGestures::MoveDownMouseAction;
				}
			}

			break;
		default:
			break;
	}
}

QString MouseProfile::Gesture::Step::toString() const
{
	QString string;

	switch (type)
	{
		case QEvent::MouseButtonPress:
			string += QLatin1String("press");

			break;
		case QEvent::MouseButtonRelease:
			string += QLatin1String("release");

			break;
		case QEvent::MouseButtonDblClick:
			string += QLatin1String("doubleClick");

			break;
		case QEvent::Wheel:
			string += QLatin1String("scroll");

			break;
		case QEvent::MouseMove:
			string += QLatin1String("move");

			break;
		default:
			break;
	}

	if (type == QEvent::Wheel || type == QEvent::MouseMove)
	{
		switch (direction)
		{
			case MouseGestures::MoveUpMouseAction:
				string += QLatin1String("Up");

				break;
			case MouseGestures::MoveDownMouseAction:
				string += QLatin1String("Down");

				break;
			case MouseGestures::MoveLeftMouseAction:
				string += QLatin1String("Left");

				break;
			case MouseGestures::MoveRightMouseAction:
				string += QLatin1String("Right");

				break;
			case MouseGestures::MoveHorizontallyMouseAction:
				string += QLatin1String("Horizontal");

				break;
			case MouseGestures::MoveVerticallyMouseAction:
				string += QLatin1String("Vertical");

				break;
			default:
				break;
		}
	}
	else
	{
		switch (button)
		{
			case Qt::LeftButton:
				string += QLatin1String("Left");

				break;
			case Qt::RightButton:
				string += QLatin1String("Right");

				break;
			case Qt::MiddleButton:
				string += QLatin1String("Middle");

				break;
			case Qt::BackButton:
				string += QLatin1String("Back");

				break;
			case Qt::ForwardButton:
				string += QLatin1String("Forward");

				break;
			case Qt::TaskButton:
				string += QLatin1String("Task");

				break;
			default:
				break;
		}

		for (int i = 4; i <= 24; ++i)
		{
			if (button == static_cast<Qt::MouseButton>(Qt::ExtraButton1 << (i - 1)))
			{
				string += QStringLiteral("Extra%1").arg(i);
			}
		}
	}

	if (modifiers.testFlag(Qt::ShiftModifier))
	{
		string += QLatin1String("+shift");
	}

	if (modifiers.testFlag(Qt::ControlModifier))
	{
		string += QLatin1String("+ctrl");
	}

	if (modifiers.testFlag(Qt::AltModifier))
	{
		string += QLatin1String("+alt");
	}

	if (modifiers.testFlag(Qt::MetaModifier))
	{
		string += QLatin1String("+meta");
	}

	return string;
}

MouseProfile::Gesture::Step MouseProfile::Gesture::Step::fromString(const QString &string)
{
	Step step;
	const QStringList parts(string.split(QLatin1Char('+')));
	const QString event(parts.first());

	if (event.startsWith(QLatin1String("press")))
	{
		step.type = QEvent::MouseButtonPress;
	}
	else if (event.startsWith(QLatin1String("release")))
	{
		step.type = QEvent::MouseButtonRelease;
	}
	else if (event.startsWith(QLatin1String("doubleClick")))
	{
		step.type = QEvent::MouseButtonDblClick;
	}
	else if (event.startsWith(QLatin1String("scroll")))
	{
		step.type = QEvent::Wheel;
	}
	else if (event.startsWith(QLatin1String("move")))
	{
		step.type = QEvent::MouseMove;
	}

	if (step.type == QEvent::Wheel || step.type == QEvent::MouseMove)
	{
		if (event.endsWith(QLatin1String("Up")))
		{
			step.direction = MouseGestures::MoveUpMouseAction;
		}
		else if (event.endsWith(QLatin1String("Down")))
		{
			step.direction = MouseGestures::MoveDownMouseAction;
		}
		else if (event.endsWith(QLatin1String("Left")))
		{
			step.direction = MouseGestures::MoveLeftMouseAction;
		}
		else if (event.endsWith(QLatin1String("Right")))
		{
			step.direction = MouseGestures::MoveRightMouseAction;
		}
		else if (event.endsWith(QLatin1String("Horizontal")))
		{
			step.direction = MouseGestures::MoveHorizontallyMouseAction;
		}
		else if (event.endsWith(QLatin1String("Vertical")))
		{
			step.direction = MouseGestures::MoveVerticallyMouseAction;
		}
	}
	else
	{
		if (event.endsWith(QLatin1String("Left")))
		{
			step.button = Qt::LeftButton;
		}
		else if (event.endsWith(QLatin1String("Right")))
		{
			step.button = Qt::RightButton;
		}
		else if (event.endsWith(QLatin1String("Middle")))
		{
			step.button = Qt::MiddleButton;
		}
		else if (event.endsWith(QLatin1String("Back")))
		{
			step.button = Qt::BackButton;
		}
		else if (event.endsWith(QLatin1String("Forward")))
		{
			step.button = Qt::ForwardButton;
		}
		else if (event.endsWith(QLatin1String("Task")))
		{
			step.button = Qt::TaskButton;
		}
		else
		{
			const int number(QRegularExpression(QLatin1String("Extra(\\d{1,2})$")).match(event).captured(1).toInt());

			if (1 <= number && number <= 24)
			{
				step.button = static_cast<Qt::MouseButton>(Qt::ExtraButton1 << (number - 1));
			}
		}
	}

	for (int i = 1; i < parts.count(); ++i)
	{
		if (parts.at(i) == QLatin1String("shift"))
		{
			step.modifiers |= Qt::ShiftModifier;
		}
		else if (parts.at(i) == QLatin1String("ctrl"))
		{
			step.modifiers |= Qt::ControlModifier;
		}
		else if (parts.at(i) == QLatin1String("alt"))
		{
			step.modifiers |= Qt::AltModifier;
		}
		else if (parts.at(i) == QLatin1String("meta"))
		{
			step.modifiers |= Qt::MetaModifier;
		}
	}

	return step;
}

bool MouseProfile::Gesture::Step::operator ==(const Step &other) const
{
	return (type == other.type && button == other.button && direction == other.direction && (modifiers == other.modifiers || type == QEvent::MouseMove));
}

bool MouseProfile::Gesture::Step::operator !=(const Step &other) const
{
	return !(*this == other);
}

bool MouseProfile::Gesture::operator ==(const Gesture &other) const
{
	return (steps == other.steps && parameters == other.parameters && action == other.action);
}

MouseProfile::MouseProfile(const QString &identifier, LoadMode mode) : Addon(),
	m_identifier(identifier),
	m_isModified(false)
{
	if (identifier.isEmpty())
	{
		return;
	}

	const JsonSettings settings(SessionsManager::getReadableDataPath(QLatin1String("mouse/") + identifier + QLatin1String(".json")));
	const QStringList comments(settings.getComment().split(QLatin1Char('\n')));

	for (int i = 0; i < comments.count(); ++i)
	{
		const QString key(comments.at(i).section(QLatin1Char(':'), 0, 0).trimmed());
		const QString value(comments.at(i).section(QLatin1Char(':'), 1).trimmed());

		if (key == QLatin1String("Title"))
		{
			m_title = value;
		}
		else if (key == QLatin1String("Description"))
		{
			m_description = value;
		}
		else if (key == QLatin1String("Author"))
		{
			m_author = value;
		}
		else if (key == QLatin1String("Version"))
		{
			m_version = value;
		}
	}

	if (mode == MetaDataOnlyMode)
	{
		return;
	}

	const QJsonArray contextsArray(settings.array());

	for (int i = 0; i < contextsArray.count(); ++i)
	{
		const QJsonObject contextObject(contextsArray.at(i).toObject());
		const GesturesManager::GesturesContext context(static_cast<GesturesManager::GesturesContext>(GesturesManager::getContextIdentifier(contextObject.value(QLatin1String("context")).toString())));

		if (context == GesturesManager::UnknownContext)
		{
			continue;
		}

		const QJsonArray gesturesArray(contextObject.value(QLatin1String("gestures")).toArray());

		for (int j = 0; j < gesturesArray.count(); ++j)
		{
			const QJsonObject actionObject(gesturesArray.at(j).toObject());
			const QJsonArray stepsArray(actionObject.value(QLatin1String("steps")).toArray());

			if (stepsArray.isEmpty())
			{
				continue;
			}

			const QString actionIdentifier(actionObject.value(QLatin1String("action")).toString());
			const bool isNativeGesture(actionIdentifier == QLatin1String("NoAction"));
			const int action(isNativeGesture ? NATIVE_GESTURE : ActionsManager::getActionIdentifier(actionIdentifier));

			if (action < 0 && !isNativeGesture)
			{
				continue;
			}

			QVector<MouseProfile::Gesture::Step> steps;
			steps.reserve(stepsArray.count());

			for (int k = 0; k < stepsArray.count(); ++k)
			{
				steps.append(Gesture::Step::fromString(stepsArray.at(k).toString()));
			}

			MouseProfile::Gesture definition;
			definition.steps = steps;
			definition.parameters = actionObject.value(QLatin1String("parameters")).toVariant().toMap();
			definition.action = action;

			m_definitions[context].append(definition);
		}
	}
}

void MouseProfile::setTitle(const QString &title)
{
	if (title != m_title)
	{
		m_title = title;
		m_isModified = true;
	}
}

void MouseProfile::setDescription(const QString &description)
{
	if (description != m_description)
	{
		m_description = description;
		m_isModified = true;
	}
}

void MouseProfile::setAuthor(const QString &author)
{
	if (author != m_author)
	{
		m_author = author;
		m_isModified = true;
	}
}

void MouseProfile::setVersion(const QString &version)
{
	if (version != m_version)
	{
		m_version = version;
		m_isModified = true;
	}
}

void MouseProfile::setDefinitions(const QHash<int, QVector<MouseProfile::Gesture> > &definitions)
{
	if (definitions != m_definitions)
	{
		m_definitions = definitions;
		m_isModified = true;
	}
}

void MouseProfile::setModified(bool isModified)
{
	m_isModified = isModified;
}

QString MouseProfile::getName() const
{
	return m_identifier;
}

QString MouseProfile::getTitle() const
{
	return (m_title.isEmpty() ? QCoreApplication::translate("Otter::MouseProfile", "(Untitled)") : m_title);
}

QString MouseProfile::getDescription() const
{
	return m_description;
}

QString MouseProfile::getAuthor() const
{
	return m_author;
}

QString MouseProfile::getVersion() const
{
	return m_version;
}

QHash<int, QVector<MouseProfile::Gesture> > MouseProfile::getDefinitions() const
{
	return m_definitions;
}

Addon::AddonType MouseProfile::getType() const
{
	return Addon::UnknownType;
}

bool MouseProfile::isModified() const
{
	return m_isModified;
}

bool MouseProfile::save()
{
	JsonSettings settings(SessionsManager::getWritableDataPath(QLatin1String("mouse/") + m_identifier + QLatin1String(".json")));
	QString comment;
	QTextStream stream(&comment);
	stream.setCodec("UTF-8");
	stream << QLatin1String("Title: ") << (m_title.isEmpty() ? QT_TR_NOOP("(Untitled)") : m_title) << QLatin1Char('\n');
	stream << QLatin1String("Description: ") << m_description << QLatin1Char('\n');
	stream << QLatin1String("Type: mouse-profile\n");
	stream << QLatin1String("Author: ") << m_author << QLatin1Char('\n');
	stream << QLatin1String("Version: ") << m_version;

	settings.setComment(comment);

	QJsonArray contextsArray;
	QHash<int, QVector<MouseProfile::Gesture> >::const_iterator contextsIterator;

	for (contextsIterator = m_definitions.begin(); contextsIterator != m_definitions.end(); ++contextsIterator)
	{
		QJsonArray gesturesArray;

		for (int i = 0; i < contextsIterator.value().count(); ++i)
		{
			const MouseProfile::Gesture &gesture(contextsIterator.value().at(i));
			QJsonArray stepsArray;

			for (int j = 0; j < gesture.steps.count(); ++j)
			{
				stepsArray.append(gesture.steps.at(j).toString());
			}

			QJsonObject gestureObject{{QLatin1String("action"), ((gesture.action == NATIVE_GESTURE) ? QLatin1String("NoAction") : ActionsManager::getActionName(gesture.action))}, {QLatin1String("steps"), stepsArray}};

			if (!gesture.parameters.isEmpty())
			{
				gestureObject.insert(QLatin1String("parameters"), QJsonObject::fromVariantMap(gesture.parameters));
			}

			gesturesArray.append(gestureObject);
		}

		contextsArray.append(QJsonObject({{QLatin1String("context"), GesturesManager::getContextName(contextsIterator.key())}, {QLatin1String("gestures"), gesturesArray}}));
	}

	settings.setArray(contextsArray);

	const bool result(settings.save());

	if (result)
	{
		m_isModified = false;
	}

	return result;
}

GesturesManager* GesturesManager::m_instance(nullptr);
MouseGestures::Recognizer* GesturesManager::m_recognizer(nullptr);
QPointer<QObject> GesturesManager::m_trackedObject(nullptr);
QPoint GesturesManager::m_lastClick;
QPoint GesturesManager::m_lastPosition;
QVariantMap GesturesManager::m_paramaters;
QHash<GesturesManager::GesturesContext, QVector<MouseProfile::Gesture> > GesturesManager::m_gestures;
QHash<GesturesManager::GesturesContext, QVector<QVector<MouseProfile::Gesture::Step> > > GesturesManager::m_nativeGestures;
QVector<QInputEvent*> GesturesManager::m_events;
QVector<MouseProfile::Gesture::Step> GesturesManager::m_steps;
QVector<GesturesManager::GesturesContext> GesturesManager::m_contexts;
int GesturesManager::m_gesturesContextEnumerator(0);
bool GesturesManager::m_isReleasing(false);
bool GesturesManager::m_afterScroll(false);

GesturesManager::GesturesManager(QObject *parent) : QObject(parent),
	m_reloadTimer(0)
{
	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &GesturesManager::handleOptionChanged);
}

void GesturesManager::createInstance()
{
	if (!m_instance)
	{
		m_nativeGestures[GesturesManager::GenericContext] = {{{QEvent::MouseButtonDblClick, Qt::LeftButton}}, {{QEvent::MouseButtonPress, Qt::LeftButton}, {QEvent::MouseButtonRelease, Qt::LeftButton}}, {{QEvent::MouseButtonPress, Qt::LeftButton}, {QEvent::MouseMove, MouseGestures::UnknownMouseAction}}};
		m_nativeGestures[GesturesManager::LinkContext] = {{{QEvent::MouseButtonPress, Qt::LeftButton}, {QEvent::MouseButtonRelease, Qt::LeftButton}}, {{QEvent::MouseButtonPress, Qt::LeftButton}, {QEvent::MouseMove, MouseGestures::UnknownMouseAction}}};
		m_nativeGestures[GesturesManager::ContentEditableContext] = {{{QEvent::MouseButtonPress, Qt::MiddleButton}}};
		m_nativeGestures[GesturesManager::TabHandleContext] = {{{QEvent::MouseButtonPress, Qt::LeftButton}, {QEvent::MouseMove, MouseGestures::UnknownMouseAction}}};

		m_instance = new GesturesManager(QCoreApplication::instance());
		m_gesturesContextEnumerator = GesturesManager::staticMetaObject.indexOfEnumerator(QLatin1String("GesturesContext").data());

		loadProfiles();
	}
}

void GesturesManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_reloadTimer)
	{
		killTimer(m_reloadTimer);

		m_reloadTimer = 0;

		loadProfiles();
	}
}

void GesturesManager::loadProfiles()
{
	m_gestures.clear();

	MouseProfile::Gesture contextMenuGestureDefinition;
	contextMenuGestureDefinition.steps = {MouseProfile::Gesture::Step(QEvent::MouseButtonPress, Qt::RightButton), MouseProfile::Gesture::Step(QEvent::MouseButtonRelease, Qt::RightButton)};
	contextMenuGestureDefinition.action = ActionsManager::ContextMenuAction;

	for (int i = (UnknownContext + 1); i < OtherContext; ++i)
	{
		m_gestures[static_cast<GesturesContext>(i)] = {contextMenuGestureDefinition};
	}

	const QStringList gestureProfiles(SettingsManager::getOption(SettingsManager::Browser_MouseProfilesOrderOption).toStringList());
	const bool areMouseGesturesEnabled(SettingsManager::getOption(SettingsManager::Browser_EnableMouseGesturesOption).toBool());

	for (int i = 0; i < gestureProfiles.count(); ++i)
	{
		const MouseProfile profile(gestureProfiles.at(i));
		const QHash<int, QVector<MouseProfile::Gesture> > contexts(profile.getDefinitions());
		QHash<int, QVector<MouseProfile::Gesture> >::const_iterator iterator;

		for (iterator = contexts.begin(); iterator != contexts.end(); ++iterator)
		{
			const QVector<MouseProfile::Gesture> &gestures(iterator.value());

			for (int j = 0; j < gestures.count(); ++j)
			{
				bool isAllowed(true);

				if (!areMouseGesturesEnabled)
				{
					for (int k = 0; k < gestures.at(j).steps.count(); ++k)
					{
						if (gestures.at(j).steps.at(k).type == QEvent::MouseMove)
						{
							isAllowed = false;

							break;
						}
					}
				}

				if (isAllowed)
				{
					m_gestures[static_cast<GesturesContext>(iterator.key())].append(gestures.at(j));
				}
			}
		}
	}
}

void GesturesManager::recognizeMoveStep(const QInputEvent *event)
{
	if (!m_recognizer)
	{
		return;
	}

	QHash<int, MouseGestures::ActionList> possibleMoves;

	for (int i = 0; i < m_contexts.count(); ++i)
	{
		const QVector<MouseProfile::Gesture> gestures(m_gestures[m_contexts.at(i)]);

		for (int j = 0; j < gestures.count(); ++j)
		{
			const QVector<MouseProfile::Gesture::Step> steps(gestures.at(j).steps);

			if (steps.count() > m_steps.count() && steps[m_steps.count()].type == QEvent::MouseMove && steps.mid(0, m_steps.count()) == m_steps)
			{
				MouseGestures::ActionList moves;

				for (int k = m_steps.count(); (k < steps.count() && steps.at(k).type == QEvent::MouseMove); ++k)
				{
					moves.push_back(steps.at(k).direction);
				}

				if (!moves.empty())
				{
					possibleMoves.insert(m_recognizer->registerGesture(moves), moves);
				}
			}
		}
	}

	const QMouseEvent *mouseEvent(static_cast<const QMouseEvent*>(event));

	if (mouseEvent)
	{
		m_recognizer->addPosition(mouseEvent->pos().x(), mouseEvent->pos().y());
	}

	const int gesture(m_recognizer->endGesture());
	const MouseGestures::ActionList moves(possibleMoves.value(gesture));
	MouseGestures::ActionList::const_iterator iterator;

	for (iterator = moves.begin(); iterator != moves.end(); ++iterator)
	{
		m_steps.append(MouseProfile::Gesture::Step(QEvent::MouseMove, *iterator, event->modifiers()));
	}

	if (m_steps.empty() && calculateLastMoveDistance(true) >= QApplication::startDragDistance())
	{
		m_steps.append(MouseProfile::Gesture::Step(QEvent::MouseMove, MouseGestures::UnknownMouseAction, event->modifiers()));
	}
}

void GesturesManager::cancelGesture()
{
	releaseObject();

	m_steps.clear();

	qDeleteAll(m_events);

	m_events.clear();
}

void GesturesManager::releaseObject()
{
	if (m_trackedObject)
	{
		m_trackedObject->removeEventFilter(m_instance);

		disconnect(m_trackedObject, &QObject::destroyed, m_instance, &GesturesManager::endGesture);
	}

	m_trackedObject = nullptr;
}

void GesturesManager::endGesture()
{
	cancelGesture();
}

void GesturesManager::handleOptionChanged(int identifier)
{
	switch (identifier)
	{
		case SettingsManager::Browser_EnableMouseGesturesOption:
		case SettingsManager::Browser_MouseProfilesOrderOption:
			if (m_reloadTimer == 0)
			{
				m_reloadTimer = startTimer(250);
			}

			break;
		default:
			break;
	}
}

GesturesManager* GesturesManager::getInstance()
{
	return m_instance;
}

QObject* GesturesManager::getTrackedObject()
{
	return m_trackedObject;
}

QString GesturesManager::getContextName(int identifier)
{
	QString name(GesturesManager::staticMetaObject.enumerator(m_gesturesContextEnumerator).valueToKey(identifier));

	if (!name.isEmpty())
	{
		name.chop(7);

		return name;
	}

	return {};
}

MouseProfile::Gesture GesturesManager::matchGesture()
{
	MouseProfile::Gesture bestGesture;
	bestGesture.action = UNKNOWN_GESTURE;

	int lowestDifference(std::numeric_limits<int>::max());

	for (int i = 0; i < m_contexts.count(); ++i)
	{
		const QVector<QVector<MouseProfile::Gesture::Step> > nativeGestures(m_nativeGestures[m_contexts.at(i)]);

		for (int j = 0; j < nativeGestures.count(); ++j)
		{
			const int difference(calculateGesturesDifference(nativeGestures.at(j)));

			if (difference == 0)
			{
				MouseProfile::Gesture gesture;
				gesture.action = NATIVE_GESTURE;

				return gesture;
			}

			if (difference < lowestDifference)
			{
				bestGesture = {};
				bestGesture.action = NATIVE_GESTURE;

				lowestDifference = difference;
			}
		}

		const QVector<MouseProfile::Gesture> gestures(m_gestures[m_contexts.at(i)]);

		for (int j = 0; j < gestures.count(); ++j)
		{
			const int difference(calculateGesturesDifference(gestures.at(j).steps));

			if (difference == 0)
			{
				return gestures.at(j);
			}

			if (difference < lowestDifference)
			{
				bestGesture = gestures.at(j);
				lowestDifference = difference;
			}
		}
	}

	return bestGesture;
}

int GesturesManager::getContextIdentifier(const QString &name)
{
	if (!name.endsWith(QLatin1String("Context")))
	{
		return GesturesManager::staticMetaObject.enumerator(m_gesturesContextEnumerator).keyToValue((name + QLatin1String("Context")).toLatin1());
	}

	return GesturesManager::staticMetaObject.enumerator(m_gesturesContextEnumerator).keyToValue(name.toLatin1());
}

int GesturesManager::calculateLastMoveDistance(bool measureFinished)
{
	int result(0);
	int index(m_events.count() - 1);

	if (!measureFinished && (index < 0 || m_events.at(index)->type() != QEvent::MouseMove))
	{
		return result;
	}

	while (index >= 0 && m_events.at(index)->type() != QEvent::MouseMove)
	{
		--index;
	}

	for (; index > 0 && m_events.at(index - 1)->type() == QEvent::MouseMove; --index)
	{
		const QMouseEvent *current(static_cast<QMouseEvent*>(m_events.at(index)));
		const QMouseEvent *previous(static_cast<QMouseEvent*>(m_events.at(index - 1)));

		if (!current || !previous)
		{
			break;
		}

		result += (previous->pos() - current->pos()).manhattanLength();
	}

	return result;
}

int GesturesManager::calculateGesturesDifference(const QVector<MouseProfile::Gesture::Step> &steps)
{
	if (m_steps.count() != steps.count())
	{
		return std::numeric_limits<int>::max();
	}

	int difference(0);

	for (int i = 0; i < steps.count(); ++i)
	{
		const MouseProfile::Gesture::Step matchedStep(steps.at(i));
		const MouseProfile::Gesture::Step recordedStep(m_steps.at(i));
		int stepDifference(0);

		if (i == (steps.count() - 1) && matchedStep.type == QEvent::MouseButtonPress && recordedStep.type == QEvent::MouseButtonDblClick && matchedStep.button == recordedStep.button && matchedStep.modifiers == recordedStep.modifiers)
		{
			stepDifference += 100;
		}

		if (recordedStep.type == matchedStep.type && (matchedStep.type == QEvent::MouseButtonPress || matchedStep.type == QEvent::MouseButtonRelease || matchedStep.type == QEvent::MouseButtonDblClick) && recordedStep.button == matchedStep.button && (recordedStep.modifiers | matchedStep.modifiers) == recordedStep.modifiers)
		{
			if (recordedStep.modifiers.testFlag(Qt::ControlModifier) && !matchedStep.modifiers.testFlag(Qt::ControlModifier))
			{
				stepDifference += 8;
			}

			if (recordedStep.modifiers.testFlag(Qt::ShiftModifier) && !matchedStep.modifiers.testFlag(Qt::ShiftModifier))
			{
				stepDifference += 4;
			}

			if (recordedStep.modifiers.testFlag(Qt::AltModifier) && !matchedStep.modifiers.testFlag(Qt::AltModifier))
			{
				stepDifference += 2;
			}

			if (recordedStep.modifiers.testFlag(Qt::MetaModifier) && !matchedStep.modifiers.testFlag(Qt::MetaModifier))
			{
				stepDifference += 1;
			}
		}

		if (stepDifference == 0 && matchedStep != recordedStep)
		{
			return std::numeric_limits<int>::max();
		}

		difference += stepDifference;
	}

	return difference;
}

bool GesturesManager::startGesture(QObject *object, QEvent *event, QVector<GesturesContext> contexts, const QVariantMap &parameters)
{
	QInputEvent *inputEvent(static_cast<QInputEvent*>(event));

	if (!object || !inputEvent || m_gestures.keys().toSet().intersect(contexts.toList().toSet()).isEmpty() || m_events.contains(inputEvent))
	{
		return false;
	}

	m_paramaters = parameters;

	if (!m_trackedObject)
	{
		m_contexts = contexts;
		m_isReleasing = false;
		m_afterScroll = false;
	}

	createInstance();
	releaseObject();

	m_trackedObject = object;

	if (m_trackedObject)
	{
		m_trackedObject->installEventFilter(m_instance);

		connect(m_trackedObject, &QObject::destroyed, m_instance, &GesturesManager::endGesture);
	}

	return (m_trackedObject && m_instance->eventFilter(m_trackedObject, event));
}

bool GesturesManager::continueGesture(QObject *object)
{
	if (!m_trackedObject)
	{
		return false;
	}

	if (!object)
	{
		cancelGesture();

		return false;
	}

	releaseObject();

	m_trackedObject = object;
	m_trackedObject->installEventFilter(m_instance);

	connect(m_trackedObject, &QObject::destroyed, m_instance, &GesturesManager::endGesture);

	return true;
}

bool GesturesManager::triggerAction(const MouseProfile::Gesture &gesture)
{
	if (gesture.action == UNKNOWN_GESTURE)
	{
		return false;
	}

	m_trackedObject->removeEventFilter(m_instance);

	if (gesture.action == NATIVE_GESTURE)
	{
		for (int i = 0; i < m_events.count(); ++i)
		{
			QCoreApplication::sendEvent(m_trackedObject, m_events.at(i));
		}

		cancelGesture();
	}
	else if (gesture.action == ActionsManager::ContextMenuAction)
	{
		if (m_trackedObject)
		{
			QContextMenuEvent event(QContextMenuEvent::Other, m_lastPosition);

			QCoreApplication::sendEvent(m_trackedObject, &event);
		}
	}
	else
	{
		QVariantMap parameters(gesture.parameters);
		parameters.unite(m_paramaters);

		Application::triggerAction(gesture.action, parameters, m_trackedObject, ActionsManager::MouseTrigger);
	}

	if (m_trackedObject)
	{
		m_trackedObject->installEventFilter(m_instance);
	}

	return true;
}

bool GesturesManager::isTracking()
{
	return (m_trackedObject != nullptr);
}

bool GesturesManager::eventFilter(QObject *object, QEvent *event)
{
	QMouseEvent *mouseEvent(nullptr);
	MouseProfile::Gesture gesture;

	switch (event->type())
	{
		case QEvent::MouseButtonPress:
		case QEvent::MouseButtonRelease:
		case QEvent::MouseButtonDblClick:
			mouseEvent = static_cast<QMouseEvent*>(event);

			if (!mouseEvent)
			{
				break;
			}

			if (!m_events.isEmpty() && m_events.last()->type() == event->type())
			{
				const QMouseEvent *previousMouseEvent(static_cast<QMouseEvent*>(m_events.last()));

				if (previousMouseEvent && previousMouseEvent->button() == mouseEvent->button() && previousMouseEvent->modifiers() == mouseEvent->modifiers())
				{
					break;
				}
			}

			m_events.append(new QMouseEvent(event->type(), mouseEvent->localPos(), mouseEvent->windowPos(), mouseEvent->screenPos(), mouseEvent->button(), mouseEvent->buttons(), mouseEvent->modifiers()));

			if (m_afterScroll && event->type() == QEvent::MouseButtonRelease)
			{
				break;
			}

			m_lastPosition = mouseEvent->pos();
			m_lastClick = mouseEvent->pos();

			recognizeMoveStep(mouseEvent);

			m_steps.append(MouseProfile::Gesture::Step(mouseEvent));

			if (m_isReleasing && event->type() == QEvent::MouseButtonRelease)
			{
				for (int i = (m_steps.count() - 1); i >= 0; --i)
				{
					if (m_steps.at(i).button == mouseEvent->button())
					{
						m_steps.removeAt(i);
					}
				}
			}
			else
			{
				m_isReleasing = false;
			}

			if (m_recognizer)
			{
				delete m_recognizer;

				m_recognizer = nullptr;
			}

			gesture = matchGesture();

			if (triggerAction(gesture))
			{
				m_isReleasing = true;
			}

			m_afterScroll = false;

			break;
		case QEvent::MouseMove:
			mouseEvent = static_cast<QMouseEvent*>(event);

			if (!mouseEvent)
			{
				break;
			}

			m_events.append(new QMouseEvent(event->type(), mouseEvent->localPos(), mouseEvent->windowPos(), mouseEvent->screenPos(), mouseEvent->button(), mouseEvent->buttons(), mouseEvent->modifiers()));

			m_afterScroll = false;

			if (!m_recognizer)
			{
				m_recognizer = new MouseGestures::Recognizer();
				m_recognizer->startGesture(m_lastClick.x(), m_lastClick.y());
			}

			m_lastPosition = mouseEvent->pos();

			m_recognizer->addPosition(mouseEvent->pos().x(), mouseEvent->pos().y());

			if (calculateLastMoveDistance() >= QApplication::startDragDistance())
			{
				m_steps.append(MouseProfile::Gesture::Step(QEvent::MouseMove, MouseGestures::UnknownMouseAction, mouseEvent->modifiers()));

				gesture = matchGesture();

				if (gesture.action != UNKNOWN_GESTURE)
				{
					recognizeMoveStep(mouseEvent);

					triggerAction(gesture);
				}
				else
				{
					m_steps.pop_back();
				}
			}

			break;
		case QEvent::Wheel:
			{
				const QWheelEvent *wheelEvent(static_cast<QWheelEvent*>(event));

				if (!wheelEvent)
				{
					break;
				}

				m_events.append(new QWheelEvent(wheelEvent->pos(), wheelEvent->globalPos(), wheelEvent->pixelDelta(), wheelEvent->angleDelta(), wheelEvent->delta(), wheelEvent->orientation(), wheelEvent->buttons(), wheelEvent->modifiers()));

				recognizeMoveStep(wheelEvent);

				m_steps.append(MouseProfile::Gesture::Step(wheelEvent));

				m_lastClick = wheelEvent->pos();

				if (m_recognizer)
				{
					delete m_recognizer;

					m_recognizer = nullptr;
				}

				gesture = matchGesture();

				triggerAction(gesture);

				while (!m_steps.isEmpty() && m_steps.at(m_steps.count() - 1).type == QEvent::Wheel)
				{
					m_steps.removeAt(m_steps.count() - 1);
				}

				while (!m_events.isEmpty() && m_events.at(m_events.count() - 1)->type() == QEvent::Wheel)
				{
					m_events.removeAt(m_events.count() - 1);
				}

				m_afterScroll = true;

				break;
			}
		default:
			return QObject::eventFilter(object, event);
	}

	if (m_trackedObject && mouseEvent && mouseEvent->buttons() == Qt::NoButton)
	{
		cancelGesture();
	}

	return (!m_steps.isEmpty() || gesture.action != UNKNOWN_GESTURE);
}

}
