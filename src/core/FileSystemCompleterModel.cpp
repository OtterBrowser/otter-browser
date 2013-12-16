#include "FileSystemCompleterModel.h"

namespace Otter
{

FileSystemCompleterModel::FileSystemCompleterModel(QObject *parent) : QFileSystemModel(parent)
{
	setFilter(QDir::NoDotAndDotDot | QDir::AllDirs);
	setRootPath(QString());
}

QVariant FileSystemCompleterModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::DisplayRole && index.column() == 0)
	{
		return QDir::toNativeSeparators(filePath(index));
	}

	return QFileSystemModel::data(index, role);
}

}
