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
    editor->raise();

    qDebug() << "QCheckBox EDITOR CREATED";
    return editor;
}

void CheckBoxDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->fillRect(option.rect, _color);
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

