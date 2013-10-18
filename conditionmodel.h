#ifndef CONDITIONMODEL_H
#define CONDITIONMODEL_H

#include <QAbstractItemModel>
//#include <QStandardItemModel>

#include <map>
#include <vector>
#include <string>

typedef std::vector<std::string> VecStr;

//Really this is pretty general to any 2-column model
// *** If all else fails redo this with QStandardItemModel, see test-QColumnView
class ConditionModel : public QAbstractItemModel
{
    Q_OBJECT
    public:
        explicit ConditionModel(QObject *parent = 0);

        void AddCondition(const std::string& condition, const VecStr& exprns = VecStr());

        const std::string& Condition(int i) const;
        const VecStr& Expressions(int i) const;

        int columnCount() const;
        virtual int columnCount(const QModelIndex &parent) const override;
        virtual QVariant data(const QModelIndex &index, int role) const override;
        virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
//        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        virtual QModelIndex index(int row, int column,
                                  const QModelIndex &parent = QModelIndex()) const override;
        virtual bool insertRows(int row, int count, const QModelIndex& parent) override;
        virtual QModelIndex parent(const QModelIndex& child) const override;
        virtual bool removeRows(int row, int count, const QModelIndex& parent) override;
        int rowCount() const;
        virtual int rowCount(const QModelIndex &parent) const override;
        virtual bool setData(const QModelIndex &index, const QVariant &value, int role) override;
//        virtual bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role) override;

    signals:

    public slots:

    private:
        std::vector< std::pair<std::string, VecStr > > _conditions;
            //Doing this with a map is not possible because you don't have random access iterators
            //maps.
};

#endif // CONDITIONMODEL_H
