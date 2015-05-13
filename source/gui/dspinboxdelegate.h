#ifndef DSPINBOXDELEGATE_H
#define DSPINBOXDELEGATE_H

#include <map>
#include <thread>

#include <QApplication>
#include <QDoubleSpinBox>
#include <QDebug>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyledItemDelegate>

#include "../globals/log.h"
#include "../globals/scopetracker.h"

class DSpinBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT
    public:
        explicit DSpinBoxDelegate(QObject* parent = 0);

        virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                              const QModelIndex& index) const override;
        virtual bool editorEvent(QEvent* event, QAbstractItemModel* model,
                                 const QStyleOptionViewItem& option, const QModelIndex& index) override;
        virtual void setEditorData(QWidget* editor, const QModelIndex& index) const override;
        virtual void setModelData(QWidget* editor, QAbstractItemModel* model,
                          const QModelIndex& index) const override;

        virtual void updateEditorGeometry(QWidget* editor,
            const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    signals:
        void DataChanged();

    public slots:

    private slots:
        void ValueChanged(double value);

    private:
        mutable std::map<QObject*, QModelIndex> _editors;
};

#endif // DSPINBOXDELEGATE_H
