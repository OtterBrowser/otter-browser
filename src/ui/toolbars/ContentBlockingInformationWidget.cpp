/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ContentBlockingInformationWidget.h"
#include "../ContentsWidget.h"
#include "../MainWindow.h"
#include "../ToolBarWidget.h"
#include "../Window.h"
#include "../../core/ContentBlockingManager.h"
#include "../../core/ContentBlockingProfile.h"
#include "../../core/ThemesManager.h"
#include "../../core/Utils.h"

#include <QtWidgets/QStyleOptionToolButton>
#include <QtWidgets/QStylePainter>

namespace Otter
{

ContentBlockingInformationWidget::ContentBlockingInformationWidget(Window *window, const ActionsManager::ActionEntryDefinition &definition, QWidget *parent) : ToolButtonWidget(definition, parent),
	m_window(window),
	m_elementsMenu(NULL),
	m_profilesMenu(NULL),
	m_amount(0)
{
	QMenu *menu(new QMenu(this));

	m_elementsMenu = menu->addMenu(tr("Blocked Elements"));
	m_profilesMenu = menu->addMenu(tr("Active Profiles"));

	setMenu(menu);
	setPopupMode(QToolButton::InstantPopup);
	setIcon(ThemesManager::getIcon(QLatin1String("content-blocking")));
	setWindow(window);

	ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent));

	if (toolBar && toolBar->getIdentifier() != ToolBarsManager::NavigationBar)
	{
		connect(toolBar, SIGNAL(windowChanged(Window*)), this, SLOT(setWindow(Window*)));
	}

	connect(m_elementsMenu, SIGNAL(aboutToShow()), this, SLOT(populateElementsMenu()));
	connect(m_elementsMenu, SIGNAL(triggered(QAction*)), this, SLOT(openElement(QAction*)));
	connect(m_profilesMenu, SIGNAL(aboutToShow()), this, SLOT(populateProfilesMenu()));
	connect(m_profilesMenu, SIGNAL(triggered(QAction*)), this, SLOT(toggleOption(QAction*)));
}

void ContentBlockingInformationWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QStylePainter painter(this);
	QStyleOptionToolButton option;

	initStyleOption(&option);

	if (m_icon.isNull())
	{
		m_icon = getOptions().value(QLatin1String("icon")).value<QIcon>();

		if (m_icon.isNull())
		{
			m_icon = ThemesManager::getIcon(QLatin1String("content-blocking"));
		}

		const int iconSize(option.iconSize.width());
		const int fontSize(qMax((iconSize / 2), 12));
		QFont font(option.font);
		font.setBold(true);
		font.setPixelSize(fontSize);

		QString text;

		if (m_amount > 999999)
		{
			text = QString::number(m_amount / 1000000) + QLatin1Char('M');
		}
		else if (m_amount > 999)
		{
			text = QString::number(m_amount / 1000) + QLatin1Char('K');
		}
		else
		{
			text = QString::number(m_amount);
		}

		const qreal textWidth(QFontMetricsF(font).width(text));

		font.setPixelSize(fontSize * 0.8);

		QRectF rectangle((iconSize - textWidth), (iconSize - fontSize), textWidth, fontSize);
		QPixmap pixmap(m_icon.pixmap(option.iconSize));
		QPainter iconPainter(&pixmap);
		iconPainter.fillRect(rectangle, Qt::red);
		iconPainter.setFont(font);
		iconPainter.setPen(QColor(255, 255, 255, 230));
		iconPainter.drawText(rectangle, Qt::AlignCenter, text);

		m_icon = QIcon(pixmap);
	}

	option.icon = m_icon;
	option.text = option.fontMetrics.elidedText(option.text, Qt::ElideRight, (option.rect.width() - (option.fontMetrics.width(QLatin1Char(' ')) * 2) - ((toolButtonStyle() == Qt::ToolButtonTextBesideIcon) ? option.iconSize.width() : 0)));

	painter.drawComplexControl(QStyle::CC_ToolButton, option);
}

void ContentBlockingInformationWidget::resizeEvent(QResizeEvent *event)
{
	m_icon = QIcon();

	ToolButtonWidget::resizeEvent(event);
}

void ContentBlockingInformationWidget::clear()
{
	m_amount = 0;

	updateState();
}

void ContentBlockingInformationWidget::openElement(QAction *action)
{
	MainWindow *mainWindow(MainWindow::findMainWindow(parent()));

	if (action && mainWindow)
	{
		mainWindow->getWindowsManager()->open(QUrl(action->statusTip()), WindowsManager::calculateOpenHints());
	}
}

void ContentBlockingInformationWidget::toggleOption(QAction *action)
{
	if (action && m_window)
	{
		if (action->data().isNull())
		{
			m_window->getContentsWidget()->setOption(SettingsManager::ContentBlocking_EnableContentBlockingOption, action->isChecked());
		}
		else
		{
			const QString profile(action->data().toString());
			QStringList profiles(m_window->getContentsWidget()->getOption(SettingsManager::Content_BlockingProfilesOption).toStringList());

			if (!action->isChecked())
			{
				profiles.removeAll(profile);
			}
			else if (!profiles.contains(profile))
			{
				profiles.append(profile);
			}

			m_window->getContentsWidget()->setOption(SettingsManager::Content_BlockingProfilesOption, profiles);
		}
	}
}

void ContentBlockingInformationWidget::populateElementsMenu()
{
	m_elementsMenu->clear();

	if (!m_window)
	{
		return;
	}

	const QList<NetworkManager::ResourceInformation> requests(m_window->getContentsWidget()->getBlockedRequests().mid(m_amount - 50));

	for (int i = 0; i < requests.count(); ++i)
	{
		QString type;

		switch (requests.at(i).resourceType)
		{
			case NetworkManager::MainFrameType:
				type = tr("main frame");

				break;
			case NetworkManager::SubFrameType:
				type = tr("subframe");

				break;
			case NetworkManager::StyleSheetType:
				type = tr("stylesheet");

				break;
			case NetworkManager::ScriptType:
				type = tr("script");

				break;
			case NetworkManager::ImageType:
				type = tr("image");

				break;
			case NetworkManager::ObjectType:
				type = tr("object");

				break;
			case NetworkManager::ObjectSubrequestType:
				type = tr("object subrequest");

				break;
			case NetworkManager::XmlHttpRequestType:
				type = tr("XHR");

				break;
			default:
				type = tr("other");

				break;
		}

		QAction *action(m_elementsMenu->addAction(QStringLiteral("%1\t [%2]").arg(Utils::elideText(requests.at(i).url.toString(), m_elementsMenu)).arg(type)));
		action->setStatusTip(requests.at(i).url.toString());
	}
}

void ContentBlockingInformationWidget::populateProfilesMenu()
{
	m_profilesMenu->clear();

	if (!m_window)
	{
		return;
	}

	QAction *enableContentBlockingAction(m_profilesMenu->addAction(tr("Enable Content Blocking")));
	enableContentBlockingAction->setCheckable(true);
	enableContentBlockingAction->setChecked(m_window->getContentsWidget()->getOption(SettingsManager::ContentBlocking_EnableContentBlockingOption).toBool());

	m_profilesMenu->addSeparator();

	const QList<NetworkManager::ResourceInformation> requests(m_window->getContentsWidget()->getBlockedRequests());
	QHash<QString, int> amounts;

	for (int i = 0; i < requests.count(); ++i)
	{
		const QString profile(requests.at(i).metaData.value(NetworkManager::ContentBlockingProfileMetaData).toString());

		if (amounts.contains(profile))
		{
			++amounts[profile];
		}
		else
		{
			amounts[profile] = 1;
		}
	}

	const QVector<ContentBlockingProfile*> profiles(ContentBlockingManager::getProfiles());
	const QStringList enabledProfiles(m_window->getContentsWidget()->getOption(SettingsManager::Content_BlockingProfilesOption).toStringList());

	for (int i = 0; i < profiles.count(); ++i)
	{
		if (profiles.at(i))
		{
			const int amount(amounts.value(profiles.at(i)->getName()));
			const QString title(Utils::elideText(profiles.at(i)->getTitle(), m_profilesMenu));
			QAction *profileAction(m_profilesMenu->addAction((amount > 0) ? QStringLiteral("%1 (%2)").arg(title).arg(amount) : title));
			profileAction->setData(profiles.at(i)->getName());
			profileAction->setCheckable(true);
			profileAction->setChecked(enabledProfiles.contains(profiles.at(i)->getName()));
		}
	}
}

void ContentBlockingInformationWidget::handleRequest(const NetworkManager::ResourceInformation &request)
{
	Q_UNUSED(request)

	++m_amount;

	updateState();
}

void ContentBlockingInformationWidget::updateState()
{
	m_icon = QIcon();

	QString text(tr("Blocked Elements: %1"));

	if (isCustomized())
	{
		const QVariantMap options(getOptions());

		if (options.contains(QLatin1String("text")))
		{
			text = options[QLatin1String("text")].toString();
		}
	}

	text = text.arg(m_amount);

	setText(text);
	setToolTip(text);

	m_elementsMenu->setEnabled(m_amount > 0);
}

void ContentBlockingInformationWidget::setWindow(Window *window)
{
	if (m_window)
	{
		disconnect(m_window->getContentsWidget(), SIGNAL(aboutToNavigate()), this, SLOT(clear()));
		disconnect(m_window->getContentsWidget(), SIGNAL(requestBlocked(NetworkManager::ResourceInformation)), this, SLOT(handleRequest(NetworkManager::ResourceInformation)));
	}

	m_window = window;
	m_amount = 0;

	if (window)
	{
		m_amount = window->getContentsWidget()->getBlockedRequests().count();

		connect(m_window->getContentsWidget(), SIGNAL(aboutToNavigate()), this, SLOT(clear()));
		connect(m_window->getContentsWidget(), SIGNAL(requestBlocked(NetworkManager::ResourceInformation)), this, SLOT(handleRequest(NetworkManager::ResourceInformation)));
	}

	updateState();
	setEnabled(m_window);
}

}
