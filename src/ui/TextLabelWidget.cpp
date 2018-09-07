/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "TextLabelWidget.h"
#include "../core/ThemesManager.h"

#include <QtGui/QClipboard>
#include <QtGui/QDesktopServices>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOptionFrame>

namespace Otter
{

TextLabelWidget::TextLabelWidget(QWidget *parent) : QLineEdit(parent),
	m_isEmpty(true)
{
	updateStyle();
	setFrame(false);
	setReadOnly(true);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	setStyleSheet(QLatin1String("QLineEdit {background:transparent;}"));
	QLineEdit::setText(tr("<empty>"));
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
	if (m_isEmpty)
	{
		return;
	}

	QMenu menu(this);
	QAction *copyAction(menu.addAction(ThemesManager::createIcon(QLatin1String("edit-copy")), tr("Copy"), this, &TextLabelWidget::copy));
	copyAction->setEnabled(hasSelectedText());
	copyAction->setShortcut(QKeySequence(QKeySequence::Copy));

	if (m_url.isValid())
	{
		menu.addAction(tr("Copy Link Location"), this, &TextLabelWidget::copyUrl);
	}

	menu.addSeparator();

	QAction *selectAllAction(menu.addAction(ThemesManager::createIcon(QLatin1String("edit-select-all")), tr("Select All"), this, &TextLabelWidget::selectAll));
	selectAllAction->setEnabled(!text().isEmpty());
	selectAllAction->setShortcut(QKeySequence(QKeySequence::SelectAll));

	menu.exec(event->globalPos());
}

void TextLabelWidget::copyUrl()
{
	QGuiApplication::clipboard()->setText(m_url.toString(QUrl::EncodeReserved | QUrl::EncodeSpaces));
}

void TextLabelWidget::clear()
{
	QLineEdit::setText(tr("<empty>"));

	m_url.clear();
	m_isEmpty = true;

	updateStyle();
}

void TextLabelWidget::updateStyle()
{
	QFont font(this->font());
	font.setUnderline(m_url.isValid() && style()->styleHint(QStyle::SH_UnderlineShortcut) > 0);

	QPalette palette(this->palette());

	if (m_isEmpty)
	{
		palette.setColor(QPalette::Text, QGuiApplication::palette().color(QPalette::Disabled, QPalette::WindowText));
	}
	else
	{
		palette.setColor(QPalette::Text, QGuiApplication::palette().color(m_url.isValid() ? QPalette::Link : QPalette::WindowText));
	}

	setCursor(m_url.isValid() ? Qt::PointingHandCursor : Qt::ArrowCursor);
	setFont(font);
	setPalette(palette);
}

void TextLabelWidget::setText(const QString &text)
{
	if (text != this->text())
	{
		if (text.isEmpty() != m_isEmpty)
		{
			m_isEmpty = text.isEmpty();

			updateStyle();
		}

		QLineEdit::setText(m_isEmpty ? tr("<empty>") : text);
		setCursorPosition(0);
		updateGeometry();
	}

	setReadOnly(true);
}

void TextLabelWidget::setUrl(const QUrl &url)
{
	m_url = url;
	m_isEmpty = url.isEmpty();

	updateStyle();
}

QSize TextLabelWidget::sizeHint() const
{
	const QMargins margins(textMargins());
	QSize size(fontMetrics().size(Qt::TextSingleLine, text()));
	size.setWidth(size.width() + margins.left() + margins.right() + 6);
	size.setHeight(size.height() + margins.top() + margins.bottom());

	QStyleOptionFrame option;

	initStyleOption(&option);

	return style()->sizeFromContents(QStyle::CT_LineEdit, &option, size, this);
}

bool TextLabelWidget::event(QEvent *event)
{
	const bool result(QLineEdit::event(event));

	switch (event->type())
	{
		case QEvent::ApplicationPaletteChange:
		case QEvent::StyleChange:
			updateStyle();

			break;
		case QEvent::LanguageChange:
			if (m_isEmpty)
			{
				QLineEdit::setText(tr("<empty>"));
			}

			break;
		default:
			break;
	}

	return result;
}

}
