// 文件说明：app\savefileadapter.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef SAVEFILEADAPTER_H
#define SAVEFILEADAPTER_H

#include <QSaveFile>

//保存文件
class SaveFileAdapter : public QSaveFile
{
public:
    SaveFileAdapter(const QString &name) :
        QSaveFile(name)
    {
    }

    void close() {
        // IGNORE - work-around for the problem that QTextDocumentWriter::write()
        // calls close() on the device
    }

};

#endif // SAVEFILEADAPTER_H

