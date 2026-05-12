// 文件说明：app-static\rendering\diagramrenderer.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef DIAGRAMRENDERER_H
#define DIAGRAMRENDERER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QPointF>
#include <QRectF>
#include <QDate>
#include <QPainter>
#include <QImage>

class DiagramRenderer : public QObject
{
    Q_OBJECT

public:
    explicit DiagramRenderer(QObject *parent = nullptr);
    ~DiagramRenderer();

    // Diagram types
    enum class DiagramType {
        Unknown,
        Flowchart,
        Sequence,
        Class,
        State,
        Gantt
    };

    // Node shapes for flowchart
    enum class NodeShape {
        Rectangle,
        RoundedRect,
        Diamond,
        Circle,
        Ellipse,
        Hexagon,
        Parallelogram,
        Cylinder,
        Stadium,
        Subroutine,
        Asymmetric
    };

    // Visibility for class members
    enum class Visibility {
        Public,
        Private,
        Protected,
        Package
    };

    // Flowchart structures
    struct FlowNode {
        QString id;
        QString label;
        NodeShape shape;
        QRectF bounds;
    };

    struct FlowEdge {
        QString fromId;
        QString toId;
        QString label;
        bool isDashed;
        QVector<QPointF> points;
    };

    struct Subgraph {
        QString id;
        QString title;
        QStringList nodeIds;
        QRectF bounds;
        QColor backgroundColor;
        int nestingLevel;
    };

    struct Flowchart {
        QVector<FlowNode> nodes;
        QVector<FlowEdge> edges;
        QVector<Subgraph> subgraphs;
        QString direction;
    };

    // Sequence diagram structures
    struct SequenceParticipant {
        QString id;
        QString name;
        qreal x;
    };

    struct SequenceMessage {
        QString fromId;
        QString toId;
        QString label;
        bool isAsync;
        bool isSelfCall;
        qreal y;
    };

    struct ActivationBox {
        QString participantId;
        qreal startY;
        qreal endY;
    };

    struct SequenceNote {
        QString participantId;
        QString text;
        qreal y;
        bool isLeft;
    };

    struct SequenceStructure {
        QString type;  // loop, alt, opt, par, critical, break
        QString condition;
        qreal startY;
        qreal endY;
        QStringList branches;  // For alt/par structures
    };

    struct SequenceDiagram {
        QVector<SequenceParticipant> participants;
        QVector<SequenceMessage> messages;
        QVector<ActivationBox> activations;
        QVector<SequenceNote> notes;
        QVector<SequenceStructure> structures;
    };

    // Class diagram structures
    struct ClassMember {
        QString name;
        QString type;
        Visibility visibility;
        bool isMethod;
        bool isStatic;
        bool isAbstract;
    };

    struct ClassNode {
        QString id;
        QString name;
        QVector<ClassMember> attributes;
        QVector<ClassMember> methods;
        QStringList genericTypes;
        QRectF bounds;
    };

    enum class RelationType {
        Association,
        Inheritance,
        Composition,
        Aggregation,
        Dependency,
        Realization
    };

    struct ClassRelation {
        QString fromId;
        QString toId;
        RelationType type;
        QString label;
        QString fromMultiplicity;
        QString toMultiplicity;
        QVector<QPointF> points;
    };

    struct ClassDiagram {
        QVector<ClassNode> classes;
        QVector<ClassRelation> relations;
    };

    // State diagram structures
    enum class StateType {
        Normal,
        Initial,
        Final,
        Fork,
        Join,
        Choice,
        History,
        DeepHistory
    };

    struct StateNode {
        QString id;
        QString name;
        StateType type;
        QString entryAction;
        QString exitAction;
        QString description;
        QStringList childIds;
        QString parentId;
        bool isComposite;
        QRectF bounds;
    };

    struct StateTransition {
        QString fromId;
        QString toId;
        QString label;
        QVector<QPointF> points;
    };

    struct StateDiagram {
        QVector<StateNode> states;
        QVector<StateTransition> transitions;
    };

    // Gantt chart structures
    enum class TaskStatus {
        Active,
        Done,
        Critical
    };

    struct GanttTask {
        QString id;
        QString name;
        QString section;
        TaskStatus status;
        QDate startDate;
        int duration;
        int progress;
        bool isMilestone;
        QStringList dependencies;
        QRectF bounds;
    };

    struct GanttDiagram {
        QString title;
        QDate startDate;
        QDate endDate;
        QVector<GanttTask> tasks;
        QStringList sections;
    };

    // Main AST structure
    struct DiagramAST {
        DiagramType type;
        bool valid;
        QString errorMessage;
        
        Flowchart flowchart;
        SequenceDiagram sequence;
        ClassDiagram classDiagram;
        StateDiagram stateDiagram;
        GanttDiagram gantt;
    };

    // Main API
    DiagramAST parse(const QString &code);
    QImage render(const DiagramAST &diagram, const QSize &size = QSize(800, 600));

    // Specific parsers
    DiagramAST parseFlowchart(const QString &code);
    DiagramAST parseSequenceDiagram(const QString &code);
    DiagramAST parseClassDiagram(const QString &code);
    DiagramAST parseStateDiagram(const QString &code);
    DiagramAST parseGanttChart(const QString &code);

    // Specific renderers
    void renderFlowchart(QPainter &painter, const DiagramAST &diagram);
    void renderSequenceDiagram(QPainter &painter, const DiagramAST &diagram);
    void renderClassDiagram(QPainter &painter, const DiagramAST &diagram);
    void renderStateDiagram(QPainter &painter, const DiagramAST &diagram);
    void renderGanttChart(QPainter &painter, DiagramAST &diagram);

private:
    // Layout helpers
    void layoutFlowchart(DiagramAST &diagram);
    void layoutSequenceDiagram(DiagramAST &diagram);
    void layoutClassDiagram(DiagramAST &diagram);
    void layoutStateDiagram(DiagramAST &diagram);
    void layoutGanttChart(DiagramAST &diagram);

    // Render helpers
    void renderFlowNode(QPainter &painter, const FlowNode &node);
    void renderClassNode(QPainter &painter, const ClassNode &classNode);
    void renderStateNode(QPainter &painter, const StateNode &state);
    void renderGanttTask(QPainter &painter, const GanttTask &task, const GanttDiagram &gantt);

    // Utility
    GanttTask* findGanttTask(GanttDiagram &gantt, const QString &id);
    QDate parseDate(const QString &dateStr, const QDate &baseDate);
    DiagramType detectDiagramType(const QString &code);
};

#endif // DIAGRAMRENDERER_H

