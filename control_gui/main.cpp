#include <QApplication>
#include "viewform.h"
#include "control.h"
//#include "main.moc"
int main(int argc, char **argv)
{
	QApplication app (argc, argv);
	
	ViewForm form;
	form.show();
  
	return app.exec();
}
