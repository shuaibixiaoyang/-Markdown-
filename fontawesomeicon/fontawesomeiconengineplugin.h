// 文件说明：fontawesomeicon\fontawesomeiconengineplugin.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef FONTAWESOMEICONENGINEPLUGIN_H
#define FONTAWESOMEICONENGINEPLUGIN_H

#include <QIconEnginePlugin>


class FontAwesomeIconEnginePlugin : public QIconEnginePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QIconEngineFactoryInterface" FILE "fontawesomeicon.json")
    
public:
    FontAwesomeIconEnginePlugin(QObject *parent = nullptr);

    QIconEngine *create(const QString &filename = QString()) override;
};

#endif // FONTAWESOMEICONENGINEPLUGIN_H

