// 文件说明：app\optionsdialog.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "optionsdialog.h"
#include "ui_optionsdialog.h"

#include <QFontComboBox>
#include <QItemEditorFactory>
#include <QKeySequence>
#include <QKeySequenceEdit>
#include <QMessageBox>
#include <QStyledItemDelegate>
#include <QTableWidgetItem>

#include <snippets/snippetcollection.h>
#include "options.h"
#include "snippetstablemodel.h"

class KeySequenceTableItem : public QTableWidgetItem
{
public:
    KeySequenceTableItem (const QKeySequence &keySequence) : 
        QTableWidgetItem(QTableWidgetItem::UserType+1),
        m_keySequence(keySequence)
    {
    }

    QVariant data(int role) const
    {
        switch (role) {
            case Qt::DisplayRole:
                return m_keySequence.toString();
            case Qt::EditRole:
                return m_keySequence;
            default:
                return QVariant();
        }
    }

    void setData(int role, const QVariant &data)
    {
        if (role == Qt::EditRole)
            m_keySequence = data.value<QKeySequence>();

        QTableWidgetItem::setData(role, data);
    }

private:
    QKeySequence m_keySequence;
};

class KeySequenceEditFactory : public QItemEditorCreatorBase
{
public:
    QWidget *createWidget(QWidget *parent) const
    {
        return new QKeySequenceEdit(parent);
    }

    QByteArray valuePropertyName() const
    {
        return "keySequence";
    }
};


// 函数说明：构造 OptionsDialog 对象，初始化本模块需要的状态、界面和资源。
OptionsDialog::OptionsDialog(Options *opt, SnippetCollection *collection, const QList<QAction*> &acts, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OptionsDialog),
    options(opt),
    snippetCollection(collection),
    actions(acts)
{
    ui->setupUi(this);

    ui->tabWidget->setIconSize(QSize(24, 24));
    ui->tabWidget->setTabIcon(0, QIcon("fa-cog.fontawesome"));
    ui->tabWidget->setTabIcon(1, QIcon("fa-file-text-o.fontawesome"));
    ui->tabWidget->setTabIcon(2, QIcon("fa-html5.fontawesome"));
    ui->tabWidget->setTabIcon(3, QIcon("fa-globe.fontawesome"));
    ui->tabWidget->setTabIcon(4, QIcon("fa-puzzle-piece.fontawesome"));
    ui->tabWidget->setTabIcon(5, QIcon("fa-keyboard-o.fontawesome"));

    for (int size : QFontDatabase::standardSizes()) {
        ui->sizeComboBox->addItem(QString().setNum(size));
        ui->defaultSizeComboBox->addItem(QString().setNum(size));
        ui->defaultFixedSizeComboBox->addItem(QString().setNum(size));
    }

    ui->portLineEdit->setValidator(new QIntValidator(0, 65535));
    ui->passwordLineEdit->setEchoMode(QLineEdit::Password);

    ui->snippetTableView->setModel(new SnippetsTableModel(snippetCollection, ui->snippetTableView));
    connect(ui->snippetTableView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &OptionsDialog::currentSnippetChanged);

    connect(ui->snippetTextEdit, &QPlainTextEdit::textChanged,
            this, &OptionsDialog::snippetTextChanged);

    // remove hoedown item from converter combo box, if hoedown is disabled
#ifndef ENABLE_HOEDOWN
    ui->converterComboBox->removeItem(1);
#endif

    setupShortcutsTable();

    // read configuration state
    readState();
}

// 函数说明：销毁 OptionsDialog 对象，释放本模块持有的资源。
OptionsDialog::~OptionsDialog()
{
    delete ui;
}

// 函数说明：实现 OptionsDialog::done 的核心逻辑，供当前模块调用。
void OptionsDialog::done(int result)
{
    if (result == QDialog::Accepted) {
        // save configuration state
        saveState();
    }

    QDialog::done(result);
}

// 函数说明：实现 OptionsDialog::manualProxyRadioButtonToggled 的核心逻辑，供当前模块调用。
void OptionsDialog::manualProxyRadioButtonToggled(bool checked)
{
    ui->hostLineEdit->setEnabled(checked);
    ui->portLineEdit->setEnabled(checked);
    ui->userNameLineEdit->setEnabled(checked);
    ui->passwordLineEdit->setEnabled(checked);
}

// 函数说明：实现 OptionsDialog::currentSnippetChanged 的核心逻辑，供当前模块调用。
void OptionsDialog::currentSnippetChanged(const QModelIndex &current, const QModelIndex &)
{
    const Snippet snippet = snippetCollection->at(current.row());

    // update text edit for snippet content
    QString formattedSnippet(snippet.snippet);
    formattedSnippet.insert(snippet.cursorPosition, "$|");
    ui->snippetTextEdit->setPlainText(formattedSnippet);
    ui->snippetTextEdit->setReadOnly(snippet.builtIn);

    // disable remove button when built-in snippet is selected
    ui->removeSnippetButton->setEnabled(!snippet.builtIn);
}

// 函数说明：实现 OptionsDialog::snippetTextChanged 的核心逻辑，供当前模块调用。
void OptionsDialog::snippetTextChanged()
{
    const QModelIndex &modelIndex = ui->snippetTableView->selectionModel()->currentIndex();
    if (modelIndex.isValid()) {
        Snippet snippet = snippetCollection->at(modelIndex.row());
        if (!snippet.builtIn) {
            snippet.snippet = ui->snippetTextEdit->toPlainText();

            // find cursor marker
            int pos = snippet.snippet.indexOf("$|");
            if (pos >= 0) {
                snippet.cursorPosition = pos;
                snippet.snippet.remove(pos, 2);
            }

            snippetCollection->update(snippet);
        }
    }
}

// 函数说明：向 OptionsDialog 管理的数据集合中添加一项内容。
void OptionsDialog::addSnippetButtonClicked()
{
    SnippetsTableModel *snippetModel = qobject_cast<SnippetsTableModel*>(ui->snippetTableView->model());

    const QModelIndex &index = snippetModel->createSnippet();

    const int row = index.row();
    QModelIndex topLeft = snippetModel->index(row, 0, QModelIndex());
    QModelIndex bottomRight = snippetModel->index(row, 1, QModelIndex());
    QItemSelection selection(topLeft, bottomRight);
    ui->snippetTableView->selectionModel()->select(selection, QItemSelectionModel::SelectCurrent);
    ui->snippetTableView->setCurrentIndex(topLeft);
    ui->snippetTableView->scrollTo(topLeft);

    ui->snippetTableView->edit(index);
}

// 函数说明：从 OptionsDialog 管理的数据集合中移除指定内容。
void OptionsDialog::removeSnippetButtonClicked()
{
    const QModelIndex &modelIndex = ui->snippetTableView->selectionModel()->currentIndex();
    if (!modelIndex.isValid()) {
        QMessageBox::critical(0, tr("Error", "Title of error message box"), tr("No snippet selected."));
        return;
    }

    SnippetsTableModel *snippetModel = qobject_cast<SnippetsTableModel*>(ui->snippetTableView->model());
    snippetModel->removeSnippet(modelIndex);
}

// 函数说明：实现 OptionsDialog::validateShortcut 的核心逻辑，供当前模块调用。
void OptionsDialog::validateShortcut(int row, int column)
{
    // Check changes to shortcut column only
    if (column != 1)
        return;

    QString newShortcut = ui->shortcutsTable->item(row, column)->text();
    QKeySequence ks(newShortcut);
    if (ks.isEmpty() && !newShortcut.isEmpty()) {
        // If new shortcut was invalid, restore the original
        ui->shortcutsTable->setItem(row, column,
            new QTableWidgetItem(actions[row]->shortcut().toString()));
    } else {
        // Check for conflicts.
        if (!ks.isEmpty()) {
            for (int c = 0; c < actions.size(); ++c) {
                if (c != row && ks == QKeySequence(ui->shortcutsTable->item(c, 1)->text())) {
                    ui->shortcutsTable->setItem(row, column,
                        new QTableWidgetItem(actions[row]->shortcut().toString()));
                    QMessageBox::information(this, tr("Conflict"), tr("This shortcut is already used for \"%1\"").arg(actions[c]->text().remove('&')));
                    return;
                }
            }
        }
        // If the new shortcut is not the same as the default, make the
        // action label bold.
        QFont font = ui->shortcutsTable->item(row, 0)->font();
        font.setBold(ks != actions[row]->property("defaultshortcut").value<QKeySequence>());
        ui->shortcutsTable->item(row, 0)->setFont(font);
    }
}

// 函数说明：初始化 OptionsDialog 的 setupShortcutsTable 相关界面、动作或服务连接。
void OptionsDialog::setupShortcutsTable()
{
    QStyledItemDelegate *delegate = new QStyledItemDelegate(ui->shortcutsTable);
    QItemEditorFactory *factory = new QItemEditorFactory();
    factory->registerEditor(QVariant::nameToType("QKeySequence"), new KeySequenceEditFactory());
    delegate->setItemEditorFactory(factory);
    ui->shortcutsTable->setItemDelegateForColumn(1, delegate);

    ui->shortcutsTable->setRowCount(actions.size());

    int i = 0;
    for (QAction *action : actions) {
        QTableWidgetItem *label = new QTableWidgetItem(action->text().remove('&'));
        label->setFlags(Qt::ItemIsSelectable);
        const QKeySequence &defaultKeySeq = action->property("defaultshortcut").value<QKeySequence>();
        if (action->shortcut() != defaultKeySeq) {
            QFont font = label->font();
            font.setBold(true);
            label->setFont(font);
        }
        QTableWidgetItem *accel = new KeySequenceTableItem(action->shortcut());
        QTableWidgetItem *def = new QTableWidgetItem(defaultKeySeq.toString());
        def->setFlags(Qt::ItemIsSelectable);
        ui->shortcutsTable->setItem(i, 0, label);
        ui->shortcutsTable->setItem(i, 1, accel);
        ui->shortcutsTable->setItem(i, 2, def);
        ++i;
    }

    ui->shortcutsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->shortcutsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->shortcutsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    connect(ui->shortcutsTable, &QTableWidget::cellChanged,
            this, &OptionsDialog::validateShortcut);
}

// 函数说明：读取 OptionsDialog 的配置或数据，并同步到运行时状态。
void OptionsDialog::readState()
{
    // general settings
    ui->converterComboBox->setCurrentIndex(options->markdownConverter());

    // editor settings
    QFont font = options->editorFont();
    ui->fontComboBox->setCurrentFont(font);
    ui->sizeComboBox->setCurrentText(QString().setNum(font.pointSize()));
    ui->sourceSingleSizedCheckBox->setChecked(options->isSourceAtSingleSizeEnabled());
    ui->tabWidthSpinBox->setValue(options->tabWidth());
    ui->lineColumnCheckBox->setChecked(options->isLineColumnEnabled());
    ui->rulerEnableCheckBox->setChecked(options->isRulerEnabled());
    ui->rulerPosSpinBox->setValue(options->rulerPos());

    // html preview settings
    ui->standardFontComboBox->setCurrentFont(options->standardFont());
    ui->defaultSizeComboBox->setCurrentText(QString().setNum(options->defaultFontSize()));
    ui->serifFontComboBox->setCurrentFont(options->serifFont());
    ui->sansSerifFontComboBox->setCurrentFont(options->sansSerifFont());
    ui->fixedFontComboBox->setCurrentFont(options->fixedFont());
    ui->defaultFixedSizeComboBox->setCurrentText(QString().setNum(options->defaultFixedFontSize()));
    ui->mathInlineCheckBox->setChecked(options->isMathInlineSupportEnabled());
    ui->mathSupportCheckBox->setChecked(options->isMathSupportEnabled());

    // proxy settings
    switch (options->proxyMode()) {
    case Options::NoProxy:
        ui->noProxyRadioButton->setChecked(true);
        break;
    case Options::SystemProxy:
        ui->systemProxyRadioButton->setChecked(true);
        break;
    case Options::ManualProxy:
        ui->manualProxyRadioButton->setChecked(true);
        break;
    }
    ui->hostLineEdit->setText(options->proxyHost());
    ui->portLineEdit->setText(QString::number(options->proxyPort()));
    ui->userNameLineEdit->setText(options->proxyUser());
    ui->passwordLineEdit->setText(options->proxyPassword());

    // shortcut settings
    for (int i = 0; i < ui->shortcutsTable->rowCount(); ++i) {
        if (options->hasCustomShortcut(actions[i]->objectName())) {
            ui->shortcutsTable->item(i, 1)->setData(Qt::EditRole, options->customShortcut(actions[i]->objectName()));
        }
    }
}

// 函数说明：保存 OptionsDialog 当前状态，保证用户修改可以持久化。
void OptionsDialog::saveState()
{
    // general settings
    options->setMarkdownConverter((Options::MarkdownConverter)ui->converterComboBox->currentIndex());

    // editor settings
    QFont font = ui->fontComboBox->currentFont();
    font.setPointSize(ui->sizeComboBox->currentText().toInt());
    options->setEditorFont(font);
    options->setSourceAtSingleSizeEnabled(ui->sourceSingleSizedCheckBox->isChecked());
    options->setTabWidth(ui->tabWidthSpinBox->value());
    options->setLineColumnEnabled(ui->lineColumnCheckBox->isChecked());
    options->setRulerEnabled(ui->rulerEnableCheckBox->isChecked());
    options->setRulerPos(ui->rulerPosSpinBox->value());
    options->setMathInlineSupportEnabled(ui->mathInlineCheckBox->isChecked());
    options->setMathSupportEnabled(ui->mathSupportCheckBox->isChecked());

    // html preview settings
    options->setStandardFont(ui->standardFontComboBox->currentFont());
    options->setDefaultFontSize(ui->defaultSizeComboBox->currentText().toInt());
    options->setSerifFont(ui->serifFontComboBox->currentFont());
    options->setSansSerifFont(ui->sansSerifFontComboBox->currentFont());
    options->setFixedFont(ui->fixedFontComboBox->currentFont());
    options->setDefaultFixedFontSize(ui->defaultFixedSizeComboBox->currentText().toInt());

    // proxy settings
    if (ui->noProxyRadioButton->isChecked()) {
        options->setProxyMode(Options::NoProxy);
    } else if (ui->systemProxyRadioButton->isChecked()) {
        options->setProxyMode(Options::SystemProxy);
    } else if (ui->manualProxyRadioButton->isChecked()) {
        options->setProxyMode(Options::ManualProxy);
    }
    options->setProxyHost(ui->hostLineEdit->text());
    options->setProxyPort(ui->portLineEdit->text().toInt());
    options->setProxyUser(ui->userNameLineEdit->text());
    options->setProxyPassword(ui->passwordLineEdit->text());

    // shortcut settings
    for (int i = 0; i < ui->shortcutsTable->rowCount(); ++i) {
        QKeySequence customKeySeq(ui->shortcutsTable->item(i, 1)->text());
        options->addCustomShortcut(actions[i]->objectName(), customKeySeq);
    }
    
    options->apply();
}


