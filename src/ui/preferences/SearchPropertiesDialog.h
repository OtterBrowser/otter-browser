#ifndef OTTER_SEARCHPROPERTIESDIALOG_H
#define OTTER_SEARCHPROPERTIESDIALOG_H

#include <QtWidgets/QDialog>

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

protected:
	void changeEvent(QEvent *event);

private:
	QVariantHash m_engineData;
	Ui::SearchPropertiesDialog *m_ui;
};

}

#endif
