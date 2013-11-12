#include "core/Application.h"
#include "ui/MainWindow.h"

#include <QtCore/QUrl>

int main(int argc, char *argv[])
{
	Otter::Application application(argc, argv);

	if (application.isRunning())
	{
		return 0;
	}

	QCommandLineParser *parser = application.getParser();
	parser->process(application);

	Otter::MainWindow *window = application.createWindow(parser->isSet("privatesession"));

	if (window && !parser->positionalArguments().isEmpty())
	{
		QStringList urls = parser->positionalArguments();

		for (int i = 0; i < urls.count(); ++i)
		{
			window->openUrl(QUrl(urls.at(i)));
		}
	}

	delete parser;

	return application.exec();
}
