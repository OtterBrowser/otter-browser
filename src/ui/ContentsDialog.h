#ifndef OTTER_CONTENTSDIALOG_H
#define OTTER_CONTENTSDIALOG_H

#include <QtWidgets/QLabel>

namespace Otter
{

class ContentsDialog : public QWidget
{
	Q_OBJECT

public:
	explicit ContentsDialog(QWidget *payload, QWidget *parent = NULL);

	bool eventFilter(QObject *object, QEvent *event);

private:
	QWidget *m_payload;
	QLabel *m_titleLabel;
	QPoint m_offset;
};

}

#endif
