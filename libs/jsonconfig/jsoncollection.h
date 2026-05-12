// 文件说明：libs\jsonconfig\jsoncollection.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef JSONCOLLECTION_H
#define JSONCOLLECTION_H

template <class T>
class JsonCollection
{
public:
    virtual ~JsonCollection() {}

    virtual int insert(const T &item) = 0;

    virtual const QString name() const = 0;
    virtual int count() const = 0;
    virtual const T &at(int offset) const = 0;
};

#endif // JSONCOLLECTION_H


