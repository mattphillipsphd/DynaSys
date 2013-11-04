#include "comboboxdelegate.h"

ComboBoxDelegate::ComboBoxDelegate(VecStr items, QObject *parent)
    : _items(items), QStyledItemDelegate(parent), _type(UNKNOWN)
{
}

QWidget* ComboBoxDelegate::createEditor(QWidget* parent,
    const QStyleOptionViewItem&/* option */,
    const QModelIndex&/* index */) const
{
    QComboBox* editor = new QComboBox(parent);
    editor->setFrame(false);
    for (const auto& it : _items)
        editor->addItem(it.c_str());

    editor->setEditable(true);
    editor->setInsertPolicy(QComboBox::InsertAtTop);

//    connect(editor, SIGNAL(currentIndexChanged(int)), this, SIGNAL(ComboBoxChanged(int)));
    connect(editor, SIGNAL(currentTextChanged(const QString&)), this, SIGNAL(ComboBoxChanged(const QString&)));

    std::cerr << "EDITOR CREATED" << std::endl;
    return editor;
}
/*QString ComboBoxDelegate::displayText(const QVariant& value, const QLocale&) const
{
    std::cerr << "Display: " << value.toString().toStdString() << std::endl;
    return _text.c_str();
}*/
/*void ComboBoxDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                       const QModelIndex& index) const
{
    if (index.row() == 6 || index.row() == 7)
    {
        // ohh it's my column
        // better do something creative
    }
    else
        QItemDelegate::paint(painter, option, index);
}*/
void ComboBoxDelegate::setEditorData(QWidget* editor,
                                    const QModelIndex& index) const
{
    QString text = index.model()->data(index, Qt::EditRole).toString();

    QComboBox* combo_box = static_cast<QComboBox*>(editor);
    combo_box->setCurrentText(text);
    std::cerr << "ComboBoxDelegate::setEditorData, " << text.toStdString() << "." << std::endl;
}

void ComboBoxDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                   const QModelIndex& index) const
{
    QComboBox* combo_box = qobject_cast<QComboBox*>(editor);
    QVariant text = combo_box->currentText();

    model->setData(index, text, Qt::EditRole);
    std::cerr << "ComboBoxDelegate::setModelData, " << text.toString().toStdString() << "." << std::endl;
}

void ComboBoxDelegate::updateEditorGeometry(QWidget* editor,
    const QStyleOptionViewItem &option, const QModelIndex& /* index */) const
{
    editor->setGeometry(option.rect);
}
ComboBoxDelegate::TYPE ComboBoxDelegate::Type(const std::string& text)
{
    if (text=="gammarand")
        return GAMMA_RAND;
    if (text=="normrand")
        return NORM_RAND;
    if (text=="unirand")
        return UNI_RAND;

    return USER;
}

void ComboBoxDelegate::SetType(const std::string& text)
{
    _type = Type(text);
}
