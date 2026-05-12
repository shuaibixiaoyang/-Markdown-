#
# Integration Test Project for CuteMarkEd
#
# Github : https://github.com/cloose/CuteMarkEd
#

QT       += testlib
QT       += gui widgets network sql charts concurrent printsupport multimedia svg qml

# WebEngine/PDF/WebSockets/OpenSSL: MinGW 不支持，使用兼容层替代
win32-g++ {
    DEFINES += NO_WEBENGINE NO_PDFWIDGETS NO_WEBSOCKETS NO_OPENSSL
} else {
    QT += webenginewidgets pdfwidgets websockets pdf
}

TARGET = integrationtest
CONFIG += console testcase
CONFIG += c++17

SOURCES += \
    ../../app/aboutdialog.cpp \
    ../../app/controls/activelabel.cpp \
    ../../app/controls/fileexplorerwidget.cpp \
    ../../app/controls/findreplacewidget.cpp \
    ../../app/controls/languagemenu.cpp \
    ../../app/controls/linenumberarea.cpp \
    ../../app/controls/recentfilesmenu.cpp \
    ../../app/exporthtmldialog.cpp \
    ../../app/exportpdfdialog.cpp \
    ../../app/highlightworkerthread.cpp \
    ../../app/htmlhighlighter.cpp \
    ../../app/htmlpreviewgenerator.cpp \
    ../../app/hunspell/spellchecker.cpp \
    ../../app/imagetooldialog.cpp \
    ../../app/mainwindow.cpp \
    ../../app/markdowneditor.cpp \
    ../../app/markdownhighlighter.cpp \
    ../../app/markdownmanipulator.cpp \
    ../../app/options.cpp \
    ../../app/optionsdialog.cpp \
    ../../app/snippetcompleter.cpp \
    ../../app/snippetstablemodel.cpp \
    ../../app/statusbarwidget.cpp \
    ../../app/tabletooldialog.cpp \
    blogpublishertest.cpp \
    cloudsyncintegrationtest.cpp \
    collaborationintegrationtest.cpp \
    datavisualizationpanelintegrationtest.cpp \
    ocrintegrationtest.cpp \
    discountmarkdownconvertertest.cpp \
    epubexportertest.cpp \
    htmlpreviewcontrollertest.cpp \
    htmltemplatetest.cpp \
    jsonsnippetfiletest.cpp \
    jsonthemefiletest.cpp \
    latexexportertest.cpp \
    main.cpp \
    mainwindowlfsintegrationtest.cpp \
    pmhmarkdownparsertest.cpp \
    revealmarkdownconvertertest.cpp \
    themecollectiontest.cpp

win32 {
    SOURCES += ../../app/hunspell/spellchecker_win.cpp
}

macx {
    OBJECTIVE_SOURCES += ../../app/hunspell/spellchecker_macx.mm
}

unix:!macx {
    SOURCES += ../../app/hunspell/spellchecker_unix.cpp
}

HEADERS += \
    ../../app/aboutdialog.h \
    ../../app/controls/activelabel.h \
    ../../app/controls/fileexplorerwidget.h \
    ../../app/controls/findreplacewidget.h \
    ../../app/controls/languagemenu.h \
    ../../app/controls/linenumberarea.h \
    ../../app/controls/recentfilesmenu.h \
    ../../app/exporthtmldialog.h \
    ../../app/exportpdfdialog.h \
    ../../app/highlightworkerthread.h \
    ../../app/htmlhighlighter.h \
    ../../app/htmlpreviewgenerator.h \
    ../../app/hunspell/spellchecker.h \
    ../../app/imagetooldialog.h \
    ../../app/mainwindow.h \
    ../../app/markdowneditor.h \
    ../../app/markdownhighlighter.h \
    ../../app/markdownmanipulator.h \
    ../../app/options.h \
    ../../app/optionsdialog.h \
    ../../app/savefileadapter.h \
    ../../app/snippetcompleter.h \
    ../../app/snippetstablemodel.h \
    ../../app/statusbarwidget.h \
    ../../app/tabletooldialog.h \
    blogpublishertest.h \
    cloudsyncintegrationtest.h \
    collaborationintegrationtest.h \
    datavisualizationpanelintegrationtest.h \
    ocrintegrationtest.h \
    discountmarkdownconvertertest.h \
    epubexportertest.h \
    htmlpreviewcontrollertest.h \
    htmltemplatetest.h \
    jsonsnippetfiletest.h \
    jsonthemefiletest.h \
    latexexportertest.h \
    mainwindowlfsintegrationtest.h \
    pmhmarkdownparsertest.h \
    revealmarkdownconvertertest.h \
    themecollectiontest.h

FORMS += \
    ../../app/aboutdialog.ui \
    ../../app/controls/fileexplorerwidget.ui \
    ../../app/controls/findreplacewidget.ui \
    ../../app/exporthtmldialog.ui \
    ../../app/exportpdfdialog.ui \
    ../../app/imagetooldialog.ui \
    ../../app/mainwindow.ui \
    ../../app/optionsdialog.ui \
    ../../app/tabletooldialog.ui

RESOURCES += \
    ../../app/resources.qrc \
    ../../app/translations.qrc

target.CONFIG += no_default_install

#
# JSON configuration library
#
INCLUDEPATH += $$PWD/../../libs/jsonconfig
INCLUDEPATH += $$PWD/../../app
INCLUDEPATH += $$PWD/../../libs

#
# macOS specific frameworks and OpenSSL
#
macx {
  LIBS += -L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto
  INCLUDEPATH += /opt/homebrew/opt/openssl@3/include
  LIBS += -framework LocalAuthentication -framework Security -framework AppKit
}

# hunspell - 统一使用项目自带的 3rdparty/hunspell
win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../../3rdparty/hunspell/lib/ -lhunspell
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../../3rdparty/hunspell/lib/ -lhunspell
else:unix: LIBS += -L$$OUT_PWD/../../3rdparty/hunspell/lib/ -lhunspell
INCLUDEPATH += $$PWD/../../3rdparty/hunspell/src

unix:!macx {
  CONFIG += link_pkgconfig
  PKGCONFIG += openssl hunspell
}

##################################################
# Use internal static library: app-static
##################################################
win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../../app-static/release/ -lapp-static
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../../app-static/debug/ -lapp-static
else:unix: LIBS += -L$$OUT_PWD/../../app-static/ -lapp-static

INCLUDEPATH += $$PWD/../../app-static
DEPENDPATH += $$PWD/../../app-static

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../../app-static/release/libapp-static.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../../app-static/debug/libapp-static.a
else:win32-msvc*:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../../app-static/release/app-static.lib
else:win32-msvc*:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../../app-static/debug/app-static.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../../app-static/libapp-static.a

#
# PEG Markdown Highlight adapter library
#
win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../../libs/peg-markdown-highlight/release/ -lpmh-adapter
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../../libs/peg-markdown-highlight/debug/ -lpmh-adapter
else:unix: LIBS += -L$$OUT_PWD/../../libs/peg-markdown-highlight/ -lpmh-adapter

INCLUDEPATH += $$PWD/../../libs/peg-markdown-highlight
DEPENDPATH += $$PWD/../../libs/peg-markdown-highlight

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../../libs/peg-markdown-highlight/release/libpmh-adapter.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../../libs/peg-markdown-highlight/debug/libpmh-adapter.a
else:win32-msvc*:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../../libs/peg-markdown-highlight/release/pmh-adapter.lib
else:win32-msvc*:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../../libs/peg-markdown-highlight/debug/pmh-adapter.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../../libs/peg-markdown-highlight/libpmh-adapter.a

#
# peg-markdown-highlight
#
win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../../3rdparty/peg-markdown-highlight/release/ -lpmh
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../../3rdparty/peg-markdown-highlight/debug/ -lpmh
else:unix: LIBS += -L$$OUT_PWD/../../3rdparty/peg-markdown-highlight/ -lpmh

INCLUDEPATH += $$PWD/../../3rdparty/peg-markdown-highlight
DEPENDPATH += $$PWD/../../3rdparty/peg-markdown-highlight

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../../3rdparty/peg-markdown-highlight/release/libpmh.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../../3rdparty/peg-markdown-highlight/debug/libpmh.a
else:win32-msvc*:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../../3rdparty/peg-markdown-highlight/release/pmh.lib
else:win32-msvc*:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../../3rdparty/peg-markdown-highlight/debug/pmh.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../../3rdparty/peg-markdown-highlight/libpmh.a

#
# Discount library - 统一使用项目自带的 3rdparty/discount
#
win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../../3rdparty/discount/release/ -ldiscount
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../../3rdparty/discount/debug/ -ldiscount
else:unix: LIBS += -L$$OUT_PWD/../../3rdparty/discount/ -ldiscount

INCLUDEPATH += $$PWD/../../3rdparty/
DEPENDPATH += $$PWD/../../3rdparty/

#win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../../3rdparty/discount/release/libdiscount.a
#else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../../3rdparty/discount/debug/libdiscount.a

#
# hoedown
#
with_hoedown {
    DEFINES += ENABLE_HOEDOWN
    SOURCES += hoedownmarkdownconvertertest.cpp
    HEADERS += hoedownmarkdownconvertertest.h

    win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../../3rdparty/hoedown/release/ -lhoedown
    else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../../3rdparty/hoedown/debug/ -lhoedown
    else:unix: LIBS += -L$$OUT_PWD/../../3rdparty/hoedown/ -lhoedown

    INCLUDEPATH += $$PWD/../../3rdparty/hoedown
    DEPENDPATH += $$PWD/../../3rdparty/hoedown

    #win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../../3rdparty/hoedown/release/libhoedown.a
    #else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../../3rdparty/hoedown/debug/libhoedown.a
}
