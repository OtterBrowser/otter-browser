#include "OptionDelegate.h"
#include "OptionWidget.h"

namespace Otter
{

OptionDelegate::OptionDelegate(bool simple, QObject *parent) : QItemDelegate(parent),
	m_simple(simple)
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
	if (m_simple)
	{
		OptionWidget *widget = qobject_cast<OptionWidget*>(editor);

		if (widget)
		{
			model->setData(index, widget->getValue());
		}
	}
}

QWidget* OptionDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	OptionWidget *widget = new OptionWidget(m_simple, index.data(Qt::UserRole).toString(), index.data(Qt::UserRole + 1).toString(), index.data(Qt::EditRole), QStringList(), index, parent);

	connect(widget, SIGNAL(commitData(QWidget*)), this, SIGNAL(commitData(QWidget*)));

	return widget;
}

}
