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

#include "FilePathWidget.h"
#include "LineEditWidget.h"
#include "../core/Utils.h"

#include <QtCore/QEvent>
#include <QtCore/QStandardPaths>
#include <QtWidgets/QHBoxLayout>

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

FilePathWidget::FilePathWidget(QWidget *parent) : QWidget(parent),
	m_browseButton(new QPushButton(tr("Browse…"), this)),
	m_lineEditWidget(new LineEditWidget(this)),
	m_completer(nullptr),
	m_selectFile(true)
{
	QHBoxLayout *layout(new QHBoxLayout(this));
	layout->addWidget(m_lineEditWidget);
	layout->addWidget(m_browseButton);
	layout->setContentsMargins(0, 0, 0, 0);

	setLayout(layout);
	setFocusPolicy(Qt::StrongFocus);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	connect(m_lineEditWidget, &LineEditWidget::textEdited, this, &FilePathWidget::updateCompleter);
	connect(m_lineEditWidget, &LineEditWidget::textChanged, this, &FilePathWidget::pathChanged);
	connect(m_browseButton, &QPushButton::clicked, this, &FilePathWidget::selectPath);
}

void FilePathWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_browseButton->setText(tr("Browse…"));
	}
}

void FilePathWidget::focusInEvent(QFocusEvent *event)
{
	QWidget::focusInEvent(event);

	m_lineEditWidget->setFocus();
}

void FilePathWidget::selectPath()
{
	QString path(m_lineEditWidget->text().isEmpty() ? QStandardPaths::standardLocations(QStandardPaths::HomeLocation).value(0) : m_lineEditWidget->text());
	path = (m_selectFile ? QFileDialog::getOpenFileName(this, tr("Select File"), path, m_filter) : QFileDialog::getExistingDirectory(this, tr("Select Directory"), path));

	if (!path.isEmpty())
	{
		m_lineEditWidget->setText(QDir::toNativeSeparators(path));
	}
}

void FilePathWidget::updateCompleter()
{
	if (!m_completer)
	{
		m_completer = new QCompleter(this);

		FileSystemCompleterModel *model(new FileSystemCompleterModel(m_completer));
		model->setFilter(m_selectFile ? (QDir::AllDirs | QDir::Files) : QDir::AllDirs);

		m_completer->setModel(model);

		m_lineEditWidget->setCompleter(m_completer);

		m_completer->complete();

		disconnect(m_lineEditWidget, &LineEditWidget::textEdited, this, &FilePathWidget::updateCompleter);
	}
}

void FilePathWidget::setFilters(const QStringList &filters)
{
	m_filter = Utils::formatFileTypes(filters);
}

void FilePathWidget::setSelectFile(bool mode)
{
	m_selectFile = mode;
}

void FilePathWidget::setPath(const QString &path)
{
	m_lineEditWidget->setText(path);
}

QString FilePathWidget::getPath() const
{
	return m_lineEditWidget->text();
}

}
