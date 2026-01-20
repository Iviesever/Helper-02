#include "MainWindow.h"

#include <QtWidgets/QApplication>

int main(int argc, char * argv[])
{
	QApplication app(argc, argv);

	app.setWindowIcon(QIcon(":/MainWindow/favicon.ico"));

	MainWindow window;
	window.show();
	return app.exec();
}
