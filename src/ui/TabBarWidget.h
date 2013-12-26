#ifndef OTTER_TABBARWIDGET_H
#define OTTER_TABBARWIDGET_H

#include <QtWidgets/QTabBar>
#include <QtWidgets/QToolButton>

namespace Otter
{

class PreviewWidget;

class TabBarWidget : public QTabBar
{
	Q_OBJECT

public:
	explicit TabBarWidget(QWidget *parent = NULL);

	QVariant getTabProperty(int index, const QString &key, const QVariant &defaultValue) const;

public slots:
	void updateTabs(int index = -1);
	void setOrientation(Qt::DockWidgetArea orientation);
	void setShape(QTabBar::Shape shape);
	void setTabProperty(int index, const QString &key, const QVariant &value);

protected:
	void timerEvent(QTimerEvent *event);
	void contextMenuEvent(QContextMenuEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void enterEvent(QEvent *event);
	void leaveEvent(QEvent *event);
	void resizeEvent(QResizeEvent *event);
	void tabInserted(int index);
	void tabRemoved(int index);
	void tabLayoutChange();
	void showPreview(int index);
	QSize tabSizeHint(int index) const;
	QSize getTabSize(bool isHorizontal) const;

protected slots:
	void closeOther();
	void cloneTab();
	void detachTab();
	void pinTab();

private:
	PreviewWidget *m_previewWidget;
	QToolButton *m_newTabButton;
	QSize m_tabSize;
	int m_clickedTab;
	int m_previewTimer;

signals:
	void requestedClone(int index);
	void requestedDetach(int index);
	void requestedPin(int index, bool pin);
	void requestedClose(int index);
	void requestedCloseOther(int index);
	void showNewTabButton(bool visible);
};

}

#endif
