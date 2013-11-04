#ifndef COMBOBOXDELEGATE_H
#define COMBOBOXDELEGATE_H

#include <QStyledItemDelegate>
#include <QComboBox>

#include <iostream>
#include <vector>
#include <string>

typedef std::vector<std::string> VecStr;
class ComboBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT

    public:
        enum TYPE
        {
            UNKNOWN = -1,
            GAMMA_RAND,
            NORM_RAND,
            UNI_RAND,
            USER
        };

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

        void SetType(const std::string& text);
        void SetType(TYPE type) { _type = type; }

        TYPE Type() const { return _type; }

    signals:
        void ComboBoxChanged(int value);
        void ComboBoxChanged(const QString& text);

    private:
        const VecStr _items;
        std::string _text;
        TYPE _type;
};

#endif // COMBOBOXDELEGATE_H
