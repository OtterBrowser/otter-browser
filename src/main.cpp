#include "ui/MainWindow.h"

#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication application(argc, argv);
	application.setApplicationName("Otter");
	application.setApplicationVersion("0.0.01");
	application.setOrganizationName("eSoftware");
	application.setOrganizationDomain("otter.emdek.pl");

	Otter::MainWindow window;
	window.show();

	return application.exec();
}
