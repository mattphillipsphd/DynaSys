#include "parsermgr.h"

ParserMgr::ParserMgr()
    : _log(Log::Instance()), _modelData( MakeModelData() ),
      _modelMgr(ModelMgr::Instance()), _varInitVals(nullptr)
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker::InitThread(std::this_thread::get_id());
    ScopeTracker st("ParserMgr::ParserMgr", std::this_thread::get_id());
#endif
}
ParserMgr::ParserMgr(const ParserMgr& other)
    : _inputs(other._inputs), _log(other._log),
      _modelData( MakeModelData() ), _modelMgr(other._modelMgr), _varInitVals(nullptr)
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::ParserMgr(const ParserMgr&)", std::this_thread::get_id());
#endif
    DeepCopy(other);
}
ParserMgr::~ParserMgr()
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::~ParserMgr", std::this_thread::get_id());
#endif
    for (auto it : _modelData)
    {
        delete[] it.first;
        delete[] it.second;
    }
    if (_varInitVals) delete[] _varInitVals;
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
void ParserMgr::AssignVariable(int row, const std::string& text)
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::AssignVariable", std::this_thread::get_id());
#endif
    _modelMgr->AssignInput(row, _modelData[ds::VAR].first, text, true);
}
void ParserMgr::ClearExpressions()
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::ClearExpressions", std::this_thread::get_id());
#endif
    std::lock_guard<std::mutex> lock(_mutex);
    _parser.SetExpr("");
}

void ParserMgr::InitData()
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::InitData", std::this_thread::get_id());
#endif
    try
    {
        AssociateVars(_parser);
        AssociateVars(_parserResult);
        for (auto& itp : _parserConds)
            AssociateVars(itp);

        _inputs.clear();

        VecStr initializations, expressions;
        for (size_t i=0; i<ds::NUM_MODELS; ++i)
        {
            const ParamModelBase* model = _modelMgr->Model((ds::PMODEL)i);
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
        for (size_t i=0; i<ds::NUM_MODELS; ++i)
        {
            const ParamModelBase* model = _modelMgr->Model((ds::PMODEL)i);
            if (!model->DoInitialize()) continue;
            double* data = _modelData[i].first,
                    *temp_data = _modelData[i].second;
            const size_t num_pars = model->NumPars();
            for (size_t k=0; k<num_pars; ++k)
            {
                const std::string& value = model->Value(k);

                //Assign the source of the data--i.e. attach an input source if needed
                _modelMgr->AssignInput(k, temp_data, value, false);

                //Initialize
                if (model->Id()==ds::INP) // ###
                    data[k] = std::stod(value.c_str());
            }
        }

        ResetVarInitVals();
    }
    catch (mu::ParserError& e)
    {
        _log->AddExcept("ParserMgr::InitModels: " + std::string(e.GetMsg()));
    }
}
void ParserMgr::InitParsers()
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::InitParsers", std::this_thread::get_id());
#endif
    try
    {
        VecStr expressions;
        for (size_t i=0; i<ds::NUM_MODELS; ++i)
        {
            const ParamModelBase* model = _modelMgr->Model((ds::PMODEL)i);
            if (model->DoEvaluate())
            {
                VecStr model_exprns = model->Expressions();
                expressions.insert(expressions.end(),
                                       model_exprns.begin(),
                                       model_exprns.end());
            }
        }
        SetExpression(expressions);
            //Note that the expressions are not actually evaluated

        _stepCt = std::numeric_limits<int>::max() - 1;
    }
    catch (mu::ParserError& e)
    {
        _log->AddExcept("ParserMgr::InitParsers: " + std::string(e.GetMsg()));
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
        if (eval_input && ++_stepCt*_modelMgr->ModelStep()*_inputsPerUnitTime>1.0)
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
    if (eval_input && ++_stepCt*_modelMgr->ModelStep()*_inputsPerUnitTime>1.0)
    {
        _stepCt = 0;
        InputEval();
    }

    const int num_conds = (int)_modelMgr->Model(ds::COND)->NumPars();
    for (int k=0; k<num_conds; ++k)
    {
        if (_parserConds.at(k).Eval())
        {
            VecStr cond_results = _modelMgr->CondResults(k);
            for (const auto& it : cond_results)
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
void ParserMgr::ResetData()
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::ResetData", std::this_thread::get_id());
#endif
    ResetDifferentials();
    ResetVariables();
}
void ParserMgr::ResetDifferentials()
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::ResetDifferentials", std::this_thread::get_id());
#endif
    const size_t num_diffs = _modelMgr->Model(ds::DIFF)->NumPars();
    std::memcpy(Data(ds::DIFF),
                ConstData(ds::INIT), num_diffs*sizeof(double));
//    qDebug() << "Diffs:" << Data(ds::DIFF)[0] << Data(ds::DIFF)[1];
}
void ParserMgr::ResetVariables()
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::ResetVariables", std::this_thread::get_id());
#endif
    const size_t num_vars = _modelMgr->Model(ds::VAR)->NumPars();
    std::memcpy(Data(ds::VAR), _varInitVals, num_vars*sizeof(double));
//    qDebug() << "Vars:" << _varInitVals[0] << _varInitVals[1];
}
void ParserMgr::ResetVarInitVals()
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::ResetVarInitVals", std::this_thread::get_id());
#endif
    if (_varInitVals) delete[] _varInitVals;

    ResetDifferentials();

    const ParamModelBase* var_model = _modelMgr->Model(ds::VAR);
    const size_t num_vars = var_model->NumPars();
    _varInitVals = new double[ds::MAX_NUM_PARS];

    VecStr expressions = var_model->Expressions();
    for (const auto& it : expressions)
        if ( Input::Type(it)==Input::USER )
            QuickEval(it);
    std::memcpy(_varInitVals, ConstData(ds::VAR), num_vars*sizeof(double));
}
void ParserMgr::TempEval()
{
    for (size_t i=0; i<ds::NUM_MODELS; ++i)
    {
        const ParamModelBase* model = _modelMgr->Model((ds::PMODEL)i);
        if ( model->DoEvaluate() )
        {
            const size_t num_pars = model->NumPars();
            double* data = _modelData[i].first;
            const double* temp = _modelData.at(i).second;
            for (size_t i=0; i<num_pars; ++i)
                data[i] = temp[i];
        }
    }
}

void ParserMgr::SetConditions()
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::SetConditions", std::this_thread::get_id());
#endif
    std::lock_guard<std::mutex> lock(_mutex);
    size_t num_conds = _modelMgr->Model(ds::COND)->NumPars();
    _parserConds = std::vector<mu::Parser>(num_conds);
    for (auto& itp : _parserConds)
        AssociateVars(itp);

    for (size_t k=0; k<num_conds; ++k)
    {
        std::string expr = _modelMgr->Model(ds::COND)->Key(k);
        _parserConds[k].SetExpr(expr);
    }
}
void ParserMgr::SetData(ds::PMODEL mi, size_t idx, double val)
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::SetData", std::this_thread::get_id());
#endif
    std::lock_guard<std::mutex> lock(_mutex);
    _modelData[mi].first[idx] = val;
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
        for (size_t i=0; i<ds::NUM_MODELS; ++i)
        {
            const ParamModelBase* model = _modelMgr->Model((ds::PMODEL)i);
            if (!model->DoEvaluate()) continue;
            VecStr temp_expressions = model->TempExpressions();
            for (const auto& it : temp_expressions)
                AddExpression(it);
            const size_t num_pars = model->NumPars();
            for (size_t k=0; k<num_pars; ++k)
                if (model->IsFreeze(k))
                {
                    std::string freeze_val = (model->Id()==ds::DIFF)
                            ? _modelMgr->Model(ds::INIT)->Value(k)
                            : "0";
                    AddExpression(model->TempKey(k) + " = " + freeze_val);
                }
            if (model->Id()==ds::VAR) // ### Really need separate parsers for both here
                for (size_t i=0; i<num_pars; ++i)
                    AddExpression(model->Key(i) + " = " + model->TempKey(i));
        }
    }
    catch (mu::ParserError& e)
    {
        _log->AddExcept("ParserMgr::SetExpressions: " + std::string(e.GetMsg()));
    }
}

const double* ParserMgr::ConstData(ds::PMODEL mi) const
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::ConstData", std::this_thread::get_id());
#endif
    return _modelData.at(mi).first;
}
double* ParserMgr::Data(ds::PMODEL mi)
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::Data", std::this_thread::get_id());
#endif
    return _modelData[mi].first;
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
        for (size_t i=0; i<ds::NUM_MODELS; ++i)
        {
            const ParamModelBase* model = _modelMgr->Model((ds::PMODEL)i);
            if (model->Id()==ds::INIT || model->Id()==ds::COND) continue;
            double* data = _modelData[i].first,
                    * temp_data = _modelData[i].second;
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
void ParserMgr::DeepCopy(const ParserMgr& other)
{
    _inputsPerUnitTime      = other._inputsPerUnitTime;
    _stepCt                 = 0;

    for (size_t i=0; i<ds::NUM_MODELS; ++i)
    {
        const size_t num_pars = _modelMgr->Model((ds::PMODEL)i)->NumPars();
        memcpy(_modelData[i].first, other._modelData.at(i).first, num_pars*sizeof(double));
        memcpy(_modelData[i].second, other._modelData.at(i).second, num_pars*sizeof(double));
    }

    InitParsers();
    InitData();
}
std::vector< std::pair<double*,double*> > ParserMgr::MakeModelData()
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::MakeModelData", std::this_thread::get_id());
#endif
    auto model_data = std::vector< std::pair<double*,double*> >(ds::NUM_MODELS);
    for (size_t i=0; i<ds::NUM_MODELS; ++i)
    {
        model_data[i].first = new double[ds::MAX_NUM_PARS];
        model_data[i].second = new double[ds::MAX_NUM_PARS];
    }
    return model_data;
}
double* ParserMgr::TempData(ds::PMODEL model)
{
#ifdef DEBUG_PM_FUNC
    ScopeTracker st("ParserMgr::TempData", std::this_thread::get_id());
#endif
    return _modelData[model].second;
}

