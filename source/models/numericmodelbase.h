#ifndef NUMERICMODELBASE_H
#define NUMERICMODELBASE_H

#include "parammodelbase.h"

class NumericModelBase : public ParamModelBase
{
    public:
        enum COLUMNS
        {
            MIN = NUM_BASE_COLUMNS,
            MAX,
            NUM_NCOLUMNS
        };

        struct NumParam : ParamModelBase::Param
        {
            static const std::string DEFAULT_MAX, DEFAULT_MIN;
            NumParam()
                : Param(), max(DEFAULT_MAX), min(DEFAULT_MIN)
            {}
            NumParam(const std::string& k)
                : Param(k), max(DEFAULT_MAX), min(DEFAULT_MIN)
            {}
            NumParam(const std::string& k, const std::string& v)
                : Param(k,v), max(DEFAULT_MAX), min(DEFAULT_MIN)
            {}
            std::string max, min;
        };

        NumericModelBase(QObject* parent, const std::string& name);
#ifdef __GNUG__
        NumericModelBase(const NumericModelBase&) = delete;
        NumericModelBase& operator=(const NumericModelBase&) = delete;
#endif
        virtual ~NumericModelBase() override;

        virtual void ProcessParamFileLine(const std::string& key, std::string rem) override;

        virtual void AddParameter(const std::string& key, const std::string& value = "") override;
        void SetMaximum(size_t idx, double val);
        void SetMinimum(size_t idx, double val);
        void SetRange(size_t idx, double min, double max);

        double Maximum(size_t idx) const;
        double Minimum(size_t idx) const;
        virtual std::string String() const override;

        virtual int columnCount(const QModelIndex &parent) const override;
        virtual QVariant data(const QModelIndex &index, int role) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        virtual bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    protected:
        virtual NumParam* Parameter(int row) override
        {
            return static_cast<NumParam*>(ParamModelBase::Parameter(row));
        }
        virtual const NumParam* Parameter(int row) const override
        {
            return static_cast<const NumParam*>(ParamModelBase::Parameter(row));
        }

    private:
        mutable std::mutex _nmutex;
};

#endif // NUMERICMODELBASE_H
