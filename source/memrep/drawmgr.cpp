#include "drawmgr.h"

DrawMgr* DrawMgr::_instance = nullptr;

DrawMgr* DrawMgr::Instance()
{
    if (!_instance) _instance = new DrawMgr();
    return _instance;
}

DrawMgr::~DrawMgr()
{
    ClearObjects();
    delete _instance;
}

void DrawMgr::AddObject(DrawBase* object)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DrawMgr::AddObject", std::this_thread::get_id());
#endif
    std::lock_guard<std::mutex> lock(_mutex);
    _objects.push_back(object);
    connect(object, SIGNAL(ReadyToDelete()), this, SLOT(Erase()));
}
void DrawMgr::ClearObjects()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DrawMgr::ClearObjects", std::this_thread::get_id());
#endif
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto it : _objects)
        delete it;
    _objects.clear();
}
void DrawMgr::MakePlotItems()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DrawMgr::MakePlotItems", std::this_thread::get_id());
#endif
    try
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto it : _objects)
            if (it->DrawState() == DrawBase::DRAWING)
                it->MakePlotItems();
    }
    catch (std::runtime_error& e)
    {
        _log->AddExcept("DrawMgr::MakePlotItems: " + std::string(e.what()));
        throw(e);
    }
}
void DrawMgr::Pause()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DrawMgr::Pause", std::this_thread::get_id());
#endif
    _drawState = DrawBase::PAUSED;
    BroadcastDrawState();
}
void DrawMgr::QuickEval(const std::string& exprn)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DrawMgr::QuickEval", std::this_thread::get_id());
#endif
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto it : _objects)
        it->QuickEval(exprn);
}
void DrawMgr::Resume()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DrawMgr::Resume", std::this_thread::get_id());
#endif
    if (_drawState==DrawBase::DRAWING) return;
    StartThreads();
}
void DrawMgr::Resume(DrawBase::DRAW_TYPE draw_type, int iter_max)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DrawMgr::Resume", std::this_thread::get_id());
#endif
    DrawBase* obj = GetObject(draw_type);
    if (!obj || obj->IterCt()==0 || obj->DrawState()==DrawBase::DRAWING) return;
    std::thread t( std::bind(&DrawMgr::StartThread, this, obj, iter_max) );
    t.detach();
}
void DrawMgr::Start()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DrawMgr::Start", std::this_thread::get_id());
#endif
    if (_drawState==DrawBase::DRAWING) return;
    try
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto it : _objects)
        {
            it->ClearPlotItems();
            it->Initialize();
        }
    }
    catch (std::exception& e)
    {
        _log->AddExcept("DrawMgr::StartThread: " + std::string(e.what()));
        emit Error();
        return;
    }
    StartThreads();
}
void DrawMgr::Start(DrawBase::DRAW_TYPE draw_type, int iter_max)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DrawMgr::Start", std::this_thread::get_id());
#endif
    DrawBase* obj = GetObject(draw_type);
    obj->ClearPlotItems();
    obj->Initialize();
    std::thread t( std::bind(&DrawMgr::StartThread, this, obj, iter_max) );
    t.detach();
}
void DrawMgr::Stop()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DrawMgr::Stop", std::this_thread::get_id());
#endif
    _drawState = DrawBase::STOPPED;
    BroadcastDrawState();
}
void DrawMgr::StopAndRemove(DrawBase::DRAW_TYPE draw_type)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DrawMgr::StopAndRemove", std::this_thread::get_id());
#endif
    DrawBase* obj = GetObject(draw_type);
    if (obj)
    {
        if (obj->DrawState()==DrawBase::DRAWING)
        {
            obj->SetDeleteOnFinish(true);
            obj->SetDrawState(DrawBase::STOPPED);
        }
        else
        {
            std::lock_guard<std::mutex> lock(_mutex);
            std::remove(_objects.begin(), _objects.end(), obj);
        }
    }
}

void DrawMgr::SetGlobalSpec(const std::string& key, const std::string& value)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DrawMgr::SetGlobalSpec", std::this_thread::get_id());
#endif
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto it : _objects)
        it->SetSpec(key, value);
}
void DrawMgr::SetGlobalSpec(const std::string& key, int value)
{
    SetGlobalSpec(key, std::to_string(value));
}
void DrawMgr::SetNeedRecompute()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DrawMgr::SetNeedRecompute", std::this_thread::get_id());
#endif
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto it : _objects)
        it->SetNeedRecompute(true);
}

DrawBase::DRAW_STATE DrawMgr::DrawState() const
{
    return _drawState;
}
DrawBase* DrawMgr::GetObject(DrawBase::DRAW_TYPE draw_type)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = std::find_if(_objects.begin(), _objects.end(), [=](DrawBase* obj)
    {
            return obj->Type() == draw_type;
    });
    return (it == _objects.end()) ? nullptr : *it;
}
DrawBase* DrawMgr::GetObject(size_t idx)
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _objects[idx];
}
int DrawMgr::NumDrawObjects() const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DrawMgr::NumDrawObjects", std::this_thread::get_id());
#endif
    std::lock_guard<std::mutex> lock(_mutex);
    return _objects.size();
}

void DrawMgr::Erase() //slot
{
    std::lock_guard<std::mutex> lock(_mutex);
    QObject* dobj = sender();
    _objects.erase( std::find(_objects.begin(), _objects.end(), dobj) );
}

DrawMgr::DrawMgr() : _drawState(DrawBase::STOPPED), _log(Log::Instance())
{
}
void DrawMgr::BroadcastDrawState()
{
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto it : _objects)
        it->SetDrawState(_drawState);
}
void DrawMgr::StartThread(DrawBase* obj, int iter_max)
{
    ds::AddThread(std::this_thread::get_id());
#ifdef DEBUG_FUNC
    ScopeTracker::InitThread(std::this_thread::get_id());
    ScopeTracker st("DrawMgr::StartThread", std::this_thread::get_id());
#endif
    try
    {
        obj->SetDrawState(DrawBase::DRAWING);
        obj->ResetIterCt();
        obj->SetIterMax(iter_max==-1 ? std::numeric_limits<int>::max() : iter_max);
        obj->ComputeData();
    }
    catch (std::exception& e)
    {
        ds::RemoveThread(std::this_thread::get_id());
        _log->AddExcept("DrawMgr::StartThread: " + std::string(e.what()));
        emit Error();
    }

    ds::RemoveThread(std::this_thread::get_id());
}
void DrawMgr::StartThreads()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("DrawMgr::StartThreads", std::this_thread::get_id());
#endif
    std::lock_guard<std::mutex> lock(_mutex);
    _drawState = DrawBase::DRAWING;
    for (auto it : _objects)
    {
        std::thread t( std::bind(&DrawMgr::StartThread, this, it, -1) );
        t.detach();
    }
}
