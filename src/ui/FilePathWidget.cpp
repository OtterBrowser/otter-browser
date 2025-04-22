/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHBoxLayout>

namespace Otter
{

FileSystemCompleterModel::FileSystemCompleterModel(QObject *parent) : QFileSystemModel(parent)
{
	setFilter(QDir::NoDotAndDotDot | QDir::AllDirs);
	setRootPath({});
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
	m_openMode(ExistingFileMode),
	m_isManuallySpecified(false)
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
	connect(m_browseButton, &QPushButton::clicked, this, [&]()
	{
		const QString directory(m_lineEditWidget->text().isEmpty() ? Utils::getStandardLocation(QStandardPaths::HomeLocation) : m_lineEditWidget->text());
		QString path;

		switch (m_openMode)
		{
			case ExistingDirectoryMode:
				path = QFileDialog::getExistingDirectory(this, tr("Select Directory"), directory);

				break;
			case ExistingFileMode:
				path = QFileDialog::getOpenFileName(this, tr("Select File"), directory, m_filter);

				break;
			case NewFileMode:
				path = QFileDialog::getSaveFileName(this, tr("Select File"), directory, m_filter);

				break;
		}

		if (!path.isEmpty())
		{
			m_lineEditWidget->setText(QDir::toNativeSeparators(path));
			m_isManuallySpecified = true;
		}
	});
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

	m_initialPath = getPath();
	m_lineEditWidget->setFocus();
}

void FilePathWidget::focusOutEvent(QFocusEvent *event)
{
	QWidget::focusOutEvent(event);

	const QString path(getPath());

	if (m_initialPath != path)
	{
		emit pathChanged(path);
	}
}

void FilePathWidget::updateCompleter()
{
	if (m_completer)
	{
		return;
	}

	m_completer = new QCompleter(this);

	FileSystemCompleterModel *model(new FileSystemCompleterModel(m_completer));
	model->setFilter((m_openMode == ExistingFileMode) ? (QDir::AllDirs | QDir::Files) : QDir::AllDirs);

	m_completer->setModel(model);

	m_lineEditWidget->setCompleter(m_completer);

	m_completer->complete();

	disconnect(m_lineEditWidget, &LineEditWidget::textEdited, this, &FilePathWidget::updateCompleter);
}

void FilePathWidget::setFilters(const QStringList &filters)
{
	m_filter = Utils::formatFileTypes(filters);
}

void FilePathWidget::setOpenMode(OpenMode mode)
{
	m_openMode = mode;
}

void FilePathWidget::setPath(const QString &path)
{
	if (path == m_lineEditWidget->text())
	{
		return;
	}

	m_lineEditWidget->blockSignals(true);
	m_lineEditWidget->setText(path);
	m_lineEditWidget->blockSignals(false);

	m_isManuallySpecified = false;

	emit pathChanged(path);
}

QString FilePathWidget::getPath() const
{
	return m_lineEditWidget->text();
}

FilePathWidget::OpenMode FilePathWidget::getOpenMode() const
{
	return m_openMode;
}

bool FilePathWidget::isManuallySpecified() const
{
	return m_isManuallySpecified;
}

}
