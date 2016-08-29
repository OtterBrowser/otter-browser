/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Piotr Wójcik <chocimier@tlen.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "FilePathWidget.h"
#include "../core/FileSystemCompleterModel.h"
#include "../core/Utils.h"

#include <QtCore/QStandardPaths>
#include <QtWidgets/QCompleter>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>

namespace Otter
{

FilePathWidget::FilePathWidget(QWidget *parent) : QWidget(parent),
	m_lineEdit(new QLineEdit(this)),
	m_completer(NULL),
	m_selectFile(true)
{
	QPushButton *button(new QPushButton(tr("Browse…"), this));
	QHBoxLayout *layout(new QHBoxLayout(this));
	layout->addWidget(m_lineEdit);
	layout->addWidget(button);
	layout->setContentsMargins(0, 0, 0, 0);

	setLayout(layout);
	setFocusPolicy(Qt::StrongFocus);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	connect(m_lineEdit, SIGNAL(textEdited(QString)), this, SLOT(updateCompleter()));
	connect(m_lineEdit, SIGNAL(textChanged(QString)), this, SIGNAL(pathChanged(QString)));
	connect(button, SIGNAL(clicked()), this, SLOT(selectPath()));
}

void FilePathWidget::focusInEvent(QFocusEvent *event)
{
	QWidget::focusInEvent(event);

	m_lineEdit->setFocus();
}

void FilePathWidget::selectPath()
{
	QString path(m_lineEdit->text().isEmpty() ? QStandardPaths::standardLocations(QStandardPaths::HomeLocation).value(0) : m_lineEdit->text());
	path = (m_selectFile ? QFileDialog::getOpenFileName(this, tr("Select File"), path, m_filter) : QFileDialog::getExistingDirectory(this, tr("Select Directory"), path));

	if (!path.isEmpty())
	{
		m_lineEdit->setText(QDir::toNativeSeparators(path));
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

		m_lineEdit->setCompleter(m_completer);

		m_completer->complete();

		disconnect(m_lineEdit, SIGNAL(textEdited(QString)), this, SLOT(updateCompleter()));
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
	m_lineEdit->setText(path);
}

QString FilePathWidget::getPath() const
{
	return m_lineEdit->text();
}

}
