#include "checkboxdelegate.h"

CheckBoxDelegate::CheckBoxDelegate(const std::vector<QColor>& colors, QObject *parent) :
     QStyledItemDelegate(parent), _colors(colors), _log(Log::Instance())
{
}

QWidget* CheckBoxDelegate::createEditor(QWidget* parent,
    const QStyleOptionViewItem&/* option */,
    const QModelIndex& index) const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("CheckBoxDelegate::createEditor", std::this_thread::get_id());
#endif
    QCheckBox* editor = new QCheckBox(parent);
    editor->setEnabled(true);
    editor->setCheckable(true);

    QPalette palette = editor->palette();
    palette.setColor(QPalette::Window, _colors.at(index.row()%_colors.size()));
    editor->setPalette(palette);
    editor->setAutoFillBackground(true);

    editor->raise();

    return editor;
}

bool CheckBoxDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
                         const QStyleOptionViewItem& option, const QModelIndex& index)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("CheckBoxDelegate::editorEvent", std::this_thread::get_id());
#endif
    // make sure that the item is checkable
    Qt::ItemFlags flags = model->flags(index);
    if (!(flags & Qt::ItemIsUserCheckable))
        return false;
    if (!(flags & Qt::ItemIsEnabled))
        return false;
    // make sure that we have a check state
    QVariant value = index.data(Qt::CheckStateRole);
    if (!value.isValid())
        return false;
    // make sure that we have the right event type
    if (event->type() == QEvent::MouseButtonRelease)
    {
/*        const int textMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
        QRect checkRect = QStyle::alignedRect(option.direction, Qt::AlignCenter,
                                              option.decorationSize,
                                              QRect(option.rect.x() + (2 * textMargin), option.rect.y(),
                                                    option.rect.width() - (2 * textMargin),
                                                    option.rect.height()));
*/    }
    else if (event->type() == QEvent::KeyPress)
    {
        if (static_cast<QKeyEvent*>(event)->key() != Qt::Key_Space&& static_cast<QKeyEvent*>(event)->key() != Qt::Key_Select)
            return false;
    }
    else
        return false;
    Qt::CheckState state = value.toBool() ? Qt::Unchecked : Qt::Checked;
    bool result = model->setData(index, state, Qt::CheckStateRole);

    if (result && event->type()==QEvent::MouseButtonRelease)
        emit MouseReleased();

    return result;
}

void CheckBoxDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
//#ifdef DEBUG_FUNC
//    ScopeTracker st("CheckBoxDelegate::paint", std::this_thread::get_id());
//#endif
    painter->fillRect(option.rect, _colors.at(index.row()%_colors.size()));
    QStyleOptionViewItem my_option(option);
    my_option.showDecorationSelected = false;
    QStyledItemDelegate::paint(painter, my_option, index);
}

void CheckBoxDelegate::setEditorData(QWidget* editor,
                                    const QModelIndex& index) const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("CheckBoxDelegate::setEditorData", std::this_thread::get_id());
#endif
    bool val = index.model()->data(index, Qt::DisplayRole).toBool();

    QCheckBox* checkbox = static_cast<QCheckBox*>(editor);
    checkbox->setPalette( QPalette(_colors.at(index.row()%_colors.size())) );
    checkbox->setChecked(val);
}

void CheckBoxDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                   const QModelIndex& index) const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("CheckBoxDelegate::setModelData", std::this_thread::get_id());
#endif
    QCheckBox* checkbox = qobject_cast<QCheckBox*>(editor);
    QVariant val = checkbox->isChecked();

    model->setData(index, val, Qt::EditRole);
}

void CheckBoxDelegate::updateEditorGeometry(QWidget* editor,
    const QStyleOptionViewItem &option, const QModelIndex& /* index */) const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("CheckBoxDelegate::UpdateTimePlotTable", std::this_thread::get_id());
#endif
    editor->setGeometry(option.rect);
}

