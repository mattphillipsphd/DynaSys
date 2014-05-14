#include "comboboxdelegate.h"

ComboBoxDelegate::ComboBoxDelegate(VecStr items, QObject *parent)
    : QStyledItemDelegate(parent), _items(items)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ComboBoxDelegate::ComboBoxDelegate", std::this_thread::get_id());
#endif
}

QWidget* ComboBoxDelegate::createEditor(QWidget* parent,
    const QStyleOptionViewItem&/* option */,
    const QModelIndex&/* index */) const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ComboBoxDelegate::createEditor", std::this_thread::get_id());
#endif
    QComboBox* editor = new QComboBox(parent);
    editor->setFrame(false);
    for (const auto& it : _items)
        editor->addItem(it.c_str());

    editor->setEditable(true);
    editor->setInsertPolicy(QComboBox::InsertAtTop);
//    editor->setMouseTracking(true);

    return editor;
}
/*bool ComboBoxDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
                        const QStyleOptionViewItem& option, const QModelIndex& index)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ComboBoxDelegate::editorEvent", std::this_thread::get_id());
#endif
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}*/
void ComboBoxDelegate::setEditorData(QWidget* editor,
                                    const QModelIndex& index) const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ComboBoxDelegate::setEditorData", std::this_thread::get_id());
#endif
    QString text = index.model()->data(index, Qt::EditRole).toString();

    QComboBox* combo_box = static_cast<QComboBox*>(editor);
    combo_box->setCurrentText(text);
}

void ComboBoxDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                   const QModelIndex& index) const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ComboBoxDelegate::setModelData", std::this_thread::get_id());
#endif
    QComboBox* combo_box = qobject_cast<QComboBox*>(editor);
    QVariant text = combo_box->currentText();

    model->setData(index, text, Qt::EditRole);
    emit ComboBoxChanged((size_t)index.row());
}

void ComboBoxDelegate::updateEditorGeometry(QWidget* editor,
    const QStyleOptionViewItem &option, const QModelIndex& /* index */) const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ComboBoxDelegate::updateEditorGeometry", std::this_thread::get_id());
#endif
    editor->setGeometry(option.rect);
}

