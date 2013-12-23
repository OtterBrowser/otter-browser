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
	void moveSearch(bool up);

protected slots:
	void browseDownloadsPath();
	void currentFontChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
	void fontChanged(QWidget *editor);
	void currentColorChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
	void colorChanged(QWidget *editor);
	void setupClearHistory();
	void filterSearch(const QString &filter);
	void currentSearchChanged(int currentRow);
	void addSearch();
	void editSearch();
	void removeSearch();
	void moveDownSearch();
	void moveUpSearch();
	void save();

private:
	QString m_defaultSearch;
	QStringList m_clearSettings;
	Ui::PreferencesDialog *m_ui;
};

}

#endif
