TEMPLATE = app
TARGET = control_gui
#QT = core gui

DEPENDDEPATH += ../control_lib/
INCLUDEPATH += ../control_lib/

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

HEADERS	+= viewform.h 

SOURCES +=  main.cpp \
			viewform.cpp

LIBS	+=  -L/media/mikhail/AVGR/StreamProject/control_lib -lcontrol


