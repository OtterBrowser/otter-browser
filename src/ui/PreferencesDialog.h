#ifndef OTTER_PREFERENCESDIALOG_H
#define OTTER_PREFERENCESDIALOG_H

#include <QDialog>

namespace Otter
{

namespace Ui
{
	class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
	Q_OBJECT

public:
	explicit PreferencesDialog(const QString &section, QWidget *parent = NULL);
	~PreferencesDialog();

protected:
	void changeEvent(QEvent *event);

protected slots:
	void browseDownloadsPath();
	void filterSearch(const QString &filter);
	void save();

private:
	Ui::PreferencesDialog *m_ui;
};

}

#endif
