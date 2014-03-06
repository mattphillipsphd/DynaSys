#include "parsermgr.h"

ParserMgr::ParserMgr() : _areModelsInitialized(false)
{
}
ParserMgr::~ParserMgr()
{
    ClearModels();
}

void ParserMgr::AddExpression(const std::string& exprn, bool use_mutex)
{
    if (use_mutex) _mutex.lock();
    if (!exprn.empty())
    {
        std::string old_exprn = _parser.GetExpr();
        old_exprn.erase( old_exprn.find_last_not_of(" ")+1 );
        _parser.SetExpr(old_exprn.empty() ? exprn : old_exprn + ", "
                         + exprn );
    }
    if (use_mutex) _mutex.unlock();
}
void ParserMgr::AddModel(ParamModelBase* model)
{
    double* data = new double[model->NumPars()];
    _models.push_back( std::make_pair(model, data) );
}
void ParserMgr::AssignInput(const ParamModelBase* model, size_t i, const std::string& type_str)
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
void ParserMgr::ClearExpressions()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _parser.SetExpr("");
}
void ParserMgr::ClearModels()
{
    for (auto it : _models)
        delete it.second;
    _models.clear();
    _areModelsInitialized = false;
}

const double* ParserMgr::ConstData(const ParamModelBase* model) const
{
    return const_cast<ParserMgr*>(this)->Data(model);
}

void ParserMgr::InitParsers()
{
    try
    {
        VecStr initializations, expressions;
        for (auto& it : _models)
        {
            ParamModelBase* model = it.first;
            if (model->DoInitialize())
            {
                VecStr model_inits = model->Initializations();
                initializations.insert(initializations.end(),
                                       model_inits.begin(),
                                       model_inits.end());
            }
            if (model->DoEvaluate())
            {
                VecStr model_exprns = model->Expressions();
                expressions.insert(expressions.end(),
                                       model_exprns.begin(),
                                       model_exprns.end());
            }
        }

        for (const auto& it : initializations)
            QuickEval(it);

        SetExpression(expressions);
            //Note that the expressions are not actually evaluated
    }
    catch (mu::ParserError& e)
    {
        std::cerr << e.GetMsg() << std::endl;
    }
}
void ParserMgr::InitModels()
{
    std::lock_guard<std::mutex> lock(_mutex);
    try
    {
        AssociateVars(_parser);
        AssociateVars(_parserResult);
        for (auto& itp : _parserConds)
            AssociateVars(itp);

        _inputs.clear();

        for (auto& it : _models)
        {
            ParamModelBase* model = it.first;
            if (!model->DoInitialize()) continue;
            double* data = it.second;
            const size_t num_pars = model->NumPars();
            for (size_t i=0; i<num_pars; ++i)
            {
                const std::string& value = model->Value(i);

                //Assign the source of the data--i.e. attach an input source if needed
                AssignInput(model, i, value);

                //Initialize
#ifdef QT_DEBUG
                qDebug() << model->Key(i).c_str() << value.c_str() << ", " << atof(value.c_str());
#endif
                if (model->Id()==ds::PARAMETERS)
                    data[i] = atof(value.c_str()); // ###
            }
        }
        _areModelsInitialized = true;
    }
    catch (mu::ParserError& e)
    {
        std::cerr << e.GetMsg() << std::endl;
        qDebug() << e.GetMsg().c_str();
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
double ParserMgr::Maximum(const ParamModelBase* model, size_t idx) const
{
    return model->Maximum(idx);
}
double ParserMgr::Minimum(const ParamModelBase* model, size_t idx) const
{
    return model->Minimum(idx);
}
void ParserMgr::ParserCondEval()
{
    std::lock_guard<std::mutex> lock(_mutex);
    const int num_conds = (int)_conditions->NumPars();
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
void ParserMgr::ParserEval(bool eval_input)
{
    std::lock_guard<std::mutex> lock(_mutex);
    try
    {
//        qDebug() << _parser.GetExpr().c_str();
        _parser.Eval();
        if (eval_input) InputEval();
    }
    catch (mu::ParserError& e)
    {
        std::cerr << e.GetMsg() << "," << _parser.GetExpr() << std::endl;
        qDebug() << e.GetMsg().c_str() << "," << _parser.GetExpr().c_str();
    }
}
void ParserMgr::QuickEval(const std::string& exprn)
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::string temp = _parser.GetExpr();
    _parser.SetExpr(exprn);
    _parser.Eval();
    _parser.SetExpr(temp);
}
double ParserMgr::Range(const ParamModelBase* model, size_t idx) const
{
    return Maximum(model, idx) - Minimum(model, idx);
}
void ParserMgr::ResetDifferentials()
{
    ParamModelBase* init_conds = Model(ds::INIT_CONDS);
    const size_t num_diffs = init_conds->NumPars();
    std::string init_expr;
    for (size_t i=0; i<num_diffs; ++i)
    {
        const std::string d = init_conds->ShortKey(i),
                    expr = init_conds->Value(i);
        if (i>0) init_expr += ", ";
        init_expr += d + " = " + expr;
    }
    QuickEval(init_expr);
}

void ParserMgr::SetConditions()
{
    std::lock_guard<std::mutex> lock(_mutex);
    int num_conds = (int)_conditions->NumPars();
    _parserConds = std::vector<mu::Parser>(num_conds);
    for (auto& itp : _parserConds)
        AssociateVars(itp);

    for (int k=0; k<num_conds; ++k)
    {
        std::string expr = _conditions->Condition(k);
        _parserConds[k].SetExpr(expr);
    }
}
void ParserMgr::SetCondModel(ConditionModel* conditions)
{
    _conditions = conditions;
    SetConditions();
}
void ParserMgr::SetData(const ParamModelBase* model, size_t idx, double val)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = std::find_if(_models.begin(), _models.end(),
                        [&](std::pair<ParamModelBase*,double*> p)
    {
            return p.first == model;
    });
    *(it->second + idx) = val;
}
void ParserMgr::SetExpression(const std::string& exprn)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _parser.SetExpr(exprn);
}
void ParserMgr::SetExpression(const VecStr& exprns)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _parser.SetExpr("");
    for (const auto& it: exprns)
        AddExpression(it, false);
}
void ParserMgr::SetExpressions()
{
    try
    {
        ClearExpressions();
        for (auto& it : _models)
        {
            ParamModelBase* model = it.first;
            if (!model->DoEvaluate()) continue;
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

void ParserMgr::AssociateVars(mu::Parser& parser)
{
    try
    {
        for (auto& it : _models)
        {
            ParamModelBase* model = it.first;
            if (model->Id()==ds::INIT_CONDS) continue;
            double* data = it.second;
            const size_t num_pars = model->NumPars();
            for (size_t i=0; i<num_pars; ++i)
            {
                const std::string& key = model->ShortKey(i);
                parser.DefineVar(key, &data[i]);
            }
        }
    }
    catch (mu::ParserError& e)
    {
        std::cerr << e.GetMsg() << std::endl;
        qDebug() << e.GetMsg().c_str();
    }
}
double* ParserMgr::Data(const ParamModelBase* model)
{
    auto it = std::find_if(_models.cbegin(), _models.cend(),
                        [&](std::pair<ParamModelBase*,double*> p)
    {
            return p.first == model;
    });
    return it->second;
}
ParamModelBase* ParserMgr::Model(ds::PMODEL model)
{
    auto it = std::find_if(_models.cbegin(), _models.cend(),
                        [&](std::pair<ParamModelBase*,double*> p)
    {
            return p.first->Id() == model;
    });
    return it->first;
}

