#include "WebWidget.h"
#include "ContentsWidget.h"

#include <QtCore/QUrl>
#include <QtGui/QIcon>
#include <QtWidgets/QMenu>

namespace Otter
{

WebWidget::WebWidget(bool privateWindow = false, ContentsWidget *parent) : QWidget(parent)
{
	Q_UNUSED(privateWindow)
}

void WebWidget::showContextMenu(const QPoint &position, MenuFlags flags)
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

		if (flags & FormMenu)
		{
			menu.addAction(getAction(CreateSearchAction));
			menu.addSeparator();
		}

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
		if (flags & EditMenu)
		{
			menu.addAction(getAction(UndoAction));
			menu.addAction(getAction(RedoAction));
			menu.addSeparator();
			menu.addAction(getAction(CutAction));
			menu.addAction(getAction(CopyAction));
			menu.addAction(getAction(PasteAction));
			menu.addAction(getAction(DeleteAction));
			menu.addSeparator();
			menu.addAction(getAction(SelectAllAction));
			menu.addAction(getAction(ClearAllAction));
			menu.addSeparator();

			if (flags & FormMenu)
			{
				menu.addAction(getAction(CreateSearchAction));
				menu.addSeparator();
			}

			if (flags == EditMenu || flags == (EditMenu | FormMenu))
			{
				menu.addAction(getAction(InspectElementAction));
				menu.addSeparator();
			}

			menu.addAction(getAction(SpellCheckAction));
			menu.addSeparator();
		}

		if (flags & SelectionMenu)
		{
			menu.addAction(getAction(SearchAction));
			menu.addAction(getAction(SearchMenuAction));
			menu.addSeparator();

			if (!(flags & EditMenu))
			{
				menu.addAction(getAction(CopyAction));
				menu.addSeparator();
			}

			menu.addAction(getAction(OpenSelectionAsLinkAction));
			menu.addSeparator();
		}

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

		if (flags & MediaMenu)
		{
			menu.addAction(getAction(CopyMediaUrlToClipboardAction));
			menu.addAction(getAction(SaveMediaToDiskAction));
			menu.addSeparator();
			menu.addAction(getAction(ToggleMediaPlayPauseAction));
			menu.addAction(getAction(ToggleMediaMuteAction));
			menu.addAction(getAction(ToggleMediaLoopAction));
			menu.addAction(getAction(ToggleMediaControlsAction));
			menu.addSeparator();
			menu.addAction(getAction(InspectElementAction));
			menu.addSeparator();
		}
	}

	menu.exec(mapToGlobal(position));
}

}
