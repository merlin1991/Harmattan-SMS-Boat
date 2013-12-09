QT -= gui
CONFIG += debug
HEADERS += catcher.h
SOURCES += smsIm.cpp

#libcommhistory
CONFIG += link_pkgconfig
PKGCONFIG += commhistory-qt5
