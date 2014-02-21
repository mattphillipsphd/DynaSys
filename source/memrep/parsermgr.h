#ifndef PARSERMGR_H
#define PARSERMGR_H

#include <exception>

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
        void AddExpression(const std::string& exprn);
        void AddModel(ParamModelBase* model);
        void AssignInput(const ParamModelBase* model, size_t i, const std::string& type_str);
//        void AssignInputs();
            //Assign the source of the data for the variables
        void ClearExpressions();
        void ClearModels();
        const double* ConstData(const ParamModelBase* model) const;
        void InitParsers();
        void InitModels();
        void InputEval(int idx = -1);
        void ParserCondEval();
        void ParserEval();
        void QuickEval(const std::string& exprn);
        void SetCondModel(ConditionModel* conditions);
        void SetConditions();
        void SetExpression(const std::string& exprn);
        void SetExpression(const VecStr& exprns);
        void SetExpressions();

    private:
        void AssociateVars(mu::Parser& parser);
        double* Data(const ParamModelBase* model);

        ConditionModel* _conditions;
        std::vector<Input> _inputs;
        std::vector< std::pair<ParamModelBase*, double*> > _models;
        mu::Parser _parser, _parserResult;
            //_parserResult is for when conditions get satisfied
        std::vector<mu::Parser> _parserConds;
};

#endif // PARSERMGR_H
