#ifndef OTTER_WEBBACKENDWEBKIT_H
#define OTTER_WEBBACKENDWEBKIT_H

#include "../WebBackend.h"

namespace Otter
{

class WebBackendWebKit : public WebBackend
{
	Q_OBJECT

public:
	explicit WebBackendWebKit(QObject *parent = NULL);

	WebWidget* createWidget(QWidget *parent = NULL);
	QString getTitle() const;
	QString getDescription() const;
	QIcon getIconForUrl(const QUrl &url);
};

}

#endif
