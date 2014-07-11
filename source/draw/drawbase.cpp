#include "drawbase.h"
#include "nullcline.h"
#include "phaseplot.h"
#include "timeplot.h"
#include "variableview.h"
#include "vectorfield.h"

const int DrawBase::MAX_BUF_SIZE = 8 * 1024 * 1024;

DrawBase* DrawBase::Create(DRAW_TYPE draw_type, DSPlot* plot)
{
    DrawBase* draw_object(nullptr);
    switch (draw_type)
    {
        case NULL_CLINE:
            draw_object = new Nullcline(plot);
            draw_object->_drawType = NULL_CLINE;
            break;
        case PHASE_PLOT:
            draw_object = new PhasePlot(plot);
            draw_object->_drawType = PHASE_PLOT;
            break;
        case TIME_PLOT:
            draw_object = new TimePlot(plot);
            draw_object->_drawType = TIME_PLOT;
            break;
        case VARIABLE_VIEW:
            draw_object = new VariableView(plot);
            draw_object->_drawType = VARIABLE_VIEW;
            break;
        case VECTOR_FIELD:
            draw_object = new VectorField(plot);
            draw_object->_drawType = VECTOR_FIELD;
            break;
    }
    connect(draw_object, SIGNAL(ComputeComplete()), draw_object, SLOT(IterCompleted()));
    return draw_object;
}

DrawBase::~DrawBase()
{
    for (auto it : _plotItems)
    {
        it->detach();
        delete it;
    }
}

void DrawBase::Initialize()
{
    if (!_plot)
        throw std::runtime_error("DrawBase::Initialize: Null plot object");
    for (auto it : _plotItems)
        it->attach(_plot);
}
void DrawBase::QuickEval(const std::string& exprn)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DrawBase::QuickEval", std::this_thread::get_id());
#endif
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto& it : _parserMgrs)
        it.QuickEval(exprn);
}

void DrawBase::SetNeedRecompute(bool need_update_parser)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _needRecompute = need_update_parser;
}
void DrawBase::SetOpaqueSpec(const std::string& key, const void* value)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _opaqueSpecs[key] = value;
}
void DrawBase::SetSpec(const std::string& key, const std::string& value)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _specs[key] = value;
}
void DrawBase::SetSpec(const std::string& key, double value)
{
    SetSpec(key, std::to_string(value));
}
void DrawBase::SetSpec(const std::string& key, int value)
{
    SetSpec(key, std::to_string(value));
}
void DrawBase::SetSpecs(const MapStr& specs)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _specs = specs;
}

size_t DrawBase::NumPlotItems() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _plotItems.size();
}
size_t DrawBase::NumParserMgrs() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _parserMgrs.size();
}
const void* DrawBase::OpaqueSpec(const std::string& key) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _opaqueSpecs.at(key);
}
const std::string& DrawBase::Spec(const std::string& key) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _specs.at(key);
}
double DrawBase::Spec_tod(const std::string& key) const
{
    try
    {
        return std::stod(Spec(key));
    }
    catch (std::exception&)
    {
        _log->AddExcept("DrawBase::Spec_tod: Bad value, " + Spec(key));
        emit Error();
        return 0;
    }
}
int DrawBase::Spec_toi(const std::string& key) const
{
    try
    {
        return std::stoi(Spec(key));
    }
    catch (std::exception&)
    {
        _log->AddExcept("DrawBase::Spec_toi: Bad value, " + Spec(key));
        emit Error();
        return 0;
    }
}
const MapStr& DrawBase::Specs() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _specs;
}

void DrawBase::IterCompleted() //slot
{
    if (++_iterCt == _iterMax)
        _drawState = STOPPED;
}

DrawBase::DrawBase(DSPlot* plot)
    : _log(Log::Instance()), _modelMgr(ModelMgr::Instance()),
      _data(nullptr), _iterCt(0), _iterMax(-1), _lastStep(std::chrono::system_clock::now()),
      _plot(plot)
{
}

void DrawBase::ClearPlotItems()
{
    std::lock_guard<std::mutex> lock(_mutex);
    DetachItems();
    _plotItems.clear(); //Qwt deletes these for you
}
void DrawBase::RecomputeIfNeeded()
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_needRecompute)
    {
        for (auto& it : _parserMgrs)
        {
            it.SetExpressions();
            it.SetConditions();
        }
        _needRecompute = false;
    }
}
void DrawBase::ReservePlotItems(size_t num)
{
    std::lock_guard<std::mutex> lock(_mutex);
    DetachItems();
    _plotItems.clear();
    _plotItems.reserve(num);
}

void DrawBase::AddPlotItem(QwtPlotItem* plot_item)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _plotItems.push_back(plot_item);
}

void DrawBase::InitParserMgrs(size_t num)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _parserMgrs.resize(num);
    for (auto& it : _parserMgrs)
        it.InitializeFull();
}
bool DrawBase::NeedNewStep()
{
    int steps_per_sec = Spec_toi("steps_per_sec");
    auto step_diff = std::chrono::system_clock::now() - _lastStep;
    auto diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(step_diff);
    bool need_new_step = diff_ms.count() > 1000.0/((double)steps_per_sec/_modelMgr->ModelStep());
    if (need_new_step) _lastStep = std::chrono::system_clock::now();
    return need_new_step;
}
int DrawBase::RemainingSleepMs() const
{
    auto step_diff = std::chrono::system_clock::now() - _lastStep;
    auto diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(step_diff);
    int rem = SleepMs() - diff_ms.count();
    return (rem>0) ? rem : 0;
}

void DrawBase::DetachItems()
{
    for (auto it : _plotItems)
        it->detach();
}
