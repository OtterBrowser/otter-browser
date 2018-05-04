/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_LINEEDITWIDGET_H
#define OTTER_LINEEDITWIDGET_H

#include "ItemViewWidget.h"
#include "../core/ActionExecutor.h"

#include <QtWidgets/QCompleter>
#include <QtWidgets/QLineEdit>

namespace Otter
{

class LineEditWidget;

class PopupViewWidget final : public ItemViewWidget
{
	Q_OBJECT

public:
	explicit PopupViewWidget(LineEditWidget *parent);

	bool event(QEvent *event) override;

public slots:
	void updateHeight();

protected:
	void keyPressEvent(QKeyEvent *event) override;

protected slots:
	void handleIndexEntered(const QModelIndex &index);

private:
	LineEditWidget *m_lineEditWidget;
};

class LineEditWidget : public QLineEdit, public ActionExecutor
{
	Q_OBJECT

public:
	enum DropMode
	{
		PasteDropMode = 0,
		ReplaceDropMode,
		ReplaceAndNotifyDropMode
	};

	explicit LineEditWidget(const QString &text, QWidget *parent = nullptr);
	explicit LineEditWidget(QWidget *parent = nullptr);
	~LineEditWidget();

	void activate(Qt::FocusReason reason);
	virtual void showPopup();
	virtual void hidePopup();
	void setDropMode(DropMode mode);
	void setClearOnEscape(bool clear);
	void setSelectAllOnFocus(bool select);
	PopupViewWidget* getPopup();
	ActionsManager::ActionDefinition::State getActionState(int identifier, const QVariantMap &parameters) const override;
	bool isPopupVisible() const;

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger) override;
	void setCompletion(const QString &completion);

protected:
	void resizeEvent(QResizeEvent *event) override;
	void focusInEvent(QFocusEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void dropEvent(QDropEvent *event) override;
	void initialize();

protected slots:
	void clearSelectAllOnRelease();
	void handleSelectionChanged();
	void handleTextChanged(const QString &text);
	void notifyPasteActionStateChanged();

private:
	PopupViewWidget *m_popupViewWidget;
	QCompleter *m_completer;
	QString m_completion;
	DropMode m_dropMode;
	int m_selectionStart;
	bool m_shouldClearOnEscape;
	bool m_shouldIgnoreCompletion;
	bool m_shouldSelectAllOnFocus;
	bool m_shouldSelectAllOnRelease;
	bool m_hadSelection;
	bool m_wasEmpty;

signals:
	void arbitraryActionsStateChanged(const QVector<int> &identifiers);
	void popupClicked(const QModelIndex &index);
	void textDropped(const QString &text);
};

}

#endif
