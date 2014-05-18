#ifndef PARSERMGR_H
#define PARSERMGR_H

#include <cstdlib>
#include <exception>
#include <mutex>

#include <QDebug>

#include <muParser.h>

#include "../globals/scopetracker.h"
#include "../memrep/input.h"
#include "../models/conditionmodel.h"
#include "../models/parammodelbase.h"

//#define DEBUG_PM_FUNC

class ParserMgr
{
    public:
        ParserMgr();
        ~ParserMgr();

        void AddCondModel(ConditionModel* model);
        void AddExpression(const std::string& exprn, bool use_mutex = true);
        void AddModel(ParamModelBase* model);
        bool AreModelsInitialized() const { return _areModelsInitialized; }
        void AssignInput(const ParamModelBase* model, size_t i,
                         const std::string& type_str, bool do_lock);
            //Assign the source of the data for the variables
        void ClearExpressions();
        void ClearModels();
        const double* ConstData(ds::PMODEL pmodel) const;
        void InitParsers();
        void InitModels();
        void InputEval(int idx = -1);
        double Maximum(const ParamModelBase* model, size_t idx) const;
        double Minimum(const ParamModelBase* model, size_t idx) const;
        inline double ModelStep() const { return _modelStep; }
        const std::string& ParserContents() const;
        void ParserEval(bool eval_input = true);
        void ParserEvalAndConds(bool eval_input = true);
        void ParserEvalAndCondsNoLock(bool eval_input = true);
        void QuickEval(const std::string& exprn);
        double Range(const ParamModelBase* model, size_t idx) const;
        void ResetDifferentials();
        void ResetValues();
        void ResetVarInitVals();
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
        std::string AnnotateErrMsg(const std::string& err_mesg, const mu::Parser& parser) const;
        void AssociateVars(mu::Parser& parser);
        inline double* Data(const ParamModelBase* model);
        inline double* Data(ds::PMODEL model);
        inline ParamModelBase* Model(ds::PMODEL model);
        inline double* TempData(const ParamModelBase* model);
        inline double* TempData(ds::PMODEL model);

        bool _areModelsInitialized;
        ConditionModel* _conditions;
        std::vector<Input> _inputs;
        Log* _log;
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
        double* _varInitVals; //For temporary model resets
};

#endif // PARSERMGR_H
