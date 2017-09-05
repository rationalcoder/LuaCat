TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -std=c++11 -Wno-missing-field-initializers -Winline -fno-rtti -fno-exceptions

HEADERS += \
           include/lc/lc.hpp \
           include/lc/detail/lc_common.hpp \
           include/lc/detail/lc_utility.hpp \
           include/lc/detail/lc_stack.hpp \
           include/lc/detail/lc_storage.hpp

SOURCES += main.cpp

INCLUDEPATH += include
INCLUDEPATH += D:/projects/middleware/lua-5.3.3/include/

LIBS += -L"D:/projects/middleware/lua-5.3.3/" -llua53
