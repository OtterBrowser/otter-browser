#ifndef OTTER_ACTIONSMANAGER_H
#define OTTER_ACTIONSMANAGER_H

#include <QtCore/QObject>
#include <QtWidgets/QAction>

namespace Otter
{

class ActionsManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static void registerWindow(QWidget *window);
	static void registerAction(QWidget *window, const QLatin1String &name, const QString &text, const QIcon &icon = QIcon());
	static void registerAction(QWidget *window, QAction *action);
	static void registerActions(QWidget *window, QList<QAction*> actions);
	static void triggerAction(const QLatin1String &action);
	static void setupLocalAction(QAction *localAction, const QLatin1String &globalAction, bool connectTrigger = false);
	static void restoreDefaultShortcut(const QLatin1String &action);
	static void setShortcut(const QLatin1String &action, const QKeySequence &shortcut);
	static QAction* getAction(const QLatin1String &action);
	static QKeySequence getShortcut(const QLatin1String &action);
	static QKeySequence getDefaultShortcut(const QLatin1String &action);
	static QStringList getIdentifiers();
	static bool hasShortcut(const QKeySequence &shortcut, const QLatin1String &excludeAction);

protected slots:
	void removeWindow(QObject *window);

private:
	explicit ActionsManager(QObject *parent = NULL);

	void addWindow(QWidget *window);

	static ActionsManager *m_instance;
	static QHash<QWidget*, QHash<QString, QAction*> > m_actions;
};

}

#endif
