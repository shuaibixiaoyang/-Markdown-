// 文件说明：app\highlightworkerthread.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef HIGHLIGHTWORKERTHREAD_H
#define HIGHLIGHTWORKERTHREAD_H
//解析 Markdown 语法（标题、粗体、代码块、链接...）
#include <QtCore/qthread.h>
#include <QtCore/qqueue.h>
#include <QtCore/qmutex.h>
#include <QtCore/qwaitcondition.h>

#include "pmh_definitions.h"

//任务结构体： 文本+偏移量
struct Task
{
    QString text;
    unsigned long offset;
};
// 高亮工作线程
// 作用：在后台解析 Markdown 语法，不卡 UI
class HighlightWorkerThread : public QThread
{
    Q_OBJECT

public:
    explicit HighlightWorkerThread(QObject *parent = nullptr);
    //把解析任务加入队列
    void enqueue(const QString &text, unsigned long offset = 0);

signals:
    //解析完成后发送结果给ui线程上色
    void resultReady(pmh_element **elements, unsigned long offset);

protected:
    //线程主循环
    virtual void run();

private:
    //任务队列
    QQueue<Task> tasks;
    //锁：保证队列安全
    QMutex tasksMutex;
    //等待新任务
    QWaitCondition bufferNotEmpty;
};

#endif // HIGHLIGHTWORKERTHREAD_H

