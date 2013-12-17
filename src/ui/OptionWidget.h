#ifndef OTTER_OPTIONWIDGET_H
#define OTTER_OPTIONWIDGET_H

#include <QtWidgets/QComboBox>
#include <QtWidgets/QFontComboBox>
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
	void selectColor();
	void modified();
	void reset();
	void save();

private:
	QString m_option;
	QPushButton *m_resetButton;
	QPushButton *m_saveButton;
	QPushButton *m_colorButton;
	QComboBox *m_comboBox;
	QFontComboBox *m_fontComboBox;
	QLineEdit *m_lineEdit;
	QSpinBox *m_spinBox;
};

}

#endif
