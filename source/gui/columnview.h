#ifndef COLUMNVIEW_H
#define COLUMNVIEW_H

#include <QColumnView>
#include <QListView>

class ColumnView : public QColumnView
{
    Q_OBJECT
    public:
        explicit ColumnView(QWidget *parent = 0);

    protected:
        virtual QAbstractItemView* createColumn(const QModelIndex& rootIndex) override;

    signals:

    public slots:

};

#endif // COLUMNVIEW_H
