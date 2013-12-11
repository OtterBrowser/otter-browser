#ifndef OTTER_WEBBACKENDWEBKIT_H
#define OTTER_WEBBACKENDWEBKIT_H

#include "../../../../core/WebBackend.h"

namespace Otter
{

class WebBackendWebKit : public WebBackend
{
	Q_OBJECT

public:
	explicit WebBackendWebKit(QObject *parent = NULL);

	WebWidget* createWidget(bool privateWindow = false, QWidget *parent = NULL);
	QString getTitle() const;
	QString getDescription() const;
	QIcon getIconForUrl(const QUrl &url);
};

}

#endif
