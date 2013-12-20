#include "ShortcutDelegate.h"

#include <QtGui/QRegularExpressionValidator>
#include <QtWidgets/QLineEdit>

namespace Otter
{

ShortcutDelegate::ShortcutDelegate(QObject *parent) : QItemDelegate(parent)
{
}

void ShortcutDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)
	Q_UNUSED(index)

	editor->setGeometry(option.rect);
}

void ShortcutDelegate::setEditorData(QWidget *editor, const QModelIndex &index)
{
	Q_UNUSED(editor)
	Q_UNUSED(index)
}

void ShortcutDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	QLineEdit *widget = qobject_cast<QLineEdit*>(editor);

	if (widget)
	{
		model->setData(index, widget->text());
	}
}

QWidget* ShortcutDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	QStringList shortcuts;

	for (int i = 0; i < index.model()->rowCount(); ++i)
	{
		const QString shortcut = index.model()->index(i, 1).data(Qt::DisplayRole).toString();

		if (index.row() != i && !shortcut.isEmpty())
		{
			shortcuts.append(shortcut);
		}
	}

	QLineEdit *widget = new QLineEdit(index.data(Qt::DisplayRole).toString(), parent);
	widget->setValidator(new QRegularExpressionValidator(QRegularExpression((shortcuts.isEmpty() ? QString() : QString("(?!\\b(%1)\\b)").arg(shortcuts.join('|'))) + "[a-z0-9]*"), widget));

	return widget;
}

}
