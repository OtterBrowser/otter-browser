/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Jan Bajer aka bajasoft <jbajer@gmail.com>
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
#include "../../../core/Application.h"
#include "../../../core/ContentFiltersManager.h"
#include "../../../core/ThemesManager.h"
#include "../../../core/Utils.h"
#include "../../../ui/ContentsWidget.h"
#include "../../../ui/ToolBarWidget.h"
#include "../../../ui/Window.h"

#include <QtWidgets/QStyleOptionToolButton>
#include <QtWidgets/QStylePainter>

namespace Otter
{

ContentBlockingInformationWidget::ContentBlockingInformationWidget(Window *window, const ToolBarsManager::ToolBarDefinition::Entry &definition, QWidget *parent) : ToolButtonWidget(definition, parent),
	m_window(window),
	m_elementsMenu(nullptr),
	m_profilesMenu(nullptr),
	m_requestsAmount(0),
	m_isContentBlockingEnabled(false)
{
	QMenu *menu(new QMenu(this));

	m_profilesMenu = menu->addMenu(tr("Active Profiles"));
	m_elementsMenu = menu->addMenu(tr("Blocked Elements"));

	setMenu(menu);
	setPopupMode(QToolButton::MenuButtonPopup);
	setIcon(ThemesManager::createIcon(QLatin1String("content-blocking")));
	setDefaultAction(new QAction(this));
	setWindow(window);

	const ToolBarWidget *toolBar(qobject_cast<ToolBarWidget*>(parent));

	if (toolBar && toolBar->getDefinition().isGlobal())
	{
		connect(toolBar, &ToolBarWidget::windowChanged, this, &ContentBlockingInformationWidget::setWindow);
	}

	connect(m_elementsMenu, &QMenu::aboutToShow, this, &ContentBlockingInformationWidget::populateElementsMenu);
	connect(m_elementsMenu, &QMenu::triggered, this, &ContentBlockingInformationWidget::openElement);
	connect(m_profilesMenu, &QMenu::aboutToShow, this, &ContentBlockingInformationWidget::populateProfilesMenu);
	connect(m_profilesMenu, &QMenu::triggered, this, &ContentBlockingInformationWidget::toggleOption);
	connect(defaultAction(), &QAction::triggered, this, &ContentBlockingInformationWidget::toggleContentBlocking);
}

void ContentBlockingInformationWidget::changeEvent(QEvent *event)
{
	ToolButtonWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		updateState();
	}
}

void ContentBlockingInformationWidget::resizeEvent(QResizeEvent *event)
{
	ToolButtonWidget::resizeEvent(event);

	updateState();
}

void ContentBlockingInformationWidget::clear()
{
	m_requestsAmount = 0;

	updateState();
}

void ContentBlockingInformationWidget::openElement(QAction *action)
{
	if (action)
	{
		Application::triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), QUrl(action->statusTip())}}, parentWidget());
	}
}

void ContentBlockingInformationWidget::toggleContentBlocking()
{
	if (m_window && !m_window->isAboutToClose())
	{
		m_isContentBlockingEnabled = !m_window->getOption(SettingsManager::ContentBlocking_EnableContentBlockingOption).toBool();

		m_window->setOption(SettingsManager::ContentBlocking_EnableContentBlockingOption, m_isContentBlockingEnabled);

		updateState();
	}
}

void ContentBlockingInformationWidget::toggleOption(QAction *action)
{
	if (!m_window || !action || action->data().isNull())
	{
		return;
	}

	const QString profile(action->data().toString());
	const QStringList currentProfiles(m_window->getOption(SettingsManager::ContentBlocking_ProfilesOption).toStringList());
	QStringList profiles(currentProfiles);

	if (!action->isChecked())
	{
		profiles.removeAll(profile);
	}
	else if (!profiles.contains(profile))
	{
		profiles.append(profile);
	}

	if (profiles != currentProfiles)
	{
		m_window->setOption(SettingsManager::ContentBlocking_ProfilesOption, profiles);
	}
}

void ContentBlockingInformationWidget::populateElementsMenu()
{
	m_elementsMenu->clear();

	if (!m_window || !m_window->getWebWidget())
	{
		return;
	}

	const QVector<NetworkManager::ResourceInformation> requests(m_window->getWebWidget()->getBlockedRequests().mid(m_requestsAmount - 50));

	for (int i = 0; i < requests.count(); ++i)
	{
		const NetworkManager::ResourceInformation request(requests.at(i));
		QString type;

		switch (request.resourceType)
		{
			case NetworkManager::MainFrameType:
				type = tr("main frame");

				break;
			case NetworkManager::SubFrameType:
				type = tr("subframe");

				break;
			case NetworkManager::PopupType:
				type = tr("pop-up");

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
			case NetworkManager::WebSocketType:
				type = tr("WebSocket");

				break;
			default:
				type = tr("other");

				break;
		}

		QAction *action(m_elementsMenu->addAction(QStringLiteral("%1\t [%2]").arg(Utils::elideText(request.url.toString(), m_elementsMenu->fontMetrics(), m_elementsMenu), type)));
		action->setStatusTip(request.url.toString());
	}
}

void ContentBlockingInformationWidget::populateProfilesMenu()
{
	m_profilesMenu->clear();

	if (!m_window || !m_window->getWebWidget())
	{
		return;
	}

	QAction *enableContentBlockingAction(m_profilesMenu->addAction(tr("Enable Content Blocking"), this, &ContentBlockingInformationWidget::toggleContentBlocking));
	enableContentBlockingAction->setCheckable(true);
	enableContentBlockingAction->setChecked(m_window->getOption(SettingsManager::ContentBlocking_EnableContentBlockingOption).toBool());

	m_profilesMenu->addSeparator();

	const QVector<NetworkManager::ResourceInformation> requests(m_window->getWebWidget()->getBlockedRequests());
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

	const QVector<ContentFiltersProfile*> profiles(ContentFiltersManager::getContentBlockingProfiles());
	const QStringList enabledProfiles(m_window->getOption(SettingsManager::ContentBlocking_ProfilesOption).toStringList());

	for (int i = 0; i < profiles.count(); ++i)
	{
		ContentFiltersProfile* profile(profiles.at(i));

		if (profile)
		{
			const int amount(amounts.value(profile->getName()));
			const QString title(Utils::elideText(profile->getTitle(), m_profilesMenu->fontMetrics(), m_profilesMenu));
			QAction *profileAction(m_profilesMenu->addAction((amount > 0) ? QStringLiteral("%1 (%2)").arg(title, QString::number(amount)) : title));
			profileAction->setData(profile->getName());
			profileAction->setCheckable(true);
			profileAction->setChecked(enabledProfiles.contains(profile->getName()));
		}
	}
}

void ContentBlockingInformationWidget::handleBlockedRequest()
{
	++m_requestsAmount;

	updateState();
}

void ContentBlockingInformationWidget::updateState()
{
	const QVariantMap options(getOptions());

	if (isCustomized() && options.contains(QLatin1String("icon")))
	{
		const QVariant iconData(options[QLatin1String("icon")]);

		if (iconData.type() == QVariant::Icon)
		{
			m_icon = iconData.value<QIcon>();
		}
		else
		{
			m_icon = ThemesManager::createIcon(iconData.toString());
		}
	}
	else
	{
		m_icon = {};
	}

	if (m_icon.isNull())
	{
		m_icon = ThemesManager::createIcon(QLatin1String("content-blocking"));
	}

	const int iconSize(this->iconSize().width());
	const int fontSize(qMax((iconSize / 2), 12));
	QFont font(this->font());
	font.setBold(true);
	font.setPixelSize(fontSize);

	QString label;

	if (m_requestsAmount > 999999)
	{
		label = QString::number(m_requestsAmount / 1000000) + QLatin1Char('M');
	}
	else if (m_requestsAmount > 999)
	{
		label = QString::number(m_requestsAmount / 1000) + QLatin1Char('K');
	}
	else
	{
		label = QString::number(m_requestsAmount);
	}

	const qreal labelWidth(QFontMetricsF(font).horizontalAdvance(label));

	font.setPixelSize(qRound(fontSize * 0.8));

	const QRectF rectangle((iconSize - labelWidth), (iconSize - fontSize), labelWidth, fontSize);
	QPixmap pixmap(m_icon.pixmap(iconSize, iconSize, (m_isContentBlockingEnabled ? QIcon::Normal : QIcon::Disabled)));
	QPainter iconPainter(&pixmap);
	iconPainter.fillRect(rectangle, (m_isContentBlockingEnabled ? Qt::darkRed : Qt::darkGray));
	iconPainter.setFont(font);
	iconPainter.setPen(QColor(255, 255, 255, 230));
	iconPainter.drawText(rectangle, Qt::AlignCenter, label);

	m_icon = QIcon(pixmap);

	setText(getText());
	setToolTip(text());
	setIcon(m_icon);

	m_elementsMenu->setEnabled(m_requestsAmount > 0);
}

void ContentBlockingInformationWidget::setWindow(Window *window)
{
	if (m_window && !m_window->isAboutToClose())
	{
		disconnect(m_window, &Window::aboutToNavigate, this, &ContentBlockingInformationWidget::clear);
		disconnect(m_window, &Window::requestBlocked, this, &ContentBlockingInformationWidget::handleBlockedRequest);
	}

	m_window = window;
	m_requestsAmount = 0;

	if (window && window->getWebWidget())
	{
		m_requestsAmount = window->getWebWidget()->getBlockedRequests().count();
		m_isContentBlockingEnabled = (m_window->getOption(SettingsManager::ContentBlocking_EnableContentBlockingOption).toBool());

		connect(m_window, &Window::aboutToNavigate, this, &ContentBlockingInformationWidget::clear);
		connect(m_window, &Window::requestBlocked, this, &ContentBlockingInformationWidget::handleBlockedRequest);
	}
	else
	{
		m_isContentBlockingEnabled = false;
	}

	updateState();
	setEnabled(m_window);
}

QString ContentBlockingInformationWidget::getText() const
{
	QString text(tr("Blocked Elements: {amount}"));

	if (isCustomized())
	{
		const QVariantMap options(getOptions());

		if (options.contains(QLatin1String("text")))
		{
			text = options[QLatin1String("text")].toString();
		}
	}

	return text.replace(QLatin1String("{amount}"), QString::number(m_requestsAmount));
}

QIcon ContentBlockingInformationWidget::getIcon() const
{
	return m_icon;
}

}
