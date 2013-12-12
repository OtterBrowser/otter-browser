#ifndef OTTER_QTWEBKITWEBBACKEND_H
#define OTTER_QTWEBKITWEBBACKEND_H

#include "../../../../core/WebBackend.h"

namespace Otter
{

class QtWebKitWebBackend : public WebBackend
{
	Q_OBJECT

public:
	explicit QtWebKitWebBackend(QObject *parent = NULL);

	WebWidget* createWidget(bool privateWindow = false, ContentsWidget *parent = NULL);
	QString getTitle() const;
	QString getDescription() const;
	QIcon getIconForUrl(const QUrl &url);
};

}

#endif
