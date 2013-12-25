QT -= gui
CONFIG += debug
HEADERS += catcher.h
SOURCES += jollaIm.cpp

#libcommhistory
CONFIG += link_pkgconfig
PKGCONFIG += commhistory-qt5
