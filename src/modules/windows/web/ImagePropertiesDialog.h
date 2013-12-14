#ifndef OTTER_IMAGEPROPERTIESDIALOG_H
#define OTTER_IMAGEPROPERTIESDIALOG_H

#include <QtCore/QUrl>
#include <QtWidgets/QDialog>

namespace Otter
{

namespace Ui
{
	class ImagePropertiesDialog;
}

class ImagePropertiesDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ImagePropertiesDialog(const QUrl &url, const QString &alternativeText, const QString &longDescription, const QPixmap &pixmap, QIODevice *device, QWidget *parent = NULL);
	~ImagePropertiesDialog();

protected:
	void changeEvent(QEvent *event);

private:
	Ui::ImagePropertiesDialog *m_ui;
};

}

#endif

