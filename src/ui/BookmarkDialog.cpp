#include "BookmarkDialog.h"

#include "ui_BookmarkDialog.h"

namespace Otter
{

BookmarkDialog::BookmarkDialog(Bookmark *bookmark, QWidget *parent) : QDialog(parent),
	m_bookmark(bookmark),
	m_ui(new Ui::BookmarkDialog)
{
	m_ui->setupUi(this);
	m_ui->titleLineEdit->setText(m_bookmark->title);
	m_ui->addressLineEdit->setText(m_bookmark->url);
	m_ui->descriptionTextEdit->setPlainText(m_bookmark->description);

	connect(m_ui->newFolderButton, SIGNAL(clicked()), this, SLOT(createFolder()));
}

BookmarkDialog::~BookmarkDialog()
{
	delete m_ui;
}

void BookmarkDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			m_ui->retranslateUi(this);

			break;
		default:
			break;
	}
}

void BookmarkDialog::createFolder()
{
//TODO
}

}
