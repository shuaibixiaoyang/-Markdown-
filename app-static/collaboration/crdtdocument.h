// 文件说明：app-static\collaboration\crdtdocument.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef CRDTDOCUMENT_H
#define CRDTDOCUMENT_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include <QMutex>
#include <QUuid>

/**
 * @brief CRDT (Conflict-free Replicated Data Type) document model
 *
 * Implements RGA (Replicated Growable Array) algorithm for collaborative text editing.
 * Each character has a unique identifier that ensures consistent ordering across all sites.
 */

namespace Collaboration {

// Unique identifier for each character in the document
struct CharId {
    QString siteId;      // Unique site/user identifier
    qint64 clock;        // Lamport clock for ordering
    qint64 offset;       // Position offset for same clock values

    bool isNull() const { return siteId.isEmpty() && clock == 0; }

    bool operator<(const CharId &other) const {
        if (clock != other.clock) return clock < other.clock;
        if (siteId != other.siteId) return siteId < other.siteId;
        return offset < other.offset;
    }

    bool operator==(const CharId &other) const {
        return siteId == other.siteId && clock == other.clock && offset == other.offset;
    }

    bool operator!=(const CharId &other) const {
        return !(*this == other);
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["siteId"] = siteId;
        obj["clock"] = clock;
        obj["offset"] = offset;
        return obj;
    }

    static CharId fromJson(const QJsonObject &obj) {
        CharId id;
        id.siteId = obj["siteId"].toString();
        id.clock = obj["clock"].toInteger();
        id.offset = obj["offset"].toInteger();
        return id;
    }
};

// A character node in the CRDT document
struct CrdtChar {
    CharId id;           // Unique identifier
    QChar value;         // The actual character
    CharId parentId;     // ID of the character this was inserted after
    bool deleted;        // Tombstone marker for deleted characters

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id.toJson();
        obj["value"] = QString(value);
        obj["parentId"] = parentId.toJson();
        obj["deleted"] = deleted;
        return obj;
    }

    static CrdtChar fromJson(const QJsonObject &obj) {
        CrdtChar ch;
        ch.id = CharId::fromJson(obj["id"].toObject());
        QString v = obj["value"].toString();
        ch.value = v.isEmpty() ? QChar() : v[0];
        ch.parentId = CharId::fromJson(obj["parentId"].toObject());
        ch.deleted = obj["deleted"].toBool();
        return ch;
    }
};

// Operation types for synchronization
enum class OperationType {
    Insert,
    Delete
};

struct Operation {
    OperationType type;
    CrdtChar character;  // For insert
    CharId targetId;     // For delete

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["type"] = (type == OperationType::Insert) ? "insert" : "delete";
        if (type == OperationType::Insert) {
            obj["character"] = character.toJson();
        } else {
            obj["targetId"] = targetId.toJson();
        }
        return obj;
    }

    static Operation fromJson(const QJsonObject &obj) {
        Operation op;
        op.type = (obj["type"].toString() == "insert") ? OperationType::Insert : OperationType::Delete;
        if (op.type == OperationType::Insert) {
            op.character = CrdtChar::fromJson(obj["character"].toObject());
        } else {
            op.targetId = CharId::fromJson(obj["targetId"].toObject());
        }
        return op;
    }
};

class CrdtDocument : public QObject
{
    Q_OBJECT

public:
    explicit CrdtDocument(QObject *parent = nullptr);
    explicit CrdtDocument(const QString &siteId, QObject *parent = nullptr);

    // Site management
    void setSiteId(const QString &siteId);
    QString siteId() const { return m_siteId; }

    // Document operations
    Operation localInsert(int position, QChar character);
    Operation localDelete(int position);
    void remoteInsert(const CrdtChar &character);
    void remoteDelete(const CharId &targetId);

    // Apply operation (for undo/redo and remote sync)
    void applyOperation(const Operation &op);

    // Get current document text
    QString text() const;
    int length() const;

    // Position conversion
    int crdtToLocal(const CharId &id) const;
    CharId localToCrdt(int position) const;

    // Serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &obj);

    // Full document sync
    QJsonArray getState() const;
    void setState(const QJsonArray &state);

signals:
    void textChanged();
    void operationGenerated(const Operation &op);
    void remoteOperationApplied(int position, int charsRemoved, int charsAdded);

private:
    CharId generateId();
    int findInsertPosition(const CrdtChar &ch) const;
    int findCharIndex(const CharId &id) const;
    void insertChar(const CrdtChar &ch);

    QString m_siteId;
    qint64 m_clock;
    QVector<CrdtChar> m_chars;  // All characters including tombstones
    mutable QMutex m_mutex;
};

} // namespace Collaboration

#endif // CRDTDOCUMENT_H

