#include "parsermgr.h"

ParserMgr::ParserMgr()
{
}
ParserMgr::~ParserMgr()
{
    ClearModels();
}

void ParserMgr::AddExpression(const std::string& exprn)
{
    if (exprn.empty()) return;
    std::string old_exprn = _parser.GetExpr();
    old_exprn.erase( old_exprn.find_last_not_of(" ")+1 );
    _parser.SetExpr(old_exprn.empty() ? "" : old_exprn + ", "
                     + exprn );
}
void ParserMgr::AddModel(ParamModel* model, double* data)
{
    if (!data) data = new double[model->NumPars()];
    _models.push_back( std::make_pair(model, data) );
}
void ParserMgr::AssignInput(const ParamModel* model, size_t i, const std::string& type_str)
{
    Input::TYPE type = Input::Type(type_str);
    double* data = Data(model);
    switch (type)
    {
        case Input::UNI_RAND:
        case Input::GAMMA_RAND:
        case Input::NORM_RAND:
        {
            Input input(&data[i]);
            input.GenerateInput(type);
            _inputs.push_back(input);
            break;
        }
        case Input::TXT_FILE:
        {
            Input input(&data[i]);
            std::string file_name = model->Value(i);
            std::remove_if(file_name.begin(), file_name.end(), [&](std::string::value_type c)
            {
                return c=='"';
            });
            input.LoadInput(file_name);
            _inputs.push_back(input);
            break;
        }
        case Input::USER:
            break;
        case Input::UNKNOWN:
            throw("Error, ParserMgr::SetInput");
    }
}
/*void ParserMgr::AssignInputs()
{
    for (auto& it : _models)
    {
        ParamModel* model = it.first;
        const size_t num_pars = model->NumPars();
        for (size_t i=0; i<num_pars; ++i)
        {
            const std::string& value = model->Value(i);
            AssignInput(model, i, value);
        }
    }
}*/
void ParserMgr::ClearExpressions()
{
    _parser.SetExpr("");
}
void ParserMgr::ClearModels()
{
    for (auto it : _models)
        delete it.second;
    _models.clear();
}

double* ParserMgr::Data(const ParamModel* model)
{
    auto it = std::find_if(_models.cbegin(), _models.cend(),
                        [&](std::pair<ParamModel*,double*> p)
    {
            return p.first == model;
    });
    return it->second;
}

void ParserMgr::InitVars()
{
    try
    {
        for (auto& it : _models)
        {
            ParamModel* model = it.first;
            if (!model->DoAddToParser()) continue;
            double* data = it.second;
            const size_t num_pars = model->NumPars();
            for (size_t i=0; i<num_pars; ++i)
            {
                const std::string& key = model->ShortKey(i),
                        & value = model->Value(i);

                //Set up the parsers
                _parser.DefineVar(key, &data[i]);
                _parserResult.DefineVar(key, &data[i]);
                for (auto& itp : _parserConds)
                    itp.DefineVar(key, &data[i]);

                //Assign the source of the data--i.e. attach an input source if needed
                AssignInput(model, i, value);

                //Initialize
                data[i] = atof(value.c_str()); // ###
            }
        }
    }
    catch (mu::ParserError& e)
    {
        std::cerr << e.GetMsg() << std::endl;
        qDebug() << e.GetMsg().c_str();
    }
}
void ParserMgr::InitParsers()
{
    try
    {
        VecStr initializations, expressions;
        for (auto& it : _models)
        {
            ParamModel* model = it.first;
            VecStr model_inits = model->Initializations(),
                    model_exprns = model->Expressions();
            initializations.insert(initializations.end(),
                                   model_inits.begin(),
                                   model_inits.end());
            expressions.insert(expressions.end(),
                                   model_exprns.begin(),
                                   model_exprns.end());
        }

        for (const auto& it : initializations)
        {
            SetExpression(it);
            _parser.Eval();
        }

        SetExpression(expressions);
            //Note that the expressions are not actually evaluated
    }
    catch (mu::ParserError& e)
    {
        std::cerr << e.GetMsg() << std::endl;
    }
}

void ParserMgr::InputEval(int idx)
{
    if (idx==-1)
    {
        for (auto& it : _inputs)
            it.NextInput();
    }
    else
        _inputs[idx].NextInput();
}
void ParserMgr::ParserCondEval()
{
    int num_conds = (int)_conditions->NumPars();
    for (int k=0; k<num_conds; ++k)
    {
        if (_parserConds.at(k).Eval())
        {
            VecStr cond_exprns = _conditions->Expressions(k);
            for (const auto& it : cond_exprns)
            {
                _parserResult.SetExpr(it);
                _parserResult.Eval();
            }
        }
    }
}
void ParserMgr::ParserEval()
{
    try
    {
        _parser.Eval();
        InputEval();
    }
    catch (mu::ParserError& e)
    {
        std::cerr << e.GetMsg() << std::endl;
    }
}

void ParserMgr::SetConditions()
{
    int num_conds = (int)_conditions->NumPars();
    for (int k=0; k<num_conds; ++k)
    {
        std::string expr = _conditions->Condition(k);
        _parserConds[k].SetExpr(expr);
    }
}
void ParserMgr::SetCondModel(ConditionModel* conditions)
{
    _conditions = conditions;
    _parserConds = std::vector<mu::Parser>( conditions->NumPars() );
    SetConditions();
}
void ParserMgr::SetExpression(const std::string& exprn)
{
    _parser.SetExpr(exprn);
}
void ParserMgr::SetExpression(const VecStr& exprns)
{
    _parser.SetExpr("");
    for (const auto& it: exprns)
        AddExpression(it);
}
void ParserMgr::SetExpressions()
{
    try
    {
        ClearExpressions();
        for (auto& it : _models)
        {
            ParamModel* model = it.first;
            VecStr expressions = model->Expressions();
            for (const auto& it : expressions)
                AddExpression(it);
        }
    }
    catch (mu::ParserError& e)
    {
        std::cerr << e.GetMsg() << std::endl;
        qDebug() << e.GetMsg().c_str();
    }
}
