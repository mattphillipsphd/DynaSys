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
        inline double ModelStep() const { return _modelStep; }
        void ParserCondEval();
        void ParserEval(bool eval_input = true);
        void QuickEval(const std::string& exprn);
        double Range(const ParamModelBase* model, size_t idx) const;
        void ResetDifferentials();
//        void ResetValues();
//        void ResetVarInitVals();
        void SetCondModel(ConditionModel* conditions);
        void SetConditions();
        void SetData(const ParamModelBase* model, size_t idx, double val);
            //No range checking
        void SetExpression(const std::string& exprn);
        void SetExpression(const VecStr& exprns);
        void SetExpressions();
        void SetModelStep(double step);
        void TempEval();

    private:
        void AssociateVars(mu::Parser& parser);
        inline double* Data(const ParamModelBase* model);
        inline ParamModelBase* Model(ds::PMODEL model);
        inline double* TempData(const ParamModelBase* model);

        bool _areModelsInitialized;
        ConditionModel* _conditions;
        std::vector<Input> _inputs;
        std::vector< std::tuple<ParamModelBase*, double*, double*> > _models;
            //Model evaluation happens in a two-step process so that all variables and differentials
            //can be updated simultaneously; the third element is a temporary that is used for
            //this purpose.
        double _modelStep;
        mutable std::mutex _mutex;
        mu::Parser _parser, _parserResult;
            //_parserResult is for when conditions get satisfied
        std::vector<mu::Parser> _parserConds;
        int _stepCt;
        std::vector<double> _varInitVals; //For temporary model resets
};

#endif // PARSERMGR_H
