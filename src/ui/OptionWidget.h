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
	explicit OptionWidget(bool simple, const QString &option, const QString &type, const QVariant &value, const QStringList &choices, const QModelIndex &index, QWidget *parent = NULL);

	QVariant getValue() const;
	QModelIndex getIndex() const;

protected slots:
	void selectColor();
	void modified();
	void reset();
	void save();

private:
	QString m_option;
	QModelIndex m_index;
	QPushButton *m_resetButton;
	QPushButton *m_saveButton;
	QPushButton *m_colorButton;
	QComboBox *m_comboBox;
	QFontComboBox *m_fontComboBox;
	QLineEdit *m_lineEdit;
	QSpinBox *m_spinBox;

signals:
	void commitData(QWidget *editor);
};

}

#endif
