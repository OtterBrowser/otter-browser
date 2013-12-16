#ifndef OTTER_FILESYSTEMCOMPLETERMODEL_H
#define OTTER_FILESYSTEMCOMPLETERMODEL_H

#include <QtWidgets/QFileSystemModel>

namespace Otter
{

class FileSystemCompleterModel : public QFileSystemModel
{

public:
	explicit FileSystemCompleterModel(QObject *parent = NULL);

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
};

}

#endif
