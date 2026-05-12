// 文件说明：app\highlightworkerthread.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "highlightworkerthread.h"

#include "pmh_parser.h"

// 函数说明：构造 HighlightWorkerThread 对象，初始化本模块需要的状态、界面和资源。
HighlightWorkerThread::HighlightWorkerThread(QObject *parent) :
    QThread(parent)
{
}


//任务加入队列
void HighlightWorkerThread::enqueue(const QString &text, unsigned long offset)
{
    QMutexLocker locker(&tasksMutex);
    tasks.enqueue(Task {text, offset});
    bufferNotEmpty.wakeOne();
}

    //线程主循环
void HighlightWorkerThread::run()
{
    forever {
        Task task;

        {
            // wait for new task
            QMutexLocker locker(&tasksMutex);
            while (tasks.count() == 0) {
                bufferNotEmpty.wait(&tasksMutex);
            }

            // get last task from queue and skip all previous tasks
            while (!tasks.isEmpty())
                task = tasks.dequeue();
        }

        // end processing?
        if (task.text.isNull()) {
            return;
        }

        // delay processing by 500 ms to see if more tasks are coming
        // (e.g. because the user is typing fast)
        this->msleep(500);

        // no more new tasks? (must check under lock to avoid data race)
        bool empty;
        {
            QMutexLocker locker(&tasksMutex);
            empty = tasks.isEmpty();
        }
        if (empty) {
            // parse markdown and generate syntax elements
            pmh_element **elements;
            pmh_markdown_to_elements(task.text.toUtf8().data(), pmh_EXT_NONE, &elements);

            emit resultReady(elements, task.offset);
        }
    }
}

