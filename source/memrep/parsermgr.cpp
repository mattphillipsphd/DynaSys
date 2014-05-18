#include "parsermgr.h"

ParserMgr::ParserMgr() : _areModelsInitialized(false), _log(Log::Instance()),
    _modelStep(ds::DEFAULT_MODEL_STEP), _varInitVals(nullptr)
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker::InitThread(std::this_thread::get_id());
    ScopeTracker st("ParserMgr::ParserMgr", std::this_thread::get_id());
#endif
}
ParserMgr::~ParserMgr()
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::~ParserMgr", std::this_thread::get_id());
#endif
    ClearModels();
}

void ParserMgr::AddExpression(const std::string& exprn, bool use_mutex)
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::AddExpression", std::this_thread::get_id());
#endif
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
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::AddModel", std::this_thread::get_id());
#endif
    double* data = new double[model->NumPars()],
            * temp = new double[model->NumPars()];
    _models.push_back( std::make_tuple(model, data, temp) );
}
void ParserMgr::AssignInput(const ParamModelBase* model, size_t i,
                            const std::string& type_str, bool do_lock)
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::AssignInput", std::this_thread::get_id());
#endif
    std::unique_lock<std::mutex> ulock(_mutex, std::defer_lock);
    if (do_lock) ulock.lock();

    Input::TYPE type = Input::Type(type_str);
    double* data = TempData(model);
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
            file_name.erase(
                std::remove_if(file_name.begin(), file_name.end(), [&](std::string::value_type c)
                {
                    return c=='"';
                }),
                    file_name.end());
            input.LoadInput(file_name);
            _inputs.push_back(input);
            break;
        }
        case Input::USER:
            break;
        case Input::UNKNOWN:
            throw std::runtime_error("ParserMgr::SetInput: Unknown input.");
    }
}
void ParserMgr::ClearExpressions()
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::ClearExpressions", std::this_thread::get_id());
#endif
    std::lock_guard<std::mutex> lock(_mutex);
    _parser.SetExpr("");
}
void ParserMgr::ClearModels()
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::ClearModels", std::this_thread::get_id());
#endif
    _models.clear();
    _areModelsInitialized = false;
}

const double* ParserMgr::ConstData(ds::PMODEL pmodel) const
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::ConstData", std::this_thread::get_id());
#endif
    return const_cast<ParserMgr*>(this)->Data(pmodel);
}

void ParserMgr::InitParsers()
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::InitParsers", std::this_thread::get_id());
#endif
    try
    {
        VecStr initializations, expressions;
        for (auto& it : _models)
        {
            ParamModelBase* model = std::get<0>(it);
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

        _stepCt = std::numeric_limits<int>::max() - 1;
    }
    catch (mu::ParserError& e)
    {
        _log->AddExcept("ParserMgr::InitParsers: " + std::string(e.GetMsg()));
    }
}
void ParserMgr::InitModels()
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::InitModels", std::this_thread::get_id());
#endif
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
            ParamModelBase* model = std::get<0>(it);
            if (!model->DoInitialize()) continue;
            double* data = std::get<1>(it);
            const size_t num_pars = model->NumPars();
            for (size_t i=0; i<num_pars; ++i)
            {
                const std::string& value = model->Value(i);

                //Assign the source of the data--i.e. attach an input source if needed
                AssignInput(model, i, value, false);

                //Initialize
                if (model->Id()==ds::INPUTS) // ###
                    data[i] = std::atof(value.c_str());
            }
        }
        _areModelsInitialized = true;
    }
    catch (mu::ParserError& e)
    {
        _log->AddExcept("ParserMgr::InitModels: " + std::string(e.GetMsg()));
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
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::Maximum", std::this_thread::get_id());
#endif
    return model->Maximum(idx);
}
double ParserMgr::Minimum(const ParamModelBase* model, size_t idx) const
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::Minimum", std::this_thread::get_id());
#endif
    return model->Minimum(idx);
}
const std::string& ParserMgr::ParserContents() const
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::ParserContents", std::this_thread::get_id());
#endif
    return _parser.GetExpr();
}
void ParserMgr::ParserEval(bool eval_input)
{
    std::lock_guard<std::mutex> lock(_mutex);
    try
    {
        _parser.Eval();
        TempEval();
        if (eval_input && ++_stepCt*_modelStep>1.0)
        {
            _stepCt = 0;
            InputEval();
        }
    }
    catch (mu::ParserError& e)
    {
        _log->AddExcept("ParserMgr::ParserEval: " + AnnotateErrMsg(e.GetMsg(), _parser)
                        + "\n" + _parser.GetExpr());
        throw std::runtime_error("Parser error");
    }
}
void ParserMgr::ParserEvalAndConds(bool eval_input)
{
    std::lock_guard<std::mutex> lock(_mutex);
    try
    {
        ParserEvalAndCondsNoLock(eval_input);
    }
    catch (mu::ParserError& e)
    {
        _log->AddExcept("ParserMgr::ParserEvalAndConds: " + AnnotateErrMsg(e.GetMsg(), _parser)
                        + "\n" + _parser.GetExpr());
        throw std::runtime_error("Parser error");
    }
}
void ParserMgr::ParserEvalAndCondsNoLock(bool eval_input)
{
    _parser.Eval();
    TempEval();
    if (eval_input && ++_stepCt*_modelStep>1.0)
    {
        _stepCt = 0;
        InputEval();
    }

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
void ParserMgr::QuickEval(const std::string& exprn)
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::QuickEval", std::this_thread::get_id());
    std::lock_guard<std::mutex> lock(_mutex);
    try
    {
        st.Add("1");
        std::string temp = _parser.GetExpr();
        st.Add("2");
        _parser.SetExpr(exprn);
        st.Add("3");
        _parser.Eval();
        st.Add("4");
        _parser.SetExpr(temp);
        st.Add("5");
#else
    std::lock_guard<std::mutex> lock(_mutex);
    try
    {
        std::string temp = _parser.GetExpr();
        _parser.SetExpr(exprn);
        _parser.Eval();
        _parser.SetExpr(temp);
#endif
    }
    catch (mu::ParserError& e)
    {
        _log->AddExcept("ParserMgr::QuickEval: " + AnnotateErrMsg(e.GetMsg(), _parser)
                        + "\n" + _parser.GetExpr());
        throw std::runtime_error("Parser error");
    }
}
double ParserMgr::Range(const ParamModelBase* model, size_t idx) const
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::Range", std::this_thread::get_id());
#endif
    return Maximum(model, idx) - Minimum(model, idx);
}
void ParserMgr::ResetDifferentials()
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::ResetDifferentials", std::this_thread::get_id());
#endif
    const size_t num_diffs = Model(ds::DIFFERENTIALS)->NumPars();
    std::memcpy(Data(ds::DIFFERENTIALS),
                ConstData(ds::INIT_CONDS), num_diffs*sizeof(double));
}
void ParserMgr::ResetValues()
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::ResetValues", std::this_thread::get_id());
#endif
    ResetDifferentials();
    const size_t num_vars = Model(ds::VARIABLES)->NumPars();
    std::memcpy(Data(ds::VARIABLES), _varInitVals, num_vars*sizeof(double));
}
void ParserMgr::ResetVarInitVals()
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::ResetVarInitVals", std::this_thread::get_id());
#endif
    if (_varInitVals) delete[] _varInitVals;

    ResetDifferentials();

    ParamModelBase* var_model = Model(ds::VARIABLES);
    const size_t num_vars = var_model->NumPars();
    _varInitVals = new double[num_vars];

    VecStr expressions = var_model->Expressions();
    for (const auto& it : expressions)
        if ( Input::Type(it)==Input::USER )
            QuickEval(it);
    std::memcpy(_varInitVals, Data(ds::VARIABLES), num_vars*sizeof(double));
}

void ParserMgr::SetConditions()
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::SetConditions", std::this_thread::get_id());
#endif
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
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::SetCondModel", std::this_thread::get_id());
#endif
    _conditions = conditions;
    SetConditions();
}
void ParserMgr::SetData(const ParamModelBase* model, size_t idx, double val)
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::SetData", std::this_thread::get_id());
#endif
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = std::find_if(_models.begin(), _models.end(),
                        [&](std::tuple<ParamModelBase*,double*,double*> p)
    {
            return std::get<0>(p) == model;
    });
    *(std::get<1>(*it) + idx) = val;
}
void ParserMgr::SetExpression(const std::string& exprn)
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::SetExpression", std::this_thread::get_id());
#endif
    std::lock_guard<std::mutex> lock(_mutex);
    _parser.SetExpr(exprn);
}
void ParserMgr::SetExpression(const VecStr& exprns)
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::SetExpression", std::this_thread::get_id());
#endif
    std::lock_guard<std::mutex> lock(_mutex);
    _parser.SetExpr("");
    for (const auto& it: exprns)
        AddExpression(it, false);
}
void ParserMgr::SetExpressions()
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::SetExpressions", std::this_thread::get_id());
#endif
    try
    {
        ClearExpressions();
        for (auto& it : _models)
        {
            ParamModelBase* model = std::get<0>(it);
            if (!model->DoEvaluate()) continue;
            VecStr temp_expressions = model->TempExpressions();
            for (const auto& it : temp_expressions)
                AddExpression(it);
            const size_t num_pars = model->NumPars();
            for (size_t i=0; i<num_pars; ++i)
                if (model->IsFreeze(i))
                {
                    std::string freeze_val = (model->Id()==ds::DIFFERENTIALS)
                            ? Model(ds::INIT_CONDS)->Value(i)
                            : "0";
                    AddExpression(model->TempKey(i) + " = " + freeze_val);
                }
            if (model->Id()==ds::VARIABLES) // ### Really need separate parsers for both here
                for (size_t i=0; i<num_pars; ++i)
                    AddExpression(model->Key(i) + " = " + model->TempKey(i));
        }
    }
    catch (mu::ParserError& e)
    {
        _log->AddExcept("ParserMgr::SetExpressions: " + std::string(e.GetMsg()));
    }
}
void ParserMgr::SetModelStep(double step)
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::SetModelStep", std::this_thread::get_id());
#endif
    _modelStep = step;
}
void ParserMgr::TempEval()
{
    for (auto& it : _models)
    {
        const ParamModelBase* model = std::get<0>(it);
        if ( model->DoEvaluate() )
        {
            const size_t num_pars = model->NumPars();
            double* data = std::get<1>(it);
            const double* temp = std::get<2>(it);
            for (size_t i=0; i<num_pars; ++i)
                data[i] = temp[i];
        }
    }
}

std::string ParserMgr::AnnotateErrMsg(const std::string& err_mesg, const mu::Parser& parser) const
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::AnnotateErrMsg", std::this_thread::get_id());
#endif
    const char* pos_str = "position ";
    size_t pos = err_mesg.find(pos_str);
    if (pos!=std::string::npos)
    {
        size_t spc = err_mesg.find_first_of(' ', pos);
        std::string s = err_mesg.substr(spc);
        int errpos = std::stoi(s);
        std::string pstr = parser.GetExpr();
        std::string small = "..." + pstr.substr(errpos, 20) + "...";
        return err_mesg + "  " + small;
    }
    return err_mesg;
}
void ParserMgr::AssociateVars(mu::Parser& parser)
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::AssociateVars", std::this_thread::get_id());
#endif
    try
    {
        for (auto& it : _models)
        {
            ParamModelBase* model = std::get<0>(it);
            if (model->Id()==ds::INIT_CONDS) continue;
            double* data = std::get<1>(it),
                    * temp_data = std::get<2>(it);
            const size_t num_pars = model->NumPars();
            for (size_t i=0; i<num_pars; ++i)
            {
                const std::string& key = model->ShortKey(i);
                parser.DefineVar(key, &data[i]);
                if (model->DoEvaluate())
                    parser.DefineVar(model->TempKey(i), &temp_data[i]);
            }
        }
    }
    catch (mu::ParserError& e)
    {
        _log->AddExcept("ParserMgr::AssociateVars: " + AnnotateErrMsg(e.GetMsg(), parser));
    }
}
double* ParserMgr::Data(const ParamModelBase* model)
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::Data", std::this_thread::get_id());
#endif
    auto it = std::find_if(_models.cbegin(), _models.cend(),
                        [&](std::tuple<ParamModelBase*,double*,double*> p)
    {
            return std::get<0>(p) == model;
    });
    return std::get<1>(*it);
}
double* ParserMgr::Data(ds::PMODEL model)
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::Data", std::this_thread::get_id());
#endif
    return std::get<1>(_models[model]);
}
ParamModelBase* ParserMgr::Model(ds::PMODEL model)
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::Model", std::this_thread::get_id());
#endif
    return std::get<0>(_models[model]);
}
double* ParserMgr::TempData(const ParamModelBase* model)
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::TempData", std::this_thread::get_id());
#endif
    auto it = std::find_if(_models.cbegin(), _models.cend(),
                        [&](std::tuple<ParamModelBase*,double*,double*> p)
    {
            return std::get<0>(p) == model;
    });
    return std::get<2>(*it);
}
double* ParserMgr::TempData(ds::PMODEL model)
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::TempData", std::this_thread::get_id());
#endif
    return std::get<2>(_models[model]);
}

