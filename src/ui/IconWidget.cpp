/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "IconWidget.h"
#include "../core/Utils.h"

#include <QtCore/QBuffer>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMenu>

namespace Otter
{

IconWidget::IconWidget(QWidget *parent) : QToolButton(parent)
{
	QMenu *menu = new QMenu(this);
	menu->addAction(tr("Select From File…"), this, SLOT(selectFromFile()));
	menu->addAction(tr("Select From Theme…"), this, SLOT(selectFromTheme()));
	menu->addSeparator();
	menu->addAction(Utils::getIcon(QLatin1String("edit-clear")), tr("Clear"), this, SLOT(clear()))->setEnabled(false);

	setMenu(menu);
	setToolTip(tr("Select Icon"));
	setPopupMode(QToolButton::InstantPopup);
	setMinimumSize(16, 16);
	setMaximumSize(64, 64);
}

void IconWidget::resizeEvent(QResizeEvent *event)
{
	QToolButton::resizeEvent(event);

	const int iconSize = (qMin(height(), width()) * 0.9);

	setIconSize(QSize(iconSize, iconSize));
}

void IconWidget::clear()
{
	m_icon = QString();

	QToolButton::setIcon(m_placeholderIcon);

	menu()->actions().at(3)->setEnabled(false);

	emit iconChanged(QIcon());
}

void IconWidget::selectFromFile()
{
	const QString path = QFileDialog::getOpenFileName(this, tr("Select Icon"), QString(), tr("Images (*.png *.jpg *.bmp *.gif *.ico)"));

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

	menu()->actions().at(3)->setEnabled(true);

	emit iconChanged(icon);
}

void IconWidget::selectFromTheme()
{
	const QString name = QInputDialog::getText(this, tr("Select Icon"), tr("Icon Name:"), QLineEdit::Normal, (m_icon.startsWith(QLatin1String("data:")) ? QString() : m_icon));

	if (!name.isEmpty())
	{
		setIcon(name);
	}
}

void IconWidget::setIcon(const QString &data)
{
	m_icon = data;

	if (data.isEmpty())
	{
		clear();

		return;
	}

	QIcon icon;

	if (data.startsWith(QLatin1String("data:image/")))
	{
		icon = QIcon(QPixmap::fromImage(QImage::fromData(QByteArray::fromBase64(data.mid(data.indexOf(QLatin1String("base64,")) + 7).toUtf8()))));
	}
	else
	{
		icon = Utils::getIcon(data);
	}

	QToolButton::setIcon(icon);

	menu()->actions().at(3)->setEnabled(true);

	emit iconChanged(icon);
}

void IconWidget::setIcon(const QIcon &icon)
{
	if (icon.isNull())
	{
		clear();

		return;
	}

	QByteArray data;
	QBuffer buffer(&data);
	buffer.open(QIODevice::WriteOnly);

	icon.pixmap(16, 16).save(&buffer, "PNG");

	m_icon = QStringLiteral("data:image/png;base64,%1").arg(QString(data.toBase64()));

	QToolButton::setIcon(icon);

	menu()->actions().at(3)->setEnabled(true);

	emit iconChanged(icon);
}

void IconWidget::setPlaceholderIcon(const QIcon &icon)
{
	m_placeholderIcon = icon;

	if (m_icon.isEmpty())
	{
		QToolButton::setIcon(m_placeholderIcon);
	}
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
