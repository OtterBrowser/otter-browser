/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_SESSIONSMANAGER_H
#define OTTER_SESSIONSMANAGER_H

#include "SettingsManager.h"
#include "ToolBarsManager.h"
#include "Utils.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QRect>

namespace Otter
{

class MainWindow;
class SessionModel;

class Session final : public QObject
{
public:
	struct Identity final
	{
		QString name;
		QString title;
		QString description;
		QColor color;
		QIcon icon;
		bool isEphemeral = true;

		bool isValid() const
		{
			return !name.isEmpty();
		}
	};

	struct Window final
	{
		struct History final
		{
			struct Entry final
			{
				QString url;
				QString title;
				QIcon icon;
				QDateTime time;
				QPoint position;
				int zoom = SettingsManager::getOption(SettingsManager::Content_DefaultZoomOption).toInt();

				QString getTitle() const
				{
					if (title.isEmpty())
					{
						if (url == QLatin1String("about:start") && SettingsManager::getOption(SettingsManager::StartPage_EnableStartPageOption).toBool())
						{
							return QCoreApplication::translate("Otter::SessionsManager", "Start Page");
						}

						return QCoreApplication::translate("Otter::SessionsManager", "(Unknown)");
					}

					return title;
				}
			};

			QVector<Entry> entries;
			int index = -1;

			bool isEmpty() const
			{
				return (entries.isEmpty() || (entries.count() == 1 && Utils::isUrlEmpty(QUrl(entries.value(0).url))));
			}

			bool isValid() const
			{
				return (index >= 0);
			}
		};

		struct State final
		{
			QRect geometry;
			Qt::WindowState state = ((SettingsManager::getOption(SettingsManager::Interface_NewTabOpeningActionOption).toString() == QLatin1String("maximizeTab")) ? Qt::WindowMaximized : Qt::WindowNoState);
		};

		QString identity;
		History history;
		State state;
		QHash<int, QVariant> options;
		int parentGroup = 0;
		bool isAlwaysOnTop = false;
		bool isPinned = false;

		QString getUrl() const
		{
			if (history.index >= 0 && history.index < history.entries.count())
			{
				return history.entries.at(history.index).url;
			}

			return {};
		}

		QString getTitle() const
		{
			if (history.index >= 0 && history.index < history.entries.count())
			{
				return history.entries.at(history.index).getTitle();
			}

			return QCoreApplication::translate("main", "(Untitled)");
		}

		int getZoom() const
		{
			if (history.index >= 0 && history.index < history.entries.count())
			{
				return history.entries.at(history.index).zoom;
			}

			return SettingsManager::getOption(SettingsManager::Content_DefaultZoomOption).toInt();
		}
	};

	struct MainWindow final
	{
		struct ToolBarState final
		{
			enum ToolBarVisibility
			{
				UnspecifiedVisibilityToolBar = 0,
				AlwaysVisibleToolBar,
				AlwaysHiddenToolBar
			};

			Qt::ToolBarArea location = Qt::NoToolBarArea;
			int identifier = -1;
			int row = -1;
			ToolBarVisibility normalVisibility = UnspecifiedVisibilityToolBar;
			ToolBarVisibility fullScreenVisibility = UnspecifiedVisibilityToolBar;

			explicit ToolBarState() = default;

			explicit ToolBarState(int identifierValue, const ToolBarsManager::ToolBarDefinition &definition) : identifier(identifierValue)
			{
				normalVisibility = ((definition.normalVisibility != ToolBarsManager::AlwaysHiddenToolBar) ? AlwaysVisibleToolBar : AlwaysHiddenToolBar);
				fullScreenVisibility = ((definition.fullScreenVisibility != ToolBarsManager::AlwaysHiddenToolBar) ? AlwaysVisibleToolBar : AlwaysHiddenToolBar);
			}

			void setVisibility(ToolBarsManager::ToolBarsMode mode, ToolBarVisibility visibility)
			{
				switch (mode)
				{
					case ToolBarsManager::NormalMode:
						normalVisibility = visibility;

						break;
					case ToolBarsManager::FullScreenMode:
						fullScreenVisibility = visibility;

						break;
				}
			}

			ToolBarVisibility getVisibility(ToolBarsManager::ToolBarsMode mode) const
			{
				return ((mode == ToolBarsManager::FullScreenMode) ? fullScreenVisibility : normalVisibility);
			}

			bool isValid() const
			{
				return (identifier >= 0);
			}
		};

		QMap<QString, QVector<int> > splitters;
		QVector<Window> windows;
		QVector<ToolBarState> toolBars;
		QByteArray geometry;
		int index = -1;
		bool hasToolBarsState = false;
	};

	struct ClosedWindow final
	{
		Window window;
		QIcon icon;
		quint64 nextWindow = 0;
		quint64 previousWindow = 0;
		bool isPrivate = false;
	};

	explicit Session(QObject *parent = nullptr);
};

struct SessionInformation final
{
	QString path;
	QString title;
	QVector<Session::MainWindow> windows;
	int index = -1;
	bool isClean = true;

	bool isValid() const
	{
		return !windows.isEmpty();
	}
};

class SessionsManager final : public QObject
{
	Q_OBJECT

public:
	enum OpenHint
	{
		DefaultOpen = 0,
		PrivateOpen = 1,
		CurrentTabOpen = 2,
		NewTabOpen = 4,
		NewWindowOpen = 8,
		BackgroundOpen = 16,
		EndOpen = 32
	};

	Q_ENUM(OpenHint)
	Q_DECLARE_FLAGS(OpenHints, OpenHint)

	static void createInstance(const QString &profilePath, const QString &cachePath, bool isPrivate = false, bool isReadOnly = false);
	static void clearClosedWindows();
	static void storeClosedWindow(MainWindow *mainWindow);
	static void markSessionAsModified();
	static void removeStoredUrl(const QString &url);
	static SessionsManager* getInstance();
	static SessionModel* getModel();
	static QString getCurrentSession();
	static QString getCachePath();
	static QString getProfilePath();
	static QString getReadableDataPath(const QString &path, bool forceBundled = false);
	static QString getWritableDataPath(const QString &path);
	static QString getSessionPath(const QString &path, bool isBound = false);
	static Session::Identity getIdentity(const QString &name);
	static SessionInformation getSession(const QString &path);
	static QStringList getClosedWindows();
	static QStringList getSessions();
	static QVector<Session::Identity> getIdentities();
	static OpenHints calculateOpenHints(OpenHints hints, Qt::MouseButton button, Qt::KeyboardModifiers modifiers);
	static OpenHints calculateOpenHints(OpenHints hints = DefaultOpen, Qt::MouseButton button = Qt::LeftButton);
	static OpenHints calculateOpenHints(const QVariantMap &parameters, bool ignoreModifiers = false);
	static bool restoreClosedWindow(int index = 0);
	static bool restoreSession(const SessionInformation &session, MainWindow *mainWindow = nullptr, bool isPrivate = false);
	static bool saveSession(const QString &path = {}, const QString &title = {}, MainWindow *mainWindow = nullptr, bool isClean = true);
	static bool saveSession(const SessionInformation &session);
	static bool deleteSession(const QString &path = {});
	static bool isPrivate();
	static bool isReadOnly();
	static bool hasUrl(const QUrl &url, bool activate = false);

protected:
	explicit SessionsManager(QObject *parent);

	void timerEvent(QTimerEvent *event) override;
	void scheduleSave();

private:
	int m_saveTimer;

	static SessionsManager *m_instance;
	static SessionModel *m_model;
	static QString m_sessionPath;
	static QString m_sessionTitle;
	static QString m_cachePath;
	static QString m_profilePath;
	static QHash<QString, Session::Identity> m_identities;
	static QVector<Session::MainWindow> m_closedWindows;
	static int m_openHintEnumerator;
	static bool m_isDirty;
	static bool m_isPrivate;
	static bool m_isReadOnly;

signals:
	void closedWindowsChanged();
	void requestedRemoveStoredUrl(const QString &url);
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Otter::SessionsManager::OpenHints)

#endif
