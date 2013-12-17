#ifndef OTTER_OPTIONDELEGATE_H
#define OTTER_OPTIONDELEGATE_H

#include <QtWidgets/QStyledItemDelegate>

namespace Otter
{

class OptionDelegate : public QStyledItemDelegate
{
public:
	explicit OptionDelegate(QObject *parent);

	void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	void setEditorData(QWidget *editor, const QModelIndex &index);
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

}

#endif
