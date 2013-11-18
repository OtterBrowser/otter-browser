#include "core/Application.h"
#include "core/SessionsManager.h"
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

	const QString session = ((Otter::SessionsManager::getCurrentSession().isEmpty() || !parser->value("session").isEmpty()) ? (parser->value("session").isEmpty() ? "default" : parser->value("session")) : QString());

	if (session.isEmpty() || !Otter::SessionsManager::restoreSession(session))
	{
		Otter::MainWindow *window = application.createWindow(parser->isSet("privatesession"));

		if (window && !parser->positionalArguments().isEmpty())
		{
			QStringList urls = parser->positionalArguments();

			for (int i = 0; i < urls.count(); ++i)
			{
				window->openUrl(QUrl(urls.at(i)));
			}
		}
	}

	delete parser;

	return application.exec();
}
