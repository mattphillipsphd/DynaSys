#include "dspinboxdelegate.h"

DSpinBoxDelegate::DSpinBoxDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DSpinBoxDelegate::DSpinBoxDelegate", std::this_thread::get_id());
#endif
}

QWidget* DSpinBoxDelegate::createEditor(QWidget* parent,
    const QStyleOptionViewItem&/* option */,
    const QModelIndex& index) const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DSpinBoxDelegate::createEditor", std::this_thread::get_id());
#endif
    QDoubleSpinBox* editor = new QDoubleSpinBox(parent);
    editor->setEnabled(true);
    editor->setValue(0.0);
    editor->setRange(-1000,1000);
    editor->setSingleStep(1.0);
    editor->setDecimals(1);

    connect(editor, SIGNAL(valueChanged(double)), this, SLOT(ValueChanged(double)));
    _editors[editor] = index;

    return editor;
}
bool DSpinBoxDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
                         const QStyleOptionViewItem& option, const QModelIndex& index)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DSpinBoxDelegate::editorEvent", std::this_thread::get_id());
#endif
    QVariant value = index.data(Qt::EditRole);
    if (!value.isValid())
        return false;

/*    bool result = true;
    // make sure that we have the right event type
    if (event->type()==QEvent::MouseButtonRelease || QEvent::KeyPress)
    {
        result = model->setData(index, value, Qt::EditRole);
        if (result) emit DataChanged();
    }
*/
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
void DSpinBoxDelegate::setEditorData(QWidget* editor,
                                    const QModelIndex& index) const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DSpinBoxDelegate::setEditorData", std::this_thread::get_id());
#endif
    double val = index.model()->data(index, Qt::EditRole).toDouble();

    QDoubleSpinBox* dspinbox = static_cast<QDoubleSpinBox*>(editor);
    dspinbox->setValue(val);
}

void DSpinBoxDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                   const QModelIndex& index) const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DSpinBoxDelegate::setModelData", std::this_thread::get_id());
#endif
    QDoubleSpinBox* dspinbox = qobject_cast<QDoubleSpinBox*>(editor);
    QVariant val = dspinbox->value();

    model->setData(index, val, Qt::EditRole);
}

void DSpinBoxDelegate::updateEditorGeometry(QWidget* editor,
    const QStyleOptionViewItem &option, const QModelIndex& /* index */) const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DSpinBoxDelegate::updateEditorGeometry", std::this_thread::get_id());
#endif
    editor->setGeometry(option.rect);
}

void DSpinBoxDelegate::ValueChanged(double value) //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DSpinBoxDelegate::ValueChanged", std::this_thread::get_id());
#endif
    QObject* editor = sender();
    QModelIndex index = _editors[editor];
    const_cast<QAbstractItemModel*>(index.model())->setData(index, value, Qt::EditRole);
        // ### Need to find a way to do this without the const_cast hack!
    emit DataChanged();
}
