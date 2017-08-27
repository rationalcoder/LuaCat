TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -std=c++11 -Wno-missing-field-initializers -Winline
HEADERS += \
           lg_stack.hpp \
           lg_common.hpp \
    lg_utility.hpp \
    lg.hpp

SOURCES += main.cpp

INCLUDEPATH += D:/projects/middleware/lua-5.3.3/include/

LIBS += -L"D:/projects/middleware/lua-5.3.3/" -llua53
