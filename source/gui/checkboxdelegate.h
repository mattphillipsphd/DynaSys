#ifndef CHECKBOXDELEGATE_H
#define CHECKBOXDELEGATE_H

#include <thread>

#include <QApplication>
#include <QCheckBox>
#include <QDebug>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyledItemDelegate>

#include "../globals/log.h"
#include "../globals/scopetracker.h"

class CheckBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT
    public:
        explicit CheckBoxDelegate(const std::vector<QColor>& colors, QObject* parent = 0);

        virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                              const QModelIndex& index) const override;
        virtual bool editorEvent(QEvent* event, QAbstractItemModel* model,
                                 const QStyleOptionViewItem& option, const QModelIndex& index) override;
        virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        virtual void setEditorData(QWidget* editor, const QModelIndex& index) const override;
        virtual void setModelData(QWidget* editor, QAbstractItemModel* model,
                          const QModelIndex& index) const override;

        virtual void updateEditorGeometry(QWidget* editor,
            const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    signals:
        void MouseReleased();

    public slots:

    private:
        const std::vector<QColor>&  _colors;
        Log* _log;
};

#endif // CHECKBOXDELEGATE_H
