TARGET = qeglfs_hwcomposer

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QEglFSIntegrationPlugin
load(qt_plugin)

SOURCES += $$PWD/main.cpp

include(eglfs.pri)
