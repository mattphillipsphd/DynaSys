#include "scopetracker.h"

std::mutex ScopeTracker::_mutex;
std::map<std::thread::id, int> ScopeTracker::_tabCts;

ScopeTracker::ScopeTracker(const std::string&& name, std::thread::id tid)
    : _log(Log::Instance()), _name(name), _startTime(std::chrono::steady_clock::now()),
      _tab(MakeTab(tid)), _tid(tid)
{
    _log->AddMesg(_tab + "Entered " + _name);
    std::lock_guard<std::mutex> lock(_mutex);
    ++_tabCts[_tid];
}
ScopeTracker::~ScopeTracker()
{
    _log->AddMesg(_tab + "Exited " + _name + _add
                  + " after " + std::to_string(ElapsedMs()) + "ms");
    std::lock_guard<std::mutex> lock(_mutex);
    --_tabCts[_tid];
}

void ScopeTracker::Add(const std::string& mesg)
{
    _add += ", " + mesg;
}

int ScopeTracker::ElapsedMs() const
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - _startTime).count();
}
