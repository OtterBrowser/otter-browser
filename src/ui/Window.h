#ifndef OTTER_WINDOW_H
#define OTTER_WINDOW_H

#include <QtPrintSupport/QPrinter>
#include <QtWidgets/QWidget>
#include <QtWidgets/QUndoStack>

namespace Otter
{

namespace Ui
{
	class Window;
}

class Window : public QWidget
{
	Q_OBJECT

public:
	explicit Window(QWidget *parent = NULL);
	~Window();

	virtual void print(QPrinter *printer);
	virtual QWidget* getDocument();
	virtual QUndoStack* getUndoStack();
	virtual QString getTitle() const;
	virtual QUrl getUrl() const;
	virtual QIcon getIcon() const;
	virtual int getZoom() const;
	virtual bool isEmpty() const;

public slots:
	virtual void reload();
	virtual void stop();
	virtual void goBack();
	virtual void goForward();
	virtual void undo();
	virtual void redo();
	virtual void cut();
	virtual void copy();
	virtual void paste();
	virtual void remove();
	virtual void selectAll();
	virtual void zoomIn();
	virtual void zoomOut();
	virtual void zoomOriginal();
	virtual void setZoom(int zoom);
	virtual void setUrl(const QUrl &url);

protected:
	void changeEvent(QEvent *event);

protected slots:
	void loadUrl();
	void notifyTitleChanged();
	void notifyUrlChanged(const QUrl &url);
	void notifyIconChanged();

private:
	Ui::Window *m_ui;

signals:
	void titleChanged(const QString &title);
	void urlChanged(const QUrl &url);
	void iconChanged(const QIcon &icon);
	void undoTextChanged(const QString &undoText);
	void redoTextChanged(const QString &redoText);
	void canUndoChanged(bool canUndo);
	void canRedoChanged(bool canRedo);
};

}

#endif
