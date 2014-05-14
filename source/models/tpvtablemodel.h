#ifndef TPVTABLEMODEL_H
#define TPVTABLEMODEL_H

#include <thread>

#include <QAbstractTableModel>

#include "../globals/globals.h"
#include "../globals/scopetracker.h"

//#define DEBUG_TP_FUNC

class TPVTableModel : public QAbstractTableModel
{
    Q_OBJECT
    public:
        enum COLUMNS
        {
            SHOW = 0,
            SCALE,
            NUM_COLUMNS
        };

        explicit TPVTableModel(const VecStr& names, QObject *parent = 0);

        VecStr IsEnabled() const;
        bool IsEnabled(int idx) const;
        double LogScale(int idx) const;
        std::string Name(int idx) const;

        int columnCount() const;
        virtual int columnCount(const QModelIndex &parent) const override;
        virtual QVariant data(const QModelIndex &index, int role) const override;
        virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        int rowCount() const;
        virtual int rowCount(const QModelIndex &parent) const override;
        virtual bool setData(const QModelIndex &index, const QVariant &value, int role) override;
        virtual bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role) override;

    signals:

    public slots:

    private:
        struct RowRecord //Expand this to include min/max etc. as needed
        {
            RowRecord(const std::string& name_, bool en = false, double lscale = 0.0)
                : name(name_), is_enabled(en), log_scale(lscale)
            {}
            const std::string name;
            bool is_enabled;
            double log_scale;
        };

        std::vector<RowRecord> _data;
        VecStr _headers;
};

#endif // TPVTABLEMODEL_H
