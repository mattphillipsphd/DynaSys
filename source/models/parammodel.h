#ifndef PARAMMODEL_H
#define PARAMMODEL_H

#include <QAbstractTableModel>

#include <vector>
#include <string>
#include <tuple>
#include <mutex>

typedef std::vector<std::string> VecStr;
typedef std::pair<std::string, std::string> StrPair;
class ParamModel : public QAbstractTableModel
{
    Q_OBJECT
    public:
        explicit ParamModel(QObject* parent, const std::string& name);
//        ParamModel(const ParamModel&) = delete;
//        ParamModel& operator=(const ParamModel&) = delete;
        virtual ~ParamModel();

        virtual VecStr Expressions() const { return VecStr(); }
        virtual VecStr Initializations() const { return VecStr(); }
        const std::string& Key(size_t i) const;
        const std::string& Name() const { return _name; }
        size_t NumPars() const { return _parameters.size(); }
        virtual std::string ShortKey(size_t i) const;
        const std::string& Value(const std::string& key) const;
        const std::string& Value(size_t i) const;

        void AddParameter(const std::string& key, const std::string& value = "");
        virtual void SetPar(const std::string& key, const std::string& value);
        virtual void SetPar(int i, const std::string& value);

        int columnCount() const;
        virtual int columnCount(const QModelIndex &parent) const override;
        virtual QVariant data(const QModelIndex &index, int role) const override;
        virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        virtual bool insertRows(int row, int count, const QModelIndex& parent) override;
        virtual bool removeRows(int row, int count, const QModelIndex& parent) override;
        int rowCount() const;
        virtual int rowCount(const QModelIndex &parent) const override;
        virtual bool setData(const QModelIndex &index, const QVariant &value, int role) override;
        virtual bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role) override;

//    signals:

//    protected:

    private:
        int Index(const std::string& par_name) const;

        std::mutex _mutex;
        const std::string _name;
        std::vector<StrPair> _parameters;
};

#endif // PARAMMODEL_H
