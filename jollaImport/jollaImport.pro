QT -= gui
CONFIG += debug
HEADERS += catcher.h
SOURCES += jollaIm.cpp

#libcommhistory
CONFIG += link_pkgconfig
PKGCONFIG += commhistory-qt5

PKGCONFIG += Qt5Contacts

PKGCONFIG += qtcontacts-sqlite-qt5-extensions

PKGCONFIG += Qt5DBus
