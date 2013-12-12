#ifndef OTTER_WEBBACKEND_H
#define OTTER_WEBBACKEND_H

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtGui/QIcon>

namespace Otter
{

class ContentsWidget;
class WebWidget;

class WebBackend : public QObject
{
	Q_OBJECT

public:
	explicit WebBackend(QObject *parent = NULL);

	virtual WebWidget* createWidget(bool privateWindow = false, ContentsWidget *parent = NULL) = 0;
	virtual QString getTitle() const = 0;
	virtual QString getDescription() const = 0;
	virtual QIcon getIconForUrl(const QUrl &url) = 0;
};

}

#endif
