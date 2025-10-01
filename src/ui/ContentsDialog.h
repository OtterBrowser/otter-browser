/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_CONTENTSDIALOG_H
#define OTTER_CONTENTSDIALOG_H

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QScrollArea>

namespace Otter
{

class ContentsDialog : public QFrame
{
	Q_OBJECT

public:
	explicit ContentsDialog(const QIcon &icon, const QString &title, const QString &text, const QString &details, QDialogButtonBox::StandardButtons buttons, QWidget *payload, QWidget *parent);

	void updateSize();
	void setCheckBox(const QString &text, bool state);
	bool getCheckBoxState() const;
	bool isAccepted() const;
	bool eventFilter(QObject *object, QEvent *event) override;

protected:
	void showEvent(QShowEvent *event) override;
	void closeEvent(QCloseEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;

private:
	QBoxLayout *m_contentsLayout;
	QWidget *m_headerWidget;
	QWidget *m_payloadWidget;
	QLabel *m_closeLabel;
	QLabel *m_detailsLabel;
	QScrollArea *m_scrollArea;
	QCheckBox *m_checkBox;
	QDialogButtonBox *m_buttonBox;
	QPoint m_offset;
	bool m_isAccepted;

signals:
	void accepted(bool isChecked);
	void rejected(bool isChecked);
	void finished(int result, bool isChecked);
};

}

#endif
