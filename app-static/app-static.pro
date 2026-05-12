#
# Application Static Libary Project for CuteMarkEd
#
# Github : https://github.com/cloose/CuteMarkEd
#

QT += gui sql charts widgets concurrent multimedia network svg qml core-private printsupport

# WebEngine/PDF/WebSockets: 如果本机 Qt 未安装对应模块，则自动降级到兼容层
# OpenSSL: MinGW 无法链接 MSVC 编译的 OpenSSL，使用 Qt 内置加密替代
win32-g++ {
    DEFINES += NO_WEBENGINE NO_PDFWIDGETS NO_WEBSOCKETS NO_OPENSSL
} else:win32 {
    exists($$[QT_INSTALL_HEADERS]/QtWebEngineWidgets) {
        QT += webenginewidgets
    } else {
        DEFINES += NO_WEBENGINE
    }

    exists($$[QT_INSTALL_HEADERS]/QtPdfWidgets) {
        QT += pdfwidgets
    } else {
        DEFINES += NO_PDFWIDGETS
    }

    exists($$[QT_INSTALL_HEADERS]/QtWebSockets) {
        QT += websockets
    } else {
        DEFINES += NO_WEBSOCKETS
    }
} else {
    QT += webenginewidgets pdfwidgets websockets
}

TARGET = app-static
TEMPLATE = lib
CONFIG += staticlib
CONFIG += c++17

INCLUDEPATH += $$PWD

SOURCES += \
    snippets/jsonsnippettranslator.cpp \
    snippets/snippetcollection.cpp \
    converter/discountmarkdownconverter.cpp \
    spellchecker/dictionary.cpp \
    converter/revealmarkdownconverter.cpp \
    template/htmltemplate.cpp \
    template/presentationtemplate.cpp \
    themes/jsonthemetranslator.cpp \
    themes/stylemanager.cpp \
    themes/theme.cpp \
    themes/themecollection.cpp \
    completionlistmodel.cpp \
    datalocation.cpp \
    slidelinemapping.cpp \
    viewsynchronizer.cpp \
    revealviewsynchronizer.cpp \
    htmlpreviewcontroller.cpp \
    htmlviewsynchronizer.cpp \
    yamlheaderchecker.cpp \
    rendering/diagramrenderer.cpp \
    rendering/mathrenderer.cpp \
    rendering/renderingstylemanager.cpp \
    datavisualization/datablockparser.cpp \
    datavisualization/databaseconnector.cpp \
    datavisualization/datachartwidget.cpp \
    datavisualization/variablemanager.cpp \
    datavisualization/datavisualizationpanel.cpp \
    security/fileencryption.cpp \
    security/biometricauth.cpp \
    security/securedocument.cpp \
    security/securitymanager.cpp \
    ocr/ocrengine.cpp \
    publishing/epubexporter.cpp \
    publishing/blogpublisher.cpp \
    publishing/slideshowpresenter.cpp \
    publishing/staticsitegenerator.cpp \
    academic/citationmanager.cpp \
    input/voiceinput.cpp \
    translation/translationmanager.cpp \
    export/wordexporter.cpp \
    export/latexexporter.cpp \
    export/mindmapview.cpp \
    library/noteslibrary.cpp \
    vcs/gitmanager.cpp \
    sync/cloudsync.cpp \
    editor/focusmode.cpp \
    template/templatemanager.cpp \
    editor/multitabeditor.cpp \
    sharing/documentsharing.cpp \
    annotation/commentmanager.cpp \
    revision/revisiontracker.cpp \
    revision/timelineview.cpp \
    export/imageexporter.cpp \
    writing/wordcountpanel.cpp \
    writing/outlinenavigator.cpp \
    writing/bookmarkmanager.cpp \
    writing/enhancedspellchecker.cpp \
    writing/writinggoal.cpp \
    preview/previewthememanager.cpp \
    preview/presentationmode.cpp \
    preview/printpreviewdialog.cpp \
    preview/tocfloatingwindow.cpp \
    extension/pluginmanager.cpp \
    extension/scriptengine.cpp \
    extension/shortcutmanager.cpp \
    extension/editortheme.cpp \
    collaboration/crdtdocument.cpp \
    collaboration/collaborationserver.cpp \
    collaboration/collaborationclient.cpp \
    collaboration/collaborationmanager.cpp \
    collaboration/collaborationpanel.cpp \
    collaboration/chatwidget.cpp \
    collaboration/remotecursoroverlay.cpp \
    ai/bailianclient.cpp \
    ai/aiwritingassistant.cpp \
    ai/aiwritingpanel.cpp \
    search/searchindexmanager.cpp \
    search/advancedsearchpanel.cpp \
    editor/largefilemanager.cpp \
    editor/memoryoptimizer.cpp

# 平台特定的生物特征认证
win32-msvc* {
    SOURCES += security/biometricauth_win.cpp
    DEFINES += HAS_WINRT_BIOMETRIC
}
macx: OBJECTIVE_SOURCES += security/biometricauth_mac.mm

HEADERS += \
    snippets/snippet.h \
    snippets/jsonsnippettranslator.h \
    snippets/jsonsnippettranslatorfactory.h \
    snippets/snippetcollection.h \
    converter/markdownconverter.h \
    converter/markdowndocument.h \
    converter/discountmarkdownconverter.h \
    spellchecker/dictionary.h \
    converter/revealmarkdownconverter.h \
    template/template.h \
    template/htmltemplate.h \
    template/presentationtemplate.h \
    themes/jsonthemetranslator.h \
    themes/jsonthemetranslatorfactory.h \
    themes/stylemanager.h \
    themes/theme.h \
    themes/themecollection.h \
    completionlistmodel.h \
    datalocation.h \
    slidelinemapping.h \
    viewsynchronizer.h \
    revealviewsynchronizer.h \
    htmlpreviewcontroller.h \
    htmlviewsynchronizer.h \
    yamlheaderchecker.h \
    rendering/diagramrenderer.h \
    rendering/mathrenderer.h \
    rendering/renderingstylemanager.h \
    datavisualization/datablockparser.h \
    datavisualization/databaseconnector.h \
    datavisualization/datachartwidget.h \
    datavisualization/variablemanager.h \
    datavisualization/datavisualizationpanel.h \
    security/fileencryption.h \
    security/biometricauth.h \
    security/securedocument.h \
    security/securitymanager.h \
    security/securememory.h \
    security/passwordstrength.h \
    security/securityaudit.h \
    security/fileintegrity.h \
    security/securitywidgets.h \
    security/keycache.h \
    ocr/screencapture.h \
    ocr/imagecompressor.h \
    ocr/ocrengine.h \
    ocr/smartnamer.h \
    ocr/assetmanager.h \
    ocr/assetpanel.h \
    ocr/captureeditor.h \
    ocr/screenshotdialog.h \
    ocr/pdfocr.h \
    ocr/ocrproofread.h \
    ocr/thumbnailcache.h \
    publishing/epubexporter.h \
    publishing/blogpublisher.h \
    publishing/slideshowpresenter.h \
    publishing/staticsitegenerator.h \
    academic/citationmanager.h \
    input/voiceinput.h \
    translation/translationmanager.h \
    export/wordexporter.h \
    export/latexexporter.h \
    export/mindmapview.h \
    library/noteslibrary.h \
    vcs/gitmanager.h \
    sync/cloudsync.h \
    editor/focusmode.h \
    template/templatemanager.h \
    editor/multitabeditor.h \
    sharing/documentsharing.h \
    annotation/commentmanager.h \
    revision/revisiontracker.h \
    revision/timelineview.h \
    export/imageexporter.h \
    writing/wordcountpanel.h \
    writing/outlinenavigator.h \
    writing/bookmarkmanager.h \
    writing/enhancedspellchecker.h \
    writing/writinggoal.h \
    preview/previewthememanager.h \
    preview/presentationmode.h \
    preview/printpreviewdialog.h \
    preview/tocfloatingwindow.h \
    extension/plugininterface.h \
    extension/pluginmanager.h \
    extension/scriptengine.h \
    extension/shortcutmanager.h \
    extension/editortheme.h \
    collaboration/crdtdocument.h \
    collaboration/collaborationserver.h \
    collaboration/collaborationclient.h \
    collaboration/collaborationmanager.h \
    collaboration/collaborationpanel.h \
    collaboration/chatwidget.h \
    collaboration/remotecursoroverlay.h \
    ai/bailianclient.h \
    ai/aiwritingassistant.h \
    ai/aiwritingpanel.h \
    search/searchindexmanager.h \
    search/advancedsearchpanel.h \
    editor/largefilemanager.h \
    editor/memoryoptimizer.h \
    compat/webenginecompat.h \
    compat/websocketcompat.h \
    compat/pdfcompat.h

#unix:!symbian {
#    maemo5 {
#        target.path = /opt/usr/lib
#    } else {
#        target.path = /usr/lib
#    }
#    INSTALLS += target
#}

##################################################
# Dependencies
##################################################

#
# Add search paths below /usr/local for Mac OSX
#
macx:INCLUDEPATH += /usr/local/include

#
# JSON configuration library
#
INCLUDEPATH += $$PWD/../libs/jsonconfig

#
# Discount library - 统一使用项目自带的 3rdparty/discount
#
INCLUDEPATH += $$PWD/../3rdparty/discount

#
# Hoedown library
#
with_hoedown {
    message("app-static: Enable hoedown markdown converter support")

    DEFINES += ENABLE_HOEDOWN
    SOURCES += converter/hoedownmarkdownconverter.cpp
    HEADERS += converter/hoedownmarkdownconverter.h

    INCLUDEPATH += $$PWD/../3rdparty/hoedown
    DEPENDPATH += $$PWD/../3rdparty/hoedown
}

##################################################
# OpenSSL (for encryption)
##################################################

# OpenSSL library paths (MinGW 使用 Qt 内置加密，不需要 OpenSSL)
win32:!win32-g++ {
    exists($$PWD/../3rdparty/openssl/include):exists($$PWD/../3rdparty/openssl/lib) {
        INCLUDEPATH += $$PWD/../3rdparty/openssl/include
        LIBS += -L$$PWD/../3rdparty/openssl/lib -lssl -lcrypto
    } else {
        DEFINES += NO_OPENSSL
    }
}

macx {
    INCLUDEPATH += /opt/homebrew/opt/openssl@3/include
    LIBS += -L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto
    # LocalAuthentication framework for Touch ID
    LIBS += -framework LocalAuthentication -framework Security
}

unix:!macx {
    CONFIG += link_pkgconfig
    PKGCONFIG += openssl
}

##################################################
# Windows-specific (for Windows Hello)
##################################################

win32-msvc* {
    LIBS += -lAdvapi32 -lCredui
    # WinRT for Windows Hello
    QMAKE_CXXFLAGS += /await
    LIBS += -lWindowsApp
}

win32-g++ {
    LIBS += -lAdvapi32 -lCredui
}

##################################################
# Tesseract OCR (optional)
##################################################

with_tesseract {
    message("app-static: Enable Tesseract OCR support")
    DEFINES += ENABLE_TESSERACT
    
    win32 {
        INCLUDEPATH += $$PWD/../3rdparty/tesseract/include
        LIBS += -L$$PWD/../3rdparty/tesseract/lib -ltesseract -lleptonica
    }
    
    macx {
        # Homebrew 安装的 Tesseract
        INCLUDEPATH += /usr/local/opt/tesseract/include
        INCLUDEPATH += /usr/local/opt/leptonica/include
        LIBS += -L/usr/local/opt/tesseract/lib -ltesseract
        LIBS += -L/usr/local/opt/leptonica/lib -lleptonica
    }
    
    unix:!macx {
        PKGCONFIG += tesseract lept
    }
}
