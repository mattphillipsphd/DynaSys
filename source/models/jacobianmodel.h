#ifndef JACOBIANMODEL_H
#define JACOBIANMODEL_H

#include "parammodelbase.h"

//Each 'parameter' in this case is the gradient vector for the corresponding equation
class JacobianModel : public ParamModelBase
{
    Q_OBJECT

    public:
        JacobianModel(QObject *parent, const std::string& name);
        virtual ~JacobianModel() override;

        virtual bool DoEvaluate() const override { return false; }
        virtual bool DoInitialize() const { return false; }
        virtual void ProcessParamFileLine(const std::string& key, std::string rem) override;

        virtual std::string String() const override;

        virtual int columnCount() const;
        virtual int columnCount(const QModelIndex &parent) const override;
        virtual QVariant data(const QModelIndex &index, int role) const override;
        virtual bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    private:
        static const std::string DELIM;
};

#endif // JACOBIANMODEL_H
