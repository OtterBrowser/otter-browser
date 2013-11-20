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
	static void registerAction(QWidget *window, const QString &name, const QString &text, const QIcon &icon = QIcon());
	static void registerAction(QWidget *window, QAction *action, QString name = QString());
	static void registerActions(QWidget *window, QList<QAction*> getIdentifiers);
	static void triggerAction(const QString &action);
	static void setupLocalAction(QAction *localAction, const QString &globalAction, bool connectTrigger = false);
	static void restoreDefaultShortcut(QAction *action);
	static void restoreDefaultShortcut(const QString &action);
	static void setShortcut(QAction *action, const QKeySequence &shortcut);
	static void setShortcut(const QString &action, const QKeySequence &shortcut);
	static void setActiveWindow(QWidget *window);
	static QAction* getAction(const QString &action);
	static QKeySequence getShortcut(QAction *action);
	static QKeySequence getShortcut(const QString &action);
	static QKeySequence getDefaultShortcut(QAction *action);
	static QKeySequence getDefaultShortcut(const QString &action);
	static QStringList getIdentifiers();
	static bool hasShortcut(const QKeySequence &shortcut, const QString &excludeAction = QString());

private:
	explicit ActionsManager(QObject *parent = NULL);

	void addWindow(QWidget *window);
	void addAction(QWidget *window, QAction *action, QString name = QString());

	static ActionsManager *m_instance;
	static QWidget *m_activeWindow;
	static QHash<QWidget*, QHash<QString, QAction*> > m_actions;

private slots:
	void removeWindow(QObject *window);
};

}

#endif
