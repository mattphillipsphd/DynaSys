#ifndef COMBOBOXDELEGATE_H
#define COMBOBOXDELEGATE_H

#include <QComboBox>
#include <QEvent>
#include <QStyledItemDelegate>

#include <iostream>
#include <vector>
#include <string>

#include "../globals/scopetracker.h"
#include "../memrep/input.h"

typedef std::vector<std::string> VecStr;
class ComboBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT

    public:

        explicit ComboBoxDelegate(VecStr items, QObject *parent = 0);

        virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                              const QModelIndex& index) const override;
//        virtual bool editorEvent(QEvent* event, QAbstractItemModel* model,
//                                const QStyleOptionViewItem& option, const QModelIndex& index) override;
        virtual void setEditorData(QWidget* editor, const QModelIndex& index) const override;
        virtual void setModelData(QWidget* editor, QAbstractItemModel* model,
                          const QModelIndex& index) const override;

        virtual void updateEditorGeometry(QWidget* editor,
            const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    signals:
        void ComboBoxChanged(size_t value) const;

    private:
        const VecStr _items;
        std::string _text;
};

#endif // COMBOBOXDELEGATE_H
