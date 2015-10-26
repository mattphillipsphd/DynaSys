#include "modelmgr.h"

const std::string ModelMgr::ParVariant::END_NOTES = "###End Notes###";

ModelMgr* ModelMgr::_instance = nullptr;

ModelMgr* ModelMgr::Instance()
{
    if (!_instance) _instance = new ModelMgr();
    return _instance;
}

ModelMgr::~ModelMgr()
{
    ClearModels();
    DeleteParVariants();
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


void ModelMgr::CreateModels()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ModelMgr::CreateModels", std::this_thread::get_id());
#endif
    ClearModels();
    for (int i=0; i<ds::NUM_MODELS; ++i)
        _models[i] = ParamModelBase::Create((ds::PMODEL)i);
}
void ModelMgr::ClearModel(ds::PMODEL mi)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ModelMgr::ClearModel", std::this_thread::get_id());
#endif
    ParamModelBase* model = _models[mi];
    while (model->rowCount()>0) model->removeRow(0);
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
void ModelMgr::ClearParameters(ds::PMODEL mi)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ModelMgr::ClearParameters", std::this_thread::get_id());
#endif
    const size_t num_rows = _models.at(mi)->NumPars();
    _models[mi]->removeRows(0, num_rows, QModelIndex());
}

void ModelMgr::DeleteParVariant(size_t idx)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ModelMgr::DeleteParVariant", std::this_thread::get_id());
#endif
    assert(idx<_parVariants.size());
    delete _parVariants[idx];
    _parVariants.erase(_parVariants.begin() + idx);
}
void ModelMgr::InsertParVariant(size_t idx, ParVariant* pv)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ModelMgr::InsertParVariant", std::this_thread::get_id());
#endif
    assert(idx<=_parVariants.size());
    auto it = _parVariants.begin() + idx;
    _parVariants.insert(it, pv);
}
void ModelMgr::RemoveParameter(ds::PMODEL mi, const std::string& key)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ModelMgr::RemoveParameter", std::this_thread::get_id());
#endif
    ParamModelBase* model = _models[mi];
    int row = model->KeyIndex(key);
    model->removeRow(row);
}

void ModelMgr::SetCondValue(size_t row, const VecStr& results)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ModelMgr::SetCondParameter", std::this_thread::get_id());
#endif
    CondModel()->SetResults(row, results);
}
void ModelMgr::SetIsFreeze(ds::PMODEL mi, size_t idx, bool is_freeze)
{
    _models[mi]->SetFreeze(idx, is_freeze);
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
void ModelMgr::SetParVariants(const std::vector<ParVariant*>& par_variants)
{
    DeleteParVariants();
    _parVariants = par_variants;
}
void ModelMgr::SetPVPar(size_t index, size_t pidx, const std::string& val)
{
    _parVariants[index]->pars[pidx].second = val;
}
void ModelMgr::SetPVInputFile(size_t index, size_t pidx, const std::string& input_file)
{
    _parVariants[index]->input_files[pidx].second = input_file;
}
void ModelMgr::SetPVNotes(size_t index, const std::string& notes)
{
    _parVariants[index]->notes = notes;
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
VecStr ModelMgr::DiffVarList() const
{
    VecStr vs,
            dkeys = Model(ds::DIFF)->ShortKeys(),
            vkeys = Model(ds::VAR)->Keys();
    vs.push_back("IP");
    vs.insert(vs.end(), dkeys.cbegin(), dkeys.cend());
    vs.insert(vs.end(), vkeys.cbegin(), vkeys.cend());
    return vs;
}
bool ModelMgr::IsFreeze(ds::PMODEL mi, size_t idx) const
{
    return _models[mi]->IsFreeze(idx);
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

ModelMgr::ModelMgr() : _diffMethod(UNKNOWN), _log(Log::Instance()), _models(MakeModelVec()),
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
void ModelMgr::DeleteParVariants()
{
    for (auto it : _parVariants)
        delete it;
}
std::vector<ParamModelBase*> ModelMgr::MakeModelVec() const
{
    return std::vector<ParamModelBase*>(ds::NUM_MODELS, nullptr);
}
