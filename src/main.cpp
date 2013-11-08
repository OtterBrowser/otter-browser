#include "ui/MainWindow.h"

#include <QtCore/QLocale>
#include <QtCore/QTranslator>
#include <QtCore/QLibraryInfo>
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QTranslator qtTranslator;
	qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));

	QTranslator applicationTranslator;
	applicationTranslator.load(":/translations/otter_" + QLocale::system().name());

	QApplication application(argc, argv);
	application.setApplicationName("Otter");
	application.setApplicationVersion("0.0.01");
	application.setOrganizationName("eSoftware");
	application.setOrganizationDomain("otter.emdek.pl");
	application.installTranslator(&qtTranslator);
	application.installTranslator(&applicationTranslator);

	Otter::MainWindow window;
	window.show();

	return application.exec();
}
