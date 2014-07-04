#ifndef PARSERMGR_H
#define PARSERMGR_H

#include <cstdlib>
#include <exception>
#include <mutex>

#include <QDebug>

#include <muParser.h>

#include "modelmgr.h"
#include "../globals/scopetracker.h"

//#define DEBUG_PM_FUNC

class ParserMgr
{
    public:
        ParserMgr();
        ParserMgr(const ParserMgr& other);
#ifdef __GNUG__
        ParserMgr& operator=(const ParserMgr& other) = delete;
#endif
        ~ParserMgr();

        void AddExpression(const std::string& exprn, bool use_mutex = true);
        void ClearExpressions();
        void InitParsers(); //Associates parsers with the data variables and sets the expressions
        void InitData(); //Initializes the data variables to their appropriate initial values
        void InputEval(int idx = -1);
        const std::string& ParserContents() const;
        void ParserEval(bool eval_input = true);
        void ParserEvalAndConds(bool eval_input = true);
        void ParserEvalAndCondsNoLock(bool eval_input = true);
        void QuickEval(const std::string& exprn);
        void ResetDifferentials();
        void ResetValues();
        void ResetVarInitVals();
        void TempEval();

        void SetConditions();
        void SetData(ds::PMODEL mi, size_t idx, double val);
            //No range checking
        void SetExpression(const std::string& exprn);
        void SetExpression(const VecStr& exprns);
        void SetExpressions();
        void SetModelStep(double step);

        const double* ConstData(ds::PMODEL mi) const;
        inline double* Data(ds::PMODEL model); // ### Wish there was a better way...

    private:
        void AllocModelData();
        std::string AnnotateErrMsg(const std::string& err_mesg, const mu::Parser& parser) const;
        void AssociateVars(mu::Parser& parser);
        void ClearModelData();
        void DeepCopy(const ParserMgr& other);
        inline double* TempData(const ParamModelBase* model);
        inline double* TempData(ds::PMODEL model);

        std::vector<Input> _inputs;
        int _inputsPerUnitTime; // ### Needs to be done separately for each input
        Log* const _log;
        ModelMgr* const _modelMgr;
        std::vector< std::pair<double*, double*> > _modelData;
            //Model evaluation happens in a two-step process so that all variables and differentials
            //can be updated simultaneously; the third element is a temporary that is used for
            //this purpose.
        mutable std::mutex _mutex;
        mu::Parser _parser, _parserResult;
            //_parserResult is for when conditions get satisfied
        std::vector<mu::Parser> _parserConds;
        int _stepCt;
        double* _varInitVals; //For temporary model resets
};

#endif // PARSERMGR_H
