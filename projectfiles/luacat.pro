TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -std=c++11 -Wno-missing-field-initializers -Winline -fno-rtti -fno-exceptions

HEADERS += \
           include/lg/lg.hpp \
           include/lg/detail/lg_common.hpp \
           include/lg/detail/lg_utility.hpp \
           include/lg/detail/lg_stack.hpp \
           include/lg/detail/lg_storage.hpp

SOURCES += main.cpp

INCLUDEPATH += include
INCLUDEPATH += D:/projects/middleware/lua-5.3.3/include/

LIBS += -L"D:/projects/middleware/lua-5.3.3/" -llua53
