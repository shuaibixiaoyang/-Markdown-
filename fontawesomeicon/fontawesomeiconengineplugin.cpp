// 文件说明：fontawesomeicon\fontawesomeiconengineplugin.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "fontawesomeiconengineplugin.h"

#include "fontawesomeiconengine.h"

// 函数说明：构造 FontAwesomeIconEnginePlugin 对象，初始化本模块需要的状态、界面和资源。
FontAwesomeIconEnginePlugin::FontAwesomeIconEnginePlugin(QObject *parent) :
    QIconEnginePlugin(parent)
{
}

QIconEngine *FontAwesomeIconEnginePlugin::create(const QString &filename)
{
    FontAwesomeIconEngine *engine = new FontAwesomeIconEngine;
    if (!filename.isNull()) {
        int lastPoint = filename.lastIndexOf(".");
        engine->setIconName(filename.left(lastPoint));
    }
    return engine;
}

