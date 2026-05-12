// 文件说明：app-static\collaboration\crdtdocument.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "crdtdocument.h"
#include <QMutexLocker>
#include <algorithm>

namespace Collaboration {

// 函数说明：构造 CrdtDocument 对象，初始化本模块需要的状态、界面和资源。
CrdtDocument::CrdtDocument(QObject *parent)
    : QObject(parent)
    , m_siteId(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_clock(0)
{
}

// 函数说明：构造 CrdtDocument 对象，初始化本模块需要的状态、界面和资源。
CrdtDocument::CrdtDocument(const QString &siteId, QObject *parent)
    : QObject(parent)
    , m_siteId(siteId)
    , m_clock(0)
{
}

// 函数说明：设置 CrdtDocument 的运行参数，并触发必要的界面或数据刷新。
void CrdtDocument::setSiteId(const QString &siteId)
{
    m_siteId = siteId;
}

// 函数说明：根据当前数据生成 CrdtDocument 需要的输出结果。
CharId CrdtDocument::generateId()
{
    CharId id;
    id.siteId = m_siteId;
    id.clock = ++m_clock;
    id.offset = 0;
    return id;
}

// 函数说明：实现 CrdtDocument::localInsert 的核心逻辑，供当前模块调用。
Operation CrdtDocument::localInsert(int position, QChar character)
{
    QMutexLocker locker(&m_mutex);

    CrdtChar ch;
    ch.id = generateId();
    ch.value = character;
    ch.deleted = false;

    // Find parent (the character before insertion point)
    if (position > 0) {
        int visibleCount = 0;
        for (int i = 0; i < m_chars.size(); ++i) {
            if (!m_chars[i].deleted) {
                visibleCount++;
                if (visibleCount == position) {
                    ch.parentId = m_chars[i].id;
                    break;
                }
            }
        }
    }
    // If position is 0, parentId remains null (inserted at beginning)

    insertChar(ch);

    Operation op;
    op.type = OperationType::Insert;
    op.character = ch;

    emit textChanged();
    emit operationGenerated(op);

    return op;
}

// 函数说明：实现 CrdtDocument::localDelete 的核心逻辑，供当前模块调用。
Operation CrdtDocument::localDelete(int position)
{
    QMutexLocker locker(&m_mutex);

    Operation op;
    op.type = OperationType::Delete;

    // Find the character at the given visible position
    int visibleCount = 0;
    for (int i = 0; i < m_chars.size(); ++i) {
        if (!m_chars[i].deleted) {
            if (visibleCount == position) {
                m_chars[i].deleted = true;
                op.targetId = m_chars[i].id;
                break;
            }
            visibleCount++;
        }
    }

    emit textChanged();
    emit operationGenerated(op);

    return op;
}

// 函数说明：实现 CrdtDocument::remoteInsert 的核心逻辑，供当前模块调用。
void CrdtDocument::remoteInsert(const CrdtChar &character)
{
    QMutexLocker locker(&m_mutex);

    // Update clock to be at least as high as the remote clock
    if (character.id.clock > m_clock) {
        m_clock = character.id.clock;
    }

    // Check if already exists
    for (const auto &ch : m_chars) {
        if (ch.id == character.id) {
            return; // Already have this character
        }
    }

    int insertPos = findInsertPosition(character);
    m_chars.insert(insertPos, character);

    // Calculate local position for UI update
    int localPos = 0;
    for (int i = 0; i < insertPos; ++i) {
        if (!m_chars[i].deleted) {
            localPos++;
        }
    }

    emit remoteOperationApplied(localPos, 0, 1);
    emit textChanged();
}

// 函数说明：实现 CrdtDocument::remoteDelete 的核心逻辑，供当前模块调用。
void CrdtDocument::remoteDelete(const CharId &targetId)
{
    QMutexLocker locker(&m_mutex);

    for (int i = 0; i < m_chars.size(); ++i) {
        if (m_chars[i].id == targetId && !m_chars[i].deleted) {
            // Calculate local position before deletion
            int localPos = 0;
            for (int j = 0; j < i; ++j) {
                if (!m_chars[j].deleted) {
                    localPos++;
                }
            }

            m_chars[i].deleted = true;

            emit remoteOperationApplied(localPos, 1, 0);
            emit textChanged();
            return;
        }
    }
}

// 函数说明：应用 CrdtDocument 当前配置，让编辑器或预览立即生效。
void CrdtDocument::applyOperation(const Operation &op)
{
    if (op.type == OperationType::Insert) {
        remoteInsert(op.character);
    } else {
        remoteDelete(op.targetId);
    }
}

// 函数说明：实现 CrdtDocument::findInsertPosition 的核心逻辑，供当前模块调用。
int CrdtDocument::findInsertPosition(const CrdtChar &ch) const
{
    if (m_chars.isEmpty()) {
        return 0;
    }

    // Find parent position
    int parentIndex = -1;
    if (!ch.parentId.isNull()) {
        parentIndex = findCharIndex(ch.parentId);
    }

    // Insert after parent, but before any character with a smaller ID that also has the same parent
    int insertPos = parentIndex + 1;

    while (insertPos < m_chars.size()) {
        const CrdtChar &existing = m_chars[insertPos];

        // Stop if we find a character with a different parent that comes after ours
        if (existing.parentId != ch.parentId && !existing.parentId.isNull()) {
            // Check if existing's parent is after ch's parent
            int existingParentIdx = findCharIndex(existing.parentId);
            if (existingParentIdx > parentIndex) {
                break;
            }
        }

        // Use ID comparison for consistent ordering among siblings
        if (existing.parentId == ch.parentId && ch.id < existing.id) {
            break;
        }

        insertPos++;
    }

    return insertPos;
}

// 函数说明：实现 CrdtDocument::findCharIndex 的核心逻辑，供当前模块调用。
int CrdtDocument::findCharIndex(const CharId &id) const
{
    for (int i = 0; i < m_chars.size(); ++i) {
        if (m_chars[i].id == id) {
            return i;
        }
    }
    return -1;
}

// 函数说明：实现 CrdtDocument::insertChar 的核心逻辑，供当前模块调用。
void CrdtDocument::insertChar(const CrdtChar &ch)
{
    int pos = findInsertPosition(ch);
    m_chars.insert(pos, ch);
}

// 函数说明：实现 CrdtDocument::text 的核心逻辑，供当前模块调用。
QString CrdtDocument::text() const
{
    QMutexLocker locker(&m_mutex);

    QString result;
    for (const auto &ch : m_chars) {
        if (!ch.deleted) {
            result.append(ch.value);
        }
    }
    return result;
}

// 函数说明：实现 CrdtDocument::length 的核心逻辑，供当前模块调用。
int CrdtDocument::length() const
{
    QMutexLocker locker(&m_mutex);

    int count = 0;
    for (const auto &ch : m_chars) {
        if (!ch.deleted) {
            count++;
        }
    }
    return count;
}

// 函数说明：实现 CrdtDocument::crdtToLocal 的核心逻辑，供当前模块调用。
int CrdtDocument::crdtToLocal(const CharId &id) const
{
    QMutexLocker locker(&m_mutex);

    int localPos = 0;
    for (const auto &ch : m_chars) {
        if (ch.id == id) {
            return localPos;
        }
        if (!ch.deleted) {
            localPos++;
        }
    }
    return -1;
}

// 函数说明：实现 CrdtDocument::localToCrdt 的核心逻辑，供当前模块调用。
CharId CrdtDocument::localToCrdt(int position) const
{
    QMutexLocker locker(&m_mutex);

    int visibleCount = 0;
    for (const auto &ch : m_chars) {
        if (!ch.deleted) {
            if (visibleCount == position) {
                return ch.id;
            }
            visibleCount++;
        }
    }
    return CharId();
}

// 函数说明：实现 CrdtDocument::toJson 的核心逻辑，供当前模块调用。
QJsonObject CrdtDocument::toJson() const
{
    QMutexLocker locker(&m_mutex);

    QJsonObject obj;
    obj["siteId"] = m_siteId;
    obj["clock"] = m_clock;
    obj["chars"] = getState();
    return obj;
}

// 函数说明：实现 CrdtDocument::fromJson 的核心逻辑，供当前模块调用。
void CrdtDocument::fromJson(const QJsonObject &obj)
{
    QMutexLocker locker(&m_mutex);

    m_siteId = obj["siteId"].toString();
    m_clock = obj["clock"].toInteger();
    setState(obj["chars"].toArray());
}

// 函数说明：读取 CrdtDocument 当前保存的状态或计算结果。
QJsonArray CrdtDocument::getState() const
{
    QJsonArray arr;
    for (const auto &ch : m_chars) {
        arr.append(ch.toJson());
    }
    return arr;
}

// 函数说明：设置 CrdtDocument 的运行参数，并触发必要的界面或数据刷新。
void CrdtDocument::setState(const QJsonArray &state)
{
    m_chars.clear();
    for (const auto &val : state) {
        m_chars.append(CrdtChar::fromJson(val.toObject()));
    }
    emit textChanged();
}

} // namespace Collaboration

