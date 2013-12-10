#include "ProgressBarDelegate.h"

#include <QtWidgets/QApplication>

namespace Otter
{

ProgressBarDelegate::ProgressBarDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

void ProgressBarDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QStyleOptionProgressBar progressBarOption;
	progressBarOption.fontMetrics = option.fontMetrics;
	progressBarOption.palette = option.palette;
	progressBarOption.rect = option.rect;
	progressBarOption.state = option.state;
	progressBarOption.minimum = 0;
	progressBarOption.maximum = 100;
	progressBarOption.textAlignment = Qt::AlignCenter;
	progressBarOption.textVisible = true;
	progressBarOption.progress = index.data(Qt::DisplayRole).toInt();
	progressBarOption.text = QString("%1%").arg(progressBarOption.progress);

	QApplication::style()->drawControl(QStyle::CE_ProgressBar, &progressBarOption, painter, 0);
}

}
