#include "modelmgr.h"

ModelMgr* ModelMgr::_instance = nullptr;

ModelMgr* ModelMgr::Instance()
{
    if (!_instance) _instance = new ModelMgr();
    return _instance;
}

ModelMgr::~ModelMgr()
{
    ClearModels();
    delete _instance;
}

void ModelMgr::AddModel(ParamModelBase* model)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ModelMgr::AddModel", std::this_thread::get_id());
#endif
    _models.push_back(model);
}
void ModelMgr::AddCondParameter(const std::string& test, const VecStr& results)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ModelMgr::AddCondParameter", std::this_thread::get_id());
#endif
    CondModel()->AddCondition(test, results);
}
void ModelMgr::AddCondResult(int row, const std::string& result)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ModelMgr::AddCondResult", std::this_thread::get_id());
#endif
    CondModel()->AddResult(row, result);
}
void ModelMgr::AddParameter(ds::PMODEL mi, const std::string& key, const std::string& value)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ModelMgr::AddParameter", std::this_thread::get_id());
#endif
    _models[mi]->AddParameter(key, value);
}


void ModelMgr::AssignInput(size_t i, double* data,
                            const std::string& type_str, bool do_lock)
{
#ifdef DEBUG_FUNC
//    ScopeTracker st("ModelMgr::AssignInput", std::this_thread::get_id());
#endif
    std::unique_lock<std::mutex> ulock(_mutex, std::defer_lock);
    if (do_lock) ulock.lock();

    Input::TYPE type = Input::Type(type_str);
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
        case Input::INPUT_FILE:
        {
            Input input(&data[i]);
            std::string file_name = type_str;
            file_name.erase(
                std::remove_if(file_name.begin(), file_name.end(), [&](std::string::value_type c)
                {
                    return c=='"';
                }),
                    file_name.end());
            input.LoadInput(file_name);
            _inputsPerUnitTime = input.SamplesPerStep();
            _inputs.push_back(input);
            break;
        }
        case Input::USER:
            break;
        case Input::UNKNOWN:
            throw std::runtime_error("ModelMgr::AssignInput: Unknown input.");
    }
}
void ModelMgr::CreateModels()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ModelMgr::CreateModels", std::this_thread::get_id());
#endif
    ClearModels();
    for (int i=0; i<ds::NUM_MODELS; ++i)
        _models[i] = ParamModelBase::Create((ds::PMODEL)i);
}
void ModelMgr::ClearModels()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ModelMgr::ClearModels", std::this_thread::get_id());
#endif
    for (auto it : _models)
        delete it;
    _models = MakeModelVec();
}

void ModelMgr::SetCondValue(size_t row, const VecStr& results)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ModelMgr::SetCondParameter", std::this_thread::get_id());
#endif
    CondModel()->SetResults(row, results);
}
void ModelMgr::SetMaximum(ds::PMODEL mi, size_t idx, double val)
{
    dynamic_cast<NumericModelBase*>(_models[mi])->SetMaximum(idx, val);
}
void ModelMgr::SetMinimum(ds::PMODEL mi, size_t idx, double val)
{
    dynamic_cast<NumericModelBase*>(_models[mi])->SetMinimum(idx, val);
}
void ModelMgr::SetModel(ds::PMODEL mi, ParamModelBase* model)
{
    if (_models[mi]) delete _models[mi];
    _models[mi] = model;
}
void ModelMgr::SetNotes(Notes* notes)
{
    if (_notes) delete _notes;
    _notes = notes;
}
void ModelMgr::SetNotes(const std::string& text)
{
    Notes* notes = new Notes();
    notes->SetText(text);
    SetNotes(notes);
}
void ModelMgr::SetRange(ds::PMODEL mi, size_t idx, double min, double max)
{
    SetMinimum(mi, idx, min);
    SetMaximum(mi, idx, max);
}
void ModelMgr::SetTPVModel(TPVTableModel* tpv_model)
{
//    if (_tpvModel) delete _tpvModel;
    _tpvModel = tpv_model;
}
void ModelMgr::SetValue(ds::PMODEL mi, size_t idx, const std::string& value)
{
    _models[mi]->SetValue(idx, value);
}
void ModelMgr::SetView(QAbstractItemView* view, ds::PMODEL mi)
{
    view->setModel(_models[mi]);
}


bool ModelMgr::AreModelsInitialized() const
{
    return _models.at(0) != nullptr;
}
VecStr ModelMgr::CondResults(size_t row) const
{
    return CondModel()->Results(row);
}
double ModelMgr::Maximum(ds::PMODEL mi, size_t idx) const
{
    return dynamic_cast<const NumericModelBase*>(_models.at(mi))->Maximum(idx);
}
double ModelMgr::Minimum(ds::PMODEL mi, size_t idx) const
{
    return dynamic_cast<const NumericModelBase*>(_models.at(mi))->Minimum(idx);
}
double ModelMgr::Range(ds::PMODEL mi, size_t idx) const
{
    return Maximum(mi, idx) - Minimum(mi, idx);
}
std::string ModelMgr::Value(ds::PMODEL mi, size_t idx) const
{
    return _models.at(mi)->Value(idx);
}

ModelMgr::ModelMgr() : _log(Log::Instance()), _models(MakeModelVec()),
    _notes(nullptr), _tpvModel(nullptr)
{
    CreateModels();
}

ConditionModel* ModelMgr::CondModel()
{
    return static_cast<ConditionModel*>(_models[ds::COND]);
}
const ConditionModel* ModelMgr::CondModel() const
{
    return static_cast<ConditionModel*>(_models.at(ds::COND));
}
std::vector<ParamModelBase*> ModelMgr::MakeModelVec() const
{
    return std::vector<ParamModelBase*>(ds::NUM_MODELS, nullptr);
}
