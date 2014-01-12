#ifndef PARSERMGR_H
#define PARSERMGR_H

#include <exception>

#include <QDebug>

#include <muParser.h>

#include "../memrep/input.h"
#include "../models/conditionmodel.h"
#include "../models/parammodel.h"

typedef std::vector<std::string> VecStr;
class ParserMgr
{
    public:
        ParserMgr();
        ~ParserMgr();

        void AddCondModel(ConditionModel* model);
        void AddExpression(const std::string& exprn);
        void AddModel(ParamModel* model);
        void AssignInput(const ParamModel* model, size_t i, const std::string& type_str);
//        void AssignInputs();
            //Assign the source of the data for the variables
        void ClearExpressions();
        void ClearModels();
        const double* ConstData(const ParamModel* model) const;
        void InitParsers();
        void InitVars();
        void InputEval(int idx = -1);
        void ParserCondEval();
        void ParserEval();
        void SetCondModel(ConditionModel* conditions);
        void SetConditions();
        void SetExpression(const std::string& exprn);
        void SetExpression(const VecStr& exprns);
        void SetExpressions();

    private:
        double* Data(const ParamModel* model);
        void DefineVars(mu::Parser& parser);

        ConditionModel* _conditions;
        std::vector<Input> _inputs;
        std::vector< std::pair<ParamModel*, double*> > _models;
        mu::Parser _parser, _parserResult;
            //_parserResult is for when conditions get satisfied
        std::vector<mu::Parser> _parserConds;
};

#endif // PARSERMGR_H
