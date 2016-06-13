/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "TextLabelWidget.h"

#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QDesktopServices>
#include <QtGui/QGuiApplication>
#include <QtGui/QMouseEvent>
#include <QtGui/QStyle>
#include <QtWidgets/QMenu>

namespace Otter
{

TextLabelWidget::TextLabelWidget(QWidget *parent) : QLineEdit(parent)
{
	updateStyle();
	setFrame(false);
	setReadOnly(true);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	setStyleSheet(QLatin1String("QLineEdit {background:transparent;}"));
}

void TextLabelWidget::mousePressEvent(QMouseEvent *event)
{
	QLineEdit::mousePressEvent(event);

	m_dragStartPosition = event->pos();
}

void TextLabelWidget::mouseReleaseEvent(QMouseEvent *event)
{
	QLineEdit::mouseReleaseEvent(event);

	if (m_url.isValid() && (event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance())
	{
		QDesktopServices::openUrl(m_url);
	}
}

void TextLabelWidget::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu menu(this);
	menu.addAction(tr("Copy"), this, SLOT(copy()), QKeySequence(QKeySequence::Copy))->setEnabled(hasSelectedText());

	if (m_url.isValid())
	{
		menu.addAction(tr("Copy Link Location"), this, SLOT(copyUrl()));
	}

	menu.addSeparator();
	menu.addAction(tr("Select All"), this, SLOT(selectAll()), QKeySequence(QKeySequence::SelectAll))->setEnabled(!text().isEmpty());
	menu.exec(event->globalPos());
}

void TextLabelWidget::copyUrl()
{
	QGuiApplication::clipboard()->setText(m_url.toString(QUrl::EncodeReserved | QUrl::EncodeSpaces));
}

void TextLabelWidget::clear()
{
	QLineEdit::clear();

	m_url = QUrl();

	updateStyle();
}

void TextLabelWidget::updateStyle()
{
	QFont font(this->font());
	font.setUnderline(m_url.isValid() && style()->styleHint(QStyle::SH_UnderlineShortcut) > 0);

	QPalette palette(this->palette());
	palette.setColor(QPalette::Text, QGuiApplication::palette().color(m_url.isValid() ? QPalette::Link : QPalette::WindowText));

	setCursor(m_url.isValid() ? Qt::PointingHandCursor : Qt::ArrowCursor);
	setFont(font);
	setPalette(palette);
}

void TextLabelWidget::setText(const QString &text)
{
	if (text != this->text())
	{
		QLineEdit::setText(text);
		setCursorPosition(0);
	}

	setReadOnly(true);
}

void TextLabelWidget::setUrl(const QUrl &url)
{
	m_url = url;

	updateStyle();
}

bool TextLabelWidget::event(QEvent *event)
{
	const bool result(QLineEdit::event(event));

	if (event->type() == QEvent::ApplicationPaletteChange || event->type() == QEvent::StyleChange)
	{
		updateStyle();
	}

	return result;
}

}
