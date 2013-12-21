#ifndef OTTER_SEARCHPROPERTIESDIALOG_H
#define OTTER_SEARCHPROPERTIESDIALOG_H

#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>

namespace Otter
{

namespace Ui
{
	class SearchPropertiesDialog;
}

class SearchPropertiesDialog : public QDialog
{
	Q_OBJECT

public:
	explicit SearchPropertiesDialog(const QVariantHash &engineData, const QStringList &shortcuts, QWidget *parent = NULL);
	~SearchPropertiesDialog();

	QVariantHash getEngineData() const;
	bool eventFilter(QObject *object, QEvent *event);

protected:
	void changeEvent(QEvent *event);

protected slots:
	void insertPlaceholder(QAction *action);
	void selectIcon();

private:
	QLineEdit *m_currentLineEdit;
	QVariantHash m_engineData;
	Ui::SearchPropertiesDialog *m_ui;
};

}

#endif
