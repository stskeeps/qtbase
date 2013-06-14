QT += core-private gui-private platformsupport-private

DEFINES += QEGL_EXTRA_DEBUG

#Avoid X11 header collision
DEFINES += MESA_EGL_NO_X11_HEADERS

#To test the hooks on x11 (xlib), comment the above define too
#EGLFS_PLATFORM_HOOKS_SOURCES += qeglfshooks_x11.cpp
#LIBS += -lX11 -lX11-xcb -lxcb

SOURCES +=  $$PWD/qeglfsintegration.cpp \
            $$PWD/qeglfswindow.cpp \
            $$PWD/qeglfsbackingstore.cpp \
            $$PWD/qeglfsscreen.cpp \
            $$PWD/qeglfshooks_stub.cpp \
            $$PWD/qeglfscursor.cpp \
            $$PWD/qeglfscontext.cpp

HEADERS +=  $$PWD/qeglfsintegration.h \
            $$PWD/qeglfswindow.h \
            $$PWD/qeglfsbackingstore.h \
            $$PWD/qeglfsscreen.h \
            $$PWD/qeglfscursor.h \
            $$PWD/qeglfshooks.h \
            $$PWD/qeglfscontext.h

QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

INCLUDEPATH += $$PWD

!isEmpty(EGLFS_PLATFORM_HOOKS_SOURCES) {
    HEADERS += $$EGLFS_PLATFORM_HOOKS_HEADERS
    SOURCES += $$EGLFS_PLATFORM_HOOKS_SOURCES
    LIBS    += $$EGLFS_PLATFORM_HOOKS_LIBS
    DEFINES += EGLFS_PLATFORM_HOOKS
}

LIBS += -lhybris-eglplatformcommon -lhybris-hwcomposerwindow -lhardware -lsync
QMAKE_CXXFLAGS += -I/usr/include/android -I/usr/include/hybris/eglplatformcommon -I/usr/include/hybris/hwcomposerwindow

CONFIG += egl qpa/genericunixfontdatabase

RESOURCES += $$PWD/cursor.qrc

OTHER_FILES += \
    $$PWD/eglfs.json
