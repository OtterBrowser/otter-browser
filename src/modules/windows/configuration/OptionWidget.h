#ifndef OTTER_OPTIONWIDGET_H
#define OTTER_OPTIONWIDGET_H

#include <QtWidgets/QComboBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>

namespace Otter
{

class OptionWidget : public QWidget
{
	Q_OBJECT

public:
	explicit OptionWidget(const QString &option, const QString &type, const QStringList &choices, QWidget *parent = NULL);

protected slots:
	void modified();
	void reset();
	void save();

private:
	QString m_option;
	QPushButton *m_resetButton;
	QPushButton *m_saveButton;
	QComboBox *m_comboBox;
	QLineEdit *m_lineEdit;
	QSpinBox *m_spinBox;
};

}

#endif
