#ifndef CHECKBOXDELEGATE_H
#define CHECKBOXDELEGATE_H

#include <iostream>

#include <QCheckBox>
#include <QDebug>
#include <QPainter>
#include <QStyledItemDelegate>

class CheckBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT
    public:
        explicit CheckBoxDelegate(QColor color, QObject* parent = 0);

        virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                              const QModelIndex& index) const override;
        virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        virtual void setEditorData(QWidget* editor, const QModelIndex& index) const override;
        virtual void setModelData(QWidget* editor, QAbstractItemModel* model,
                          const QModelIndex& index) const override;

        virtual void updateEditorGeometry(QWidget* editor,
            const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    signals:
        void ComboBoxChanged(int value);

    public slots:

    private:
        QColor _color;
};

#endif // CHECKBOXDELEGATE_H
