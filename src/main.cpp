#include "core/Application.h"

int main(int argc, char *argv[])
{
	Otter::Application application(argc, argv);

	if (application.isRunning())
	{
		return 0;
	}

	application.newWindow();

	return application.exec();
}
