#ifndef OTTER_SHORTCUTDELEGATE_H
#define OTTER_SHORTCUTDELEGATE_H

#include <QtCore/QObject>
#include <QtWidgets/QItemDelegate>

namespace Otter
{

class ShortcutDelegate : public QItemDelegate
{
public:
	explicit ShortcutDelegate(QObject *parent);

	void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	void setEditorData(QWidget *editor, const QModelIndex &index);
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

}

#endif
