#ifndef OTTER_AUTHENTICATIONDIALOG_H
#define OTTER_AUTHENTICATIONDIALOG_H

#include <QtCore/QUrl>
#include <QtNetwork/QAuthenticator>
#include <QtWidgets/QDialog>

namespace Otter
{

namespace Ui
{
	class AuthenticationDialog;
}

class AuthenticationDialog : public QDialog
{
	Q_OBJECT

public:
	explicit AuthenticationDialog(const QUrl &url, QAuthenticator *authenticator, QWidget *parent = NULL);
	~AuthenticationDialog();

protected:
	void changeEvent(QEvent *event);

protected slots:
	void setup();

private:
	QAuthenticator *m_authenticator;
	Ui::AuthenticationDialog *m_ui;
};

}

#endif
