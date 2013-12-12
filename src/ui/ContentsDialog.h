#ifndef OTTER_CONTENTSDIALOG_H
#define OTTER_CONTENTSDIALOG_H

#include <QtWidgets/QWidget>

namespace Otter
{

class ContentsDialog : public QWidget
{
	Q_OBJECT

public:
	explicit ContentsDialog(QWidget *payload, QWidget *parent = NULL);

private:
	QWidget *m_payload;
};

}

#endif
