#ifndef OTTER_COOKIEPROPERTIESDIALOG_H
#define OTTER_COOKIEPROPERTIESDIALOG_H

#include <QtNetwork/QNetworkCookie>
#include <QtWidgets/QDialog>

namespace Otter
{

namespace Ui
{
	class CookiePropertiesDialog;
}

class CookiePropertiesDialog : public QDialog
{
	Q_OBJECT

public:
	explicit CookiePropertiesDialog(const QNetworkCookie &cookie, QWidget *parent = NULL);
	~CookiePropertiesDialog();

protected:
	void changeEvent(QEvent *event);

private:
	Ui::CookiePropertiesDialog *m_ui;
};

}

#endif
