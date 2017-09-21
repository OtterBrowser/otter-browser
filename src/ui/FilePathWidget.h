/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Piotr Wójcik <chocimier@tlen.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#ifndef OTTER_FILEPATHWIDGET_H
#define OTTER_FILEPATHWIDGET_H

#include <QtWidgets/QCompleter>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFileSystemModel>
#include <QtWidgets/QPushButton>

namespace Otter
{

class LineEditWidget;

class FileSystemCompleterModel final : public QFileSystemModel
{

public:
	explicit FileSystemCompleterModel(QObject *parent);

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
};

class FilePathWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit FilePathWidget(QWidget *parent = nullptr);

	void setFilters(const QStringList &filters);
	void setSelectFile(bool mode);
	void setPath(const QString &path);
	QString getPath() const;

protected:
	void changeEvent(QEvent *event) override;
	void focusInEvent(QFocusEvent *event) override;

protected slots:
	void selectPath();
	void updateCompleter();

private:
	QPushButton *m_browseButton;
	LineEditWidget *m_lineEditWidget;
	QCompleter *m_completer;
	QString m_filter;
	bool m_selectFile;

signals:
	void pathChanged(const QString &path);
};

}

#endif
