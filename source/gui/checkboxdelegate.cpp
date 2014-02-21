#include "checkboxdelegate.h"

CheckBoxDelegate::CheckBoxDelegate(QColor color, QObject *parent) :
    _color(color), QStyledItemDelegate(parent)
{
}

QWidget* CheckBoxDelegate::createEditor(QWidget* parent,
    const QStyleOptionViewItem&/* option */,
    const QModelIndex&/* index */) const
{
    QCheckBox* editor = new QCheckBox(parent);
    editor->setEnabled(true);
    editor->setCheckable(true);
    editor->raise();

    qDebug() << "QCheckBox EDITOR CREATED";
    return editor;
}

bool CheckBoxDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
                         const QStyleOptionViewItem& option, const QModelIndex& index)
{

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
        const int textMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
        QRect checkRect = QStyle::alignedRect(option.direction, Qt::AlignCenter,
                                              option.decorationSize,
                                              QRect(option.rect.x() + (2 * textMargin), option.rect.y(),
                                                    option.rect.width() - (2 * textMargin),
                                                    option.rect.height()));
//        if (!checkRect.contains(static_cast<QMouseEvent*>(event)->pos()))
//            return false;
    }
    else if (event->type() == QEvent::KeyPress)
    {
        if (static_cast<QKeyEvent*>(event)->key() != Qt::Key_Space&& static_cast<QKeyEvent*>(event)->key() != Qt::Key_Select)
            return false;
    }
    else
        return false;
    Qt::CheckState state = value.toBool() ? Qt::Unchecked : Qt::Checked;
    return model->setData(index, state, Qt::CheckStateRole);
}

void CheckBoxDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->fillRect(option.rect, _color);
    QStyleOptionViewItem my_option(option);
    my_option.showDecorationSelected = false;
    QStyledItemDelegate::paint(painter, my_option, index);
}

void CheckBoxDelegate::setEditorData(QWidget* editor,
                                    const QModelIndex& index) const
{
    bool val = index.model()->data(index, Qt::DisplayRole).toBool();

    QCheckBox* checkbox = static_cast<QCheckBox*>(editor);
    checkbox->setPalette( QPalette(_color) );
    checkbox->setChecked(val);
    qDebug() << "CheckBoxDelegate::setEditorData, " << val << ".";
}

void CheckBoxDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                   const QModelIndex& index) const
{
    QCheckBox* checkbox = qobject_cast<QCheckBox*>(editor);
    QVariant val = checkbox->isChecked();

    model->setData(index, val, Qt::EditRole);
    qDebug() << "CheckBoxDelegate::setModelData, " << val.toBool() << ".";
}

void CheckBoxDelegate::updateEditorGeometry(QWidget* editor,
    const QStyleOptionViewItem &option, const QModelIndex& /* index */) const
{
    editor->setGeometry(option.rect);
}

