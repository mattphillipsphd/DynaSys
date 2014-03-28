#ifndef COMBOBOXDELEGATE_H
#define COMBOBOXDELEGATE_H

#include <QStyledItemDelegate>
#include <QComboBox>

#include <iostream>
#include <vector>
#include <string>

#include "../memrep/input.h"

typedef std::vector<std::string> VecStr;
class ComboBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT

    public:

        explicit ComboBoxDelegate(VecStr items, QObject *parent = 0);

        virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                              const QModelIndex& index) const override;
//        virtual QString displayText(const QVariant& value, const QLocale& locale) const override;
//        virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        virtual void setEditorData(QWidget* editor, const QModelIndex& index) const override;
        virtual void setModelData(QWidget* editor, QAbstractItemModel* model,
                          const QModelIndex& index) const override;

        virtual void updateEditorGeometry(QWidget* editor,
            const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    signals:
        void ComboBoxChanged(int value);
        void ComboBoxChanged(const QString& text);

    private:
        const VecStr _items;
        std::string _text;
};

#endif // COMBOBOXDELEGATE_H
