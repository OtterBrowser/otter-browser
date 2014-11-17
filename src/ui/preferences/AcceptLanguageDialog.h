/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_ACCEPTLANGUAGEDIALOG_H
#define OTTER_ACCEPTLANGUAGEDIALOG_H

#include <QtWidgets/QDialog>
#include <QtGui/QStandardItemModel>

namespace Otter
{

namespace Ui
{
	class AcceptLanguageDialog;
}

class AcceptLanguageDialog : public QDialog
{
	Q_OBJECT

public:
	explicit AcceptLanguageDialog(QWidget *parent = 0);
	~AcceptLanguageDialog();

	QString getLanguageList();
	bool eventFilter(QObject *object, QEvent *event);

protected:
	void chooseLanguage(const QString &languageCode);
	static bool languageOrder(const QPair<QString, QString> &first, const QPair<QString, QString> &second);

protected slots:
	void languageFromComboBox();

private:
	QStandardItemModel *m_model;
	Ui::AcceptLanguageDialog *m_ui;
};


}
#endif
