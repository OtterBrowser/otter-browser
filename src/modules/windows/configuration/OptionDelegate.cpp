#include "OptionDelegate.h"
#include "OptionWidget.h"

namespace Otter
{

OptionDelegate::OptionDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

void OptionDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)
	Q_UNUSED(index)

	editor->setGeometry(option.rect);
}

void OptionDelegate::setEditorData(QWidget *editor, const QModelIndex &index)
{
	Q_UNUSED(editor)
	Q_UNUSED(index)
}

void OptionDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	Q_UNUSED(editor)
	Q_UNUSED(model)
	Q_UNUSED(index)
}

QWidget* OptionDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	return new OptionWidget(QString("%1/%2").arg(index.parent().data(Qt::DisplayRole).toString()).arg(index.parent().child(index.row(), 0).data(Qt::DisplayRole).toString()), index.parent().child(index.row(), 1).data(Qt::DisplayRole).toString(), QStringList(), parent);

}

}
