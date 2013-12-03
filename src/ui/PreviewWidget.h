#ifndef OTTER_PREVIEWWIDGET_H
#define OTTER_PREVIEWWIDGET_H

#include <QtWidgets/QLabel>

namespace Otter
{

class PreviewWidget : public QWidget
{
	Q_OBJECT

public:
	explicit PreviewWidget(QWidget *parent = 0);

public slots:
	void setPreview(const QString &text, const QPixmap &pixmap = QPixmap());

private:
	QLabel *m_textLabel;
	QLabel *m_pixmapLabel;
};

}

#endif
