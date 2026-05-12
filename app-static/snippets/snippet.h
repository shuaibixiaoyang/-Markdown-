// 文件说明：app-static\snippets\snippet.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef SNIPPET_H
#define SNIPPET_H

#include <QString>


struct Snippet
{
    QString trigger;
    QString description;
    QString snippet;
    int cursorPosition;
    bool builtIn;

    Snippet() : cursorPosition(0), builtIn(false) {}

    bool operator<(const Snippet &rhs) const
    {
        return trigger < rhs.trigger;
    }

    bool operator ==(const Snippet &rhs) const
    {
        return trigger == rhs.trigger;
    }
};

#endif // SNIPPET_H

