#include "WebWidget.h"

#include <QtCore/QUrl>
#include <QtGui/QIcon>
#include <QtWidgets/QMenu>

namespace Otter
{

WebWidget::WebWidget(QWidget *parent) : QWidget(parent)
{
}

void WebWidget::print(QPrinter *printer)
{
	Q_UNUSED(printer)
}

void WebWidget::triggerAction(WebAction action, bool checked)
{
	Q_UNUSED(action)
	Q_UNUSED(checked)
}

void WebWidget::setZoom(int zoom)
{
	Q_UNUSED(zoom)
}

void WebWidget::setUrl(const QUrl &url)
{
	Q_UNUSED(url)
}

void WebWidget::setPrivate(bool enabled)
{
	Q_UNUSED(enabled)
}

void WebWidget::showMenu(const QPoint &position, MenuFlags flags)
{
	QMenu menu;

	if (flags & StandardMenu)
	{
		menu.addAction(getAction(GoBackAction));
		menu.addAction(getAction(GoForwardAction));
		menu.addAction(getAction(RewindBackAction));
		menu.addAction(getAction(RewindForwardAction));
		menu.addSeparator();
		menu.addAction(getAction(ReloadAction));
		menu.addAction(getAction(ReloadTimeAction));
		menu.addSeparator();
		menu.addAction(getAction(BookmarkAction));
		menu.addAction(getAction(CopyAddressAction));
		menu.addAction(getAction(PrintAction));
		menu.addSeparator();
		menu.addAction(getAction(InspectElementAction));
		menu.addAction(getAction(ViewSourceAction));
		menu.addAction(getAction(ValidateAction));
		menu.addSeparator();

		if (flags & FrameMenu)
		{
			QMenu *frameMenu = new QMenu(&menu);
			frameMenu->setTitle(tr("Frame"));
			frameMenu->addAction(getAction(OpenFrameInThisTabAction));
			frameMenu->addAction(getAction(OpenFrameInNewTabAction));
			frameMenu->addAction(getAction(OpenFrameInNewTabBackgroundAction));
			frameMenu->addSeparator();
			frameMenu->addAction(getAction(ViewSourceFrameAction));
			frameMenu->addAction(getAction(ReloadFrameAction));
			frameMenu->addAction(getAction(CopyFrameLinkToClipboardAction));

			menu.addMenu(frameMenu);
			menu.addSeparator();
		}

		menu.addAction(getAction(ContentBlockingAction));
		menu.addAction(getAction(WebsitePreferencesAction));
		menu.addSeparator();
		menu.addAction(getAction(FullScreenAction));
	}
	else
	{
		if (flags & LinkMenu)
		{
			menu.addAction(getAction(OpenLinkInThisTabAction));
			menu.addAction(getAction(OpenLinkInNewTabAction));
			menu.addAction(getAction(OpenLinkInNewTabBackgroundAction));
			menu.addSeparator();
			menu.addAction(getAction(OpenLinkInNewWindowAction));
			menu.addAction(getAction(OpenLinkInNewWindowBackgroundAction));
			menu.addSeparator();
			menu.addAction(getAction(BookmarkLinkAction));
			menu.addAction(getAction(CopyLinkToClipboardAction));
			menu.addSeparator();
			menu.addAction(getAction(SaveLinkToDiskAction));
			menu.addAction(getAction(SaveLinkToDownloadsAction));

			if (!(flags & ImageMenu))
			{
				menu.addAction(getAction(InspectElementAction));
			}

			menu.addSeparator();
		}

		if (flags & ImageMenu)
		{
			menu.addAction(getAction(OpenImageInNewTabAction));
			menu.addAction(getAction(CopyImageUrlToClipboardAction));
			menu.addSeparator();
			menu.addAction(getAction(SaveImageToDiskAction));
			menu.addAction(getAction(CopyImageToClipboardAction));
			menu.addSeparator();
			menu.addAction(getAction(InspectElementAction));
			menu.addAction(getAction(ImagePropertiesAction));
			menu.addSeparator();
		}
	}

	menu.exec(mapToGlobal(position));
}

WebWidget* WebWidget::clone(QWidget *parent)
{
	Q_UNUSED(parent)

	return NULL;
}

QAction* WebWidget::getAction(WebAction action)
{
	Q_UNUSED(action)

	return NULL;
}

QUndoStack* WebWidget::getUndoStack()
{
	return NULL;
}

QString WebWidget::getTitle() const
{
	return QString();
}

QUrl WebWidget::getUrl() const
{
	return QUrl();
}

QIcon WebWidget::getIcon() const
{
	return QIcon();
}

int WebWidget::getZoom() const
{
	return 100;
}

bool WebWidget::isLoading() const
{
	return false;
}

bool WebWidget::isPrivate() const
{
	return false;
}


}
