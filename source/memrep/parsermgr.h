#ifndef PARSERMGR_H
#define PARSERMGR_H

#include <exception>
#include <mutex>

#include <QDebug>

#include <muParser.h>

#include "../memrep/input.h"
#include "../models/conditionmodel.h"
#include "../models/parammodelbase.h"

class ParserMgr
{
    public:
        ParserMgr();
        ~ParserMgr();

        void AddCondModel(ConditionModel* model);
        void AddExpression(const std::string& exprn, bool use_mutex = true);
        void AddModel(ParamModelBase* model);
        bool AreModelsInitialized() const { return _areModelsInitialized; }
        void AssignInput(const ParamModelBase* model, size_t i, const std::string& type_str);
            //Assign the source of the data for the variables
        void ClearExpressions();
        void ClearModels();
        const double* ConstData(const ParamModelBase* model) const;
        void InitParsers();
        void InitModels();
        void InputEval(int idx = -1);
        double Maximum(const ParamModelBase* model, size_t idx) const;
        double Minimum(const ParamModelBase* model, size_t idx) const;
        void ParserCondEval();
        void ParserEval(bool eval_input = true);
        void QuickEval(const std::string& exprn);
        double Range(const ParamModelBase* model, size_t idx) const;
        void ResetDifferentials();
        void SetCondModel(ConditionModel* conditions);
        void SetConditions();
        void SetData(const ParamModelBase* model, size_t idx, double val);
            //No range checking
        void SetExpression(const std::string& exprn);
        void SetExpression(const VecStr& exprns);
        void SetExpressions();
        void TempEval();

    private:
        void AssociateVars(mu::Parser& parser);
        double* Data(const ParamModelBase* model);
        ParamModelBase* Model(ds::PMODEL model);
        double* TempData(const ParamModelBase* model);

        bool _areModelsInitialized;
        ConditionModel* _conditions;
        std::vector<Input> _inputs;
        std::vector< std::tuple<ParamModelBase*, double*, double*> > _models;
        mutable std::mutex _mutex;
        mu::Parser _parser, _parserResult;
            //_parserResult is for when conditions get satisfied
        std::vector<mu::Parser> _parserConds;
};

#endif // PARSERMGR_H
