/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "IconWidget.h"
#include "../core/ThemesManager.h"
#include "../core/Utils.h"

#include <QtCore/QBuffer>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMenu>

namespace Otter
{

IconWidget::IconWidget(QWidget *parent) : QToolButton(parent)
{
	setMenu(new QMenu(this));
	setToolTip(tr("Select Icon"));
	setPopupMode(QToolButton::InstantPopup);
	setMinimumSize(16, 16);
	setMaximumSize(64, 64);

	connect(menu(), SIGNAL(aboutToShow()), this, SLOT(updateMenu()));
}

void IconWidget::resizeEvent(QResizeEvent *event)
{
	QToolButton::resizeEvent(event);

	const int iconSize(qMin(height(), width()) * 0.9);

	setIconSize(QSize(iconSize, iconSize));
}

QIcon IconWidget::createIcon(const QString &data) const
{
	if (data.startsWith(QLatin1String("data:image/")))
	{
		return QIcon(QPixmap::fromImage(QImage::fromData(QByteArray::fromBase64(data.mid(data.indexOf(QLatin1String("base64,")) + 7).toUtf8()))));
	}

	return ThemesManager::createIcon(data);
}

void IconWidget::clear()
{
	m_icon = QString();

	QToolButton::setIcon(QIcon());

	emit iconChanged(QIcon());
}

void IconWidget::reset()
{
	setIcon(m_defaultIcon);
}

void IconWidget::selectFromFile()
{
	const QString path(QFileDialog::getOpenFileName(this, tr("Select Icon"), QString(), Utils::formatFileTypes(QStringList(tr("Images (*.png *.jpg *.bmp *.gif *.ico)")))));

	if (path.isEmpty())
	{
		return;
	}

	QByteArray data;
	QBuffer buffer(&data);
	buffer.open(QIODevice::WriteOnly);

	QPixmap pixmap(path);
	pixmap.save(&buffer, "PNG");

	m_icon = QStringLiteral("data:image/png;base64,%1").arg(QString(data.toBase64()));

	QIcon icon(pixmap);

	QToolButton::setIcon(icon);

	emit iconChanged(icon);
}

void IconWidget::selectFromTheme()
{
	const QString name(QInputDialog::getText(this, tr("Select Icon"), tr("Icon Name:"), QLineEdit::Normal, (m_icon.startsWith(QLatin1String("data:")) ? QString() : m_icon)));

	if (!name.isEmpty())
	{
		setIcon(name);
	}
}

void IconWidget::updateMenu()
{
	menu()->clear();
	menu()->addAction(tr("Select From File…"), this, SLOT(selectFromFile()));
	menu()->addAction(tr("Select From Theme…"), this, SLOT(selectFromTheme()));

	if (!m_defaultIcon.isEmpty())
	{
		menu()->addSeparator();
		menu()->addAction(tr("Reset"), this, SLOT(reset()))->setEnabled(m_icon != m_defaultIcon);
	}

	menu()->addSeparator();
	menu()->addAction(ThemesManager::createIcon(QLatin1String("edit-clear")), tr("Clear"), this, SLOT(clear()))->setEnabled(!icon().isNull());
}

void IconWidget::setIcon(const QString &data)
{
	m_icon = data;

	if (data.isEmpty())
	{
		clear();

		return;
	}

	const QIcon icon(createIcon(data));

	QToolButton::setIcon(icon);

	emit iconChanged(icon);
}

void IconWidget::setIcon(const QIcon &icon)
{
	if (icon.isNull())
	{
		clear();

		return;
	}

	m_icon = Utils::savePixmapAsDataUri(icon.pixmap(16, 16));

	QToolButton::setIcon(icon);

	emit iconChanged(icon);
}

void IconWidget::setDefaultIcon(const QString &data)
{
	m_defaultIcon = data;

	if (m_icon.isEmpty())
	{
		setIcon(m_defaultIcon);
	}
}

void IconWidget::setDefaultIcon(const QIcon &icon)
{
	setDefaultIcon(Utils::savePixmapAsDataUri(icon.pixmap(16, 16)));
}

QString IconWidget::getIcon() const
{
	return m_icon;
}

int IconWidget::heightForWidth(int width) const
{
	return width;
}

bool IconWidget::hasHeightForWidth() const
{
	return true;
}

}
