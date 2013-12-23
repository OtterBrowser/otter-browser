#ifndef OTTER_CONFIGURATIONCONTENTSWIDGET_H
#define OTTER_CONFIGURATIONCONTENTSWIDGET_H

#include "../../../ui/ContentsWidget.h"

#include <QtGui/QStandardItemModel>

namespace Otter
{

namespace Ui
{
	class ConfigurationContentsWidget;
}

class Window;

class ConfigurationContentsWidget : public ContentsWidget
{
	Q_OBJECT

public:
	explicit ConfigurationContentsWidget(Window *window);
	~ConfigurationContentsWidget();

	void print(QPrinter *printer);
	QString getTitle() const;
	QString getType() const;
	QUrl getUrl() const;
	QIcon getIcon() const;

protected:
	void changeEvent(QEvent *event);

protected slots:
	void filterConfiguration(const QString &filter);
	void currentChanged(const QModelIndex &currentIndex, const QModelIndex &previousIndex);
	void optionChanged(const QString &option, const QVariant &value);

private:
	QStandardItemModel *m_model;
	QHash<WindowAction, QAction*> m_actions;
	Ui::ConfigurationContentsWidget *m_ui;
};

}

#endif
