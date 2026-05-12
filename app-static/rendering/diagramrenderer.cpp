// 文件说明：app-static\rendering\diagramrenderer.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "diagramrenderer.h"
#include "renderingstylemanager.h"

#include <QRegularExpression>
#include <QPainterPath>
#include <QFontMetrics>
#include <QtMath>

// 函数说明：构造 DiagramRenderer 对象，初始化本模块需要的状态、界面和资源。
DiagramRenderer::DiagramRenderer(QObject *parent)
    : QObject(parent)
{
}

DiagramRenderer::~DiagramRenderer() = default;

// 函数说明：实现 DiagramRenderer::detectDiagramType 的核心逻辑，供当前模块调用。
DiagramRenderer::DiagramType DiagramRenderer::detectDiagramType(const QString &code)
{
    QString trimmed = code.trimmed().toLower();
    
    if (trimmed.startsWith("graph") || trimmed.startsWith("flowchart")) {
        return DiagramType::Flowchart;
    } else if (trimmed.startsWith("sequencediagram") || trimmed.startsWith("sequence")) {
        return DiagramType::Sequence;
    } else if (trimmed.startsWith("classdiagram") || trimmed.startsWith("class")) {
        return DiagramType::Class;
    } else if (trimmed.startsWith("statediagram") || trimmed.startsWith("state")) {
        return DiagramType::State;
    } else if (trimmed.startsWith("gantt")) {
        return DiagramType::Gantt;
    }
    
    return DiagramType::Unknown;
}

// 函数说明：解析输入内容，转换为 DiagramRenderer 后续处理使用的数据结构。
DiagramRenderer::DiagramAST DiagramRenderer::parse(const QString &code)
{
    DiagramType type = detectDiagramType(code);
    
    switch (type) {
        case DiagramType::Flowchart:
            return parseFlowchart(code);
        case DiagramType::Sequence:
            return parseSequenceDiagram(code);
        case DiagramType::Class:
            return parseClassDiagram(code);
        case DiagramType::State:
            return parseStateDiagram(code);
        case DiagramType::Gantt:
            return parseGanttChart(code);
        default:
            DiagramAST ast;
            ast.type = DiagramType::Unknown;
            ast.valid = false;
            ast.errorMessage = "Unknown diagram type";
            return ast;
    }
}

// 函数说明：渲染 DiagramRenderer 的显示内容或导出片段。
QImage DiagramRenderer::render(const DiagramAST &diagram, const QSize &size)
{
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(RenderingStyleManager::instance().colorScheme().backgroundColor);
    
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    
    DiagramAST mutableDiagram = diagram;
    
    switch (diagram.type) {
        case DiagramType::Flowchart:
            renderFlowchart(painter, mutableDiagram);
            break;
        case DiagramType::Sequence:
            renderSequenceDiagram(painter, mutableDiagram);
            break;
        case DiagramType::Class:
            renderClassDiagram(painter, mutableDiagram);
            break;
        case DiagramType::State:
            renderStateDiagram(painter, mutableDiagram);
            break;
        case DiagramType::Gantt:
            renderGanttChart(painter, mutableDiagram);
            break;
        default:
            break;
    }
    
    painter.end();
    return image;
}

// ==================== GANTT CHART ====================

DiagramRenderer::DiagramAST DiagramRenderer::parseGanttChart(const QString &code)
{
    DiagramAST diagram;
    diagram.type = DiagramType::Gantt;
    diagram.valid = true;
    
    QStringList lines = code.split('\n');
    QString currentSection;
    QDate baseDate = QDate::currentDate();
    
    for (const QString &rawLine : lines) {
        QString line = rawLine.trimmed();
        
        if (line.isEmpty() || line.startsWith("%%")) {
            continue;
        }
        
        // Skip gantt declaration
        if (line.toLower().startsWith("gantt")) {
            continue;
        }
        
        // Parse title
        if (line.startsWith("title ")) {
            diagram.gantt.title = line.mid(6).trimmed();
            continue;
        }
        
        // Parse dateFormat (ignored for now, we use ISO)
        if (line.startsWith("dateFormat ")) {
            continue;
        }
        
        // Parse section
        if (line.startsWith("section ")) {
            currentSection = line.mid(8).trimmed();
            if (!diagram.gantt.sections.contains(currentSection)) {
                diagram.gantt.sections.append(currentSection);
            }
            continue;
        }
        
        // Parse task with full format: TaskName :status, progress%, id, start, duration
        QRegularExpression taskRegex("^([^:]+)\\s*:\\s*(\\w+)\\s*(?:,\\s*(\\d+)%)?\\s*,\\s*(\\w+)\\s*,\\s*([^,]+)\\s*,\\s*(\\d+)d");
        QRegularExpressionMatch taskMatch = taskRegex.match(line);
        
        if (taskMatch.hasMatch()) {
            GanttTask task;
            task.name = taskMatch.captured(1).trimmed();
            task.section = currentSection;
            task.progress = 0;
            task.isMilestone = false;
            
            QString statusStr = taskMatch.captured(2).toLower();
            if (statusStr == "done") {
                task.status = TaskStatus::Done;
            } else if (statusStr == "crit" || statusStr == "critical") {
                task.status = TaskStatus::Critical;
            } else if (statusStr == "milestone") {
                task.status = TaskStatus::Active;
                task.isMilestone = true;
            } else {
                task.status = TaskStatus::Active;
            }
            
            // Parse progress percentage if present
            QString progressStr = taskMatch.captured(3);
            if (!progressStr.isEmpty()) {
                task.progress = progressStr.toInt();
            }
            
            task.id = taskMatch.captured(4).trimmed();
            
            // Parse start date and check for dependencies
            QString startStr = taskMatch.captured(5).trimmed();
            if (startStr.startsWith("after ")) {
                QString depId = startStr.mid(6).trimmed();
                task.dependencies.append(depId);
                task.startDate = baseDate;
            } else {
                task.startDate = parseDate(startStr, baseDate);
            }
            
            // Parse duration
            task.duration = taskMatch.captured(6).toInt();
            
            // Milestones have 0 duration
            if (task.isMilestone) {
                task.duration = 0;
            }

            diagram.gantt.tasks.append(task);
            continue;
        }
        
        // Simplified task format: TaskName :status, start, duration
        QRegularExpression simpleTaskRegex("^([^:]+)\\s*:\\s*(\\w+)\\s*(?:,\\s*(\\d+)%)?\\s*,\\s*([^,]+)\\s*,\\s*(\\d+)d");
        QRegularExpressionMatch simpleMatch = simpleTaskRegex.match(line);
        
        if (simpleMatch.hasMatch()) {
            GanttTask task;
            task.name = simpleMatch.captured(1).trimmed();
            task.id = task.name.toLower().replace(" ", "_");
            task.section = currentSection;
            task.progress = 0;
            task.isMilestone = false;
            
            QString statusStr = simpleMatch.captured(2).toLower();
            if (statusStr == "done") {
                task.status = TaskStatus::Done;
            } else if (statusStr == "crit" || statusStr == "critical") {
                task.status = TaskStatus::Critical;
            } else if (statusStr == "milestone") {
                task.status = TaskStatus::Active;
                task.isMilestone = true;
            } else {
                task.status = TaskStatus::Active;
            }
            
            // Parse progress percentage if present
            QString progressStr = simpleMatch.captured(3);
            if (!progressStr.isEmpty()) {
                task.progress = progressStr.toInt();
            }
            
            // Parse start date and check for dependencies
            QString startStr = simpleMatch.captured(4).trimmed();
            if (startStr.startsWith("after ")) {
                QString depId = startStr.mid(6).trimmed();
                task.dependencies.append(depId);
                task.startDate = baseDate;
            } else {
                task.startDate = parseDate(startStr, baseDate);
            }
            
            // Parse duration
            task.duration = simpleMatch.captured(5).toInt();
            
            // Milestones have 0 duration
            if (task.isMilestone) {
                task.duration = 0;
            }
            
            diagram.gantt.tasks.append(task);
            continue;
        }
    }
    
    // Resolve dependencies - set start dates based on dependent tasks
    bool changed = true;
    int iterations = 0;
    while (changed && iterations < 100) {
        changed = false;
        iterations++;
        
        for (GanttTask &task : diagram.gantt.tasks) {
            if (!task.dependencies.isEmpty()) {
                QDate latestEnd = task.startDate;
                for (const QString &depId : task.dependencies) {
                    GanttTask *depTask = findGanttTask(diagram.gantt, depId);
                    if (depTask) {
                        QDate depEnd = depTask->startDate.addDays(depTask->duration);
                        if (depEnd > latestEnd) {
                            latestEnd = depEnd;
                        }
                    }
                }
                if (latestEnd != task.startDate) {
                    task.startDate = latestEnd;
                    changed = true;
                }
            }
        }
    }
    
    if (diagram.gantt.tasks.isEmpty()) {
        diagram.valid = false;
        diagram.errorMessage = "No tasks found in Gantt chart";
    } else {
        // Calculate date range
        diagram.gantt.startDate = diagram.gantt.tasks.first().startDate;
        diagram.gantt.endDate = diagram.gantt.tasks.first().startDate.addDays(diagram.gantt.tasks.first().duration);
        
        for (const GanttTask &task : diagram.gantt.tasks) {
            if (task.startDate < diagram.gantt.startDate) {
                diagram.gantt.startDate = task.startDate;
            }
            QDate taskEnd = task.startDate.addDays(task.duration);
            if (taskEnd > diagram.gantt.endDate) {
                diagram.gantt.endDate = taskEnd;
            }
        }
    }
    
    return diagram;
}

// 函数说明：实现 DiagramRenderer::layoutGanttChart 的核心逻辑，供当前模块调用。
void DiagramRenderer::layoutGanttChart(DiagramAST &diagram)
{
    const qreal leftMargin = 150;
    const qreal topMargin = 60;
    const qreal taskHeight = 25;
    const qreal taskSpacing = 8;
    const qreal dayWidth = 30;
    
    int totalDays = diagram.gantt.startDate.daysTo(diagram.gantt.endDate);
    if (totalDays <= 0) totalDays = 1;
    
    qreal currentY = topMargin;
    
    for (GanttTask &task : diagram.gantt.tasks) {
        int startOffset = diagram.gantt.startDate.daysTo(task.startDate);
        qreal x = leftMargin + startOffset * dayWidth;
        qreal width = task.duration * dayWidth;
        
        if (task.isMilestone) {
            width = taskHeight;
        }
        
        task.bounds = QRectF(x, currentY, qMax(width, 10.0), taskHeight);
        currentY += taskHeight + taskSpacing;
    }
}

// 函数说明：渲染 DiagramRenderer 的显示内容或导出片段。
void DiagramRenderer::renderGanttChart(QPainter &painter, DiagramAST &diagram)
{
    layoutGanttChart(diagram);
    
    const auto &style = RenderingStyleManager::instance();
    const auto colors = style.colorScheme();
    
    painter.save();
    
    const qreal leftMargin = 150;
    const qreal topMargin = 60;
    const qreal taskHeight = 25;
    const qreal taskSpacing = 8;
    const qreal dayWidth = 30;
    
    // Draw title
    if (!diagram.gantt.title.isEmpty()) {
        QFont titleFont = style.fontSettings().diagramFont;
        titleFont.setBold(true);
        titleFont.setPointSize(14);
        painter.setFont(titleFont);
        painter.setPen(colors.nodeText);
        painter.drawText(QRectF(0, 10, leftMargin + 400, 30), Qt::AlignCenter, diagram.gantt.title);
    }
    
    // Draw timeline header
    int totalDays = diagram.gantt.startDate.daysTo(diagram.gantt.endDate);
    QFont headerFont = style.fontSettings().diagramLabelFont;
    painter.setFont(headerFont);
    
    for (int i = 0; i <= totalDays; ++i) {
        QDate date = diagram.gantt.startDate.addDays(i);
        qreal x = leftMargin + i * dayWidth;
        
        // Draw date label (show every few days depending on total)
        int step = totalDays > 14 ? 7 : (totalDays > 7 ? 3 : 1);
        if (i % step == 0) {
            painter.setPen(colors.nodeText);
            painter.drawText(QRectF(x - 15, topMargin - 25, 50, 20), 
                           Qt::AlignCenter, date.toString("MM/dd"));
        }
        
        // Draw grid line
        QPen gridPen(colors.nodeBorder);
        gridPen.setStyle(Qt::DotLine);
        gridPen.setWidthF(0.5);
        painter.setPen(gridPen);
        painter.drawLine(QPointF(x, topMargin), 
                        QPointF(x, topMargin + diagram.gantt.tasks.size() * (taskHeight + taskSpacing)));
    }
    
    // Draw tasks
    qreal currentY = topMargin;
    for (const GanttTask &task : diagram.gantt.tasks) {
        // Draw task name
        QFont taskFont = style.fontSettings().diagramLabelFont;
        painter.setFont(taskFont);
        painter.setPen(colors.nodeText);
        painter.drawText(QRectF(20, task.bounds.y(), leftMargin - 30, taskHeight), 
                       Qt::AlignRight | Qt::AlignVCenter, task.name);
        
        // Draw task bar
        renderGanttTask(painter, task, diagram.gantt);
        
        currentY = task.bounds.bottom() + taskSpacing;
    }
    
    // Draw dependency arrows
    for (const GanttTask &task : diagram.gantt.tasks) {
        for (const QString &depId : task.dependencies) {
            const GanttTask *depTask = nullptr;
            for (const GanttTask &t : diagram.gantt.tasks) {
                if (t.id == depId) {
                    depTask = &t;
                    break;
                }
            }
            
            if (depTask && !depTask->bounds.isEmpty() && !task.bounds.isEmpty()) {
                QPointF fromPoint(depTask->bounds.right(), depTask->bounds.center().y());
                QPointF toPoint(task.bounds.left(), task.bounds.center().y());
                
                QPen depPen(colors.edgeColor, 1.5);
                depPen.setStyle(Qt::DashLine);
                painter.setPen(depPen);
                
                if (qAbs(fromPoint.y() - toPoint.y()) < 5) {
                    painter.drawLine(fromPoint, toPoint);
                } else {
                    qreal stepX = fromPoint.x() + 15;
                    painter.drawLine(fromPoint, QPointF(stepX, fromPoint.y()));
                    painter.drawLine(QPointF(stepX, fromPoint.y()), QPointF(stepX, toPoint.y()));
                    painter.drawLine(QPointF(stepX, toPoint.y()), toPoint);
                }
                
                // Draw arrow head
                const qreal arrowSize = 6;
                QPointF p1 = toPoint + QPointF(-arrowSize, -arrowSize/2);
                QPointF p2 = toPoint + QPointF(-arrowSize, arrowSize/2);
                
                QPainterPath arrowPath;
                arrowPath.moveTo(toPoint);
                arrowPath.lineTo(p1);
                arrowPath.lineTo(p2);
                arrowPath.closeSubpath();
                
                painter.fillPath(arrowPath, colors.edgeColor);
            }
        }
    }
    
    painter.restore();
}

// 函数说明：渲染 DiagramRenderer 的显示内容或导出片段。
void DiagramRenderer::renderGanttTask(QPainter &painter, const GanttTask &task, const GanttDiagram &gantt)
{
    const auto &style = RenderingStyleManager::instance();
    const auto colors = style.colorScheme();
    
    painter.save();
    
    QColor fillColor;
    QColor borderColor;
    
    switch (task.status) {
        case TaskStatus::Done:
            fillColor = colors.successColor;
            borderColor = fillColor.darker(120);
            break;
        case TaskStatus::Critical:
            fillColor = colors.errorColor;
            borderColor = fillColor.darker(120);
            break;
        default:
            fillColor = colors.highlightColor;
            borderColor = fillColor.darker(120);
            break;
    }
    
    if (task.isMilestone) {
        // Draw diamond for milestone
        QPainterPath diamond;
        qreal cx = task.bounds.center().x();
        qreal cy = task.bounds.center().y();
        qreal size = task.bounds.height() / 2;
        
        diamond.moveTo(cx, cy - size);
        diamond.lineTo(cx + size, cy);
        diamond.lineTo(cx, cy + size);
        diamond.lineTo(cx - size, cy);
        diamond.closeSubpath();
        
        painter.fillPath(diamond, fillColor);
        painter.setPen(QPen(borderColor, 1.5));
        painter.drawPath(diamond);
    } else {
        // Draw task bar
        QPainterPath barPath;
        barPath.addRoundedRect(task.bounds, 4, 4);
        
        painter.fillPath(barPath, fillColor);
        painter.setPen(QPen(borderColor, 1));
        painter.drawPath(barPath);
        
        // Draw progress if > 0
        if (task.progress > 0 && task.progress <= 100) {
            qreal progressWidth = task.bounds.width() * task.progress / 100.0;
            QRectF progressRect(task.bounds.x(), task.bounds.y(), 
                              progressWidth, task.bounds.height());
            
            QPainterPath progressPath;
            progressPath.addRoundedRect(progressRect, 4, 4);
            
            QColor progressColor = fillColor.darker(130);
            painter.fillPath(progressPath, progressColor);
        }
    }
    
    // Draw progress text if present
    if (task.progress > 0 && !task.isMilestone) {
        QFont progressFont = style.fontSettings().diagramLabelFont;
        progressFont.setPointSize(9);
        painter.setFont(progressFont);
        painter.setPen(borderColor);
        painter.drawText(task.bounds, Qt::AlignCenter, QString("%1%").arg(task.progress));
    }
    
    painter.restore();
}

// 函数说明：实现 DiagramRenderer::findGanttTask 的核心逻辑，供当前模块调用。
DiagramRenderer::GanttTask* DiagramRenderer::findGanttTask(GanttDiagram &gantt, const QString &id)
{
    for (GanttTask &task : gantt.tasks) {
        if (task.id == id) {
            return &task;
        }
    }
    return nullptr;
}

// 函数说明：解析输入内容，转换为 DiagramRenderer 后续处理使用的数据结构。
QDate DiagramRenderer::parseDate(const QString &dateStr, const QDate &baseDate)
{
    // Try various date formats
    QStringList formats = {
        "yyyy-MM-dd",    // ISO: 2024-01-15
        "dd-MM-yyyy",    // European: 15-01-2024
        "MM/dd/yyyy",    // US: 01/15/2024
        "dd/MM/yyyy",    // European slashes: 15/01/2024
        "yyyy/MM/dd",    // ISO slashes: 2024/01/15
        "d MMM yyyy",    // 15 Jan 2024
        "MMM d, yyyy"    // Jan 15, 2024
    };
    
    for (const QString &format : formats) {
        QDate date = QDate::fromString(dateStr, format);
        if (date.isValid()) {
            return date;
        }
    }
    
    // after dependencies handled in caller
    if (dateStr.startsWith("after ")) {
        return baseDate;
    }
    
    // Try relative days: +3d, +1w, +2m
    QRegularExpression relativeRegex("^\\+(\\d+)([dwm])$");
    QRegularExpressionMatch match = relativeRegex.match(dateStr);
    if (match.hasMatch()) {
        int value = match.captured(1).toInt();
        QString unit = match.captured(2);
        
        if (unit == "d") {
            return baseDate.addDays(value);
        } else if (unit == "w") {
            return baseDate.addDays(value * 7);
        } else if (unit == "m") {
            return baseDate.addMonths(value);
        }
    }
    
    // Default to base date
    return baseDate;
}

// ==================== STUB IMPLEMENTATIONS ====================
// These are minimal stubs - full implementations would be much larger

DiagramRenderer::DiagramAST DiagramRenderer::parseFlowchart(const QString &code)
{
    DiagramAST diagram;
    diagram.type = DiagramType::Flowchart;
    diagram.valid = true;
    // Minimal implementation
    return diagram;
}

// 函数说明：解析输入内容，转换为 DiagramRenderer 后续处理使用的数据结构。
DiagramRenderer::DiagramAST DiagramRenderer::parseSequenceDiagram(const QString &code)
{
    DiagramAST diagram;
    diagram.type = DiagramType::Sequence;
    diagram.valid = true;
    // Minimal implementation
    return diagram;
}

// 函数说明：解析输入内容，转换为 DiagramRenderer 后续处理使用的数据结构。
DiagramRenderer::DiagramAST DiagramRenderer::parseClassDiagram(const QString &code)
{
    DiagramAST diagram;
    diagram.type = DiagramType::Class;
    diagram.valid = true;
    // Minimal implementation
    return diagram;
}

// 函数说明：解析输入内容，转换为 DiagramRenderer 后续处理使用的数据结构。
DiagramRenderer::DiagramAST DiagramRenderer::parseStateDiagram(const QString &code)
{
    DiagramAST diagram;
    diagram.type = DiagramType::State;
    diagram.valid = true;
    // Minimal implementation
    return diagram;
}

// 函数说明：实现 DiagramRenderer::layoutFlowchart 的核心逻辑，供当前模块调用。
void DiagramRenderer::layoutFlowchart(DiagramAST &diagram) {}
// 函数说明：实现 DiagramRenderer::layoutSequenceDiagram 的核心逻辑，供当前模块调用。
void DiagramRenderer::layoutSequenceDiagram(DiagramAST &diagram) {}
// 函数说明：实现 DiagramRenderer::layoutClassDiagram 的核心逻辑，供当前模块调用。
void DiagramRenderer::layoutClassDiagram(DiagramAST &diagram) {}
// 函数说明：实现 DiagramRenderer::layoutStateDiagram 的核心逻辑，供当前模块调用。
void DiagramRenderer::layoutStateDiagram(DiagramAST &diagram) {}

// 函数说明：渲染 DiagramRenderer 的显示内容或导出片段。
void DiagramRenderer::renderFlowchart(QPainter &painter, const DiagramAST &diagram) {}
// 函数说明：渲染 DiagramRenderer 的显示内容或导出片段。
void DiagramRenderer::renderSequenceDiagram(QPainter &painter, const DiagramAST &diagram) {}
// 函数说明：渲染 DiagramRenderer 的显示内容或导出片段。
void DiagramRenderer::renderClassDiagram(QPainter &painter, const DiagramAST &diagram) {}
// 函数说明：渲染 DiagramRenderer 的显示内容或导出片段。
void DiagramRenderer::renderStateDiagram(QPainter &painter, const DiagramAST &diagram) {}

// 函数说明：渲染 DiagramRenderer 的显示内容或导出片段。
void DiagramRenderer::renderFlowNode(QPainter &painter, const FlowNode &node) {}
// 函数说明：渲染 DiagramRenderer 的显示内容或导出片段。
void DiagramRenderer::renderClassNode(QPainter &painter, const ClassNode &classNode) {}
// 函数说明：渲染 DiagramRenderer 的显示内容或导出片段。
void DiagramRenderer::renderStateNode(QPainter &painter, const StateNode &state) {}

