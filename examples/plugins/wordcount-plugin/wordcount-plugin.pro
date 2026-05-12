QT += core gui widgets
TEMPLATE = lib
CONFIG += plugin

TARGET = wordcount-plugin

# 指向 CuteMarkEd 源码目录（按需调整路径）
CUTEMARKED_SRC = ../../..

INCLUDEPATH += $$CUTEMARKED_SRC/app-static

HEADERS += wordcountplugin.h
SOURCES += wordcountplugin.cpp

# 安装到插件目录
# macOS: ~/Library/Application Support/<OrgName>/<AppName>/plugins
macx {
    PLUGIN_DIR = $$system("echo ~/Library/Application\\ Support/CuteMarkEd\\ Project/CuteMarkEd/plugins")
}
unix:!macx {
    PLUGIN_DIR = $$system("echo ~/.local/share/CuteMarkEd\\ Project/CuteMarkEd/plugins")
}
win32 {
    PLUGIN_DIR = $$system("echo %APPDATA%/CuteMarkEd\\ Project/CuteMarkEd/plugins")
}

target.path = $$PLUGIN_DIR
INSTALLS += target
