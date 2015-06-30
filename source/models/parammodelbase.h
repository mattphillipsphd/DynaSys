#ifndef PARAMMODELBASE_H
#define PARAMMODELBASE_H

#include <QAbstractTableModel>
#include <QDebug>

#include <vector>
#include <string>
#include <tuple>
#include <mutex>

#include "../globals/globals.h"

//Value is the string contents of the Value field.
//Expression is the statement formed from the key and value, e.g. 'a = 5'.

typedef std::pair<std::string, std::string> StrPair;
class ParamModelBase : public QAbstractTableModel
{
    Q_OBJECT
    public:
        enum COLUMNS
        {
            FREEZE = 0,
            VALUE,
            NUM_BASE_COLUMNS
        };

        struct Param
        {
            static const std::string DEFAULT_VAL;
            Param()
                : key(""), value(DEFAULT_VAL), is_freeze(false)
            {}
            Param(const std::string& k)
                : key(k), value(DEFAULT_VAL), is_freeze(false)
            {}
            Param(const std::string& k, const std::string& v)
                : key(k), value(v), is_freeze(false)
            {}
            std::string key, value;
            bool is_freeze;
        };

        static ParamModelBase* Create(ds::PMODEL mi);

        explicit ParamModelBase(QObject* parent, const std::string& name);
#ifdef __GNUG__
        ParamModelBase(const ParamModelBase&) = delete;
        ParamModelBase& operator=(const ParamModelBase&) = delete;
#endif
        virtual ~ParamModelBase();

        virtual bool DoEvaluate() const = 0; //Whether the parameter expression
            //gets evaluated by the parser (on each cycle)
        virtual bool DoInitialize() const = 0; //Whether the model values
            //are used to initialize the variables
        virtual std::string Expression(size_t i) const;
        std::string ExpressionList() const;
        virtual VecStr Expressions() const;
        ds::PMODEL Id() const { return _id; }
        virtual VecStr Initializations() const { return VecStr(); }
        bool IsFreeze(size_t idx) const;
        virtual std::string Key(size_t i) const;
        int KeyIndex(const std::string& par_name) const;
        VecStr Keys() const;
        std::string Name() const;
        size_t NumPars() const { return _parameters.size(); }
        virtual std::string ParamString(size_t i) const;
        virtual void ProcessParamFileLine(const std::string& key, std::string rem) = 0;
        virtual void SaveString(std::ofstream& out) const;
        virtual std::string ShortKey(size_t i) const;
        virtual int ShortKeyIndex(const std::string& par_name) const;
        VecStr ShortKeys() const;
        virtual std::string String() const = 0;
        virtual std::string TempExpression(size_t i) const;
        virtual std::string TempExprnForCFile(size_t i) const;
        virtual VecStr TempExpressions() const;
        std::string TempKey(size_t i) const;
        const std::string& Value(const std::string& key) const;
        const std::string& Value(size_t i) const;
        VecStr Values() const;

        virtual void AddParameter(const std::string& param, const std::string& value = "");
        void SetFreeze(int i, bool is_freeze);
        void SetValue(const std::string& param, const std::string& value);
        void SetValue(int i, const std::string& value);

        int columnCount() const;
        virtual int columnCount(const QModelIndex &parent) const override = 0;
        virtual QVariant data(const QModelIndex &index, int role) const override;
        virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        virtual bool insertRows(int row, int count, const QModelIndex& parent) override;
        virtual bool removeRows(int row, int count, const QModelIndex& parent) override;
        virtual int rowCount() const;
        virtual int rowCount(const QModelIndex &parent) const override;
        virtual bool setData(const QModelIndex &index, const QVariant &value, int role) override;
        virtual bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role) override;

    protected:
        void SetParameter(int row, Param* parameter) { _parameters[row] = parameter; }
        virtual Param* Parameter(int row) { return _parameters[row]; }
        virtual const Param* Parameter(int row) const { return _parameters.at(row); }


    private:
        const ds::PMODEL _id;
        mutable std::mutex _mutex;
        std::vector<Param*> _parameters;
};

#endif // PARAMMODELBASE_H
