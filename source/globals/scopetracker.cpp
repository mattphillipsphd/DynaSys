#include "scopetracker.h"

std::mutex ScopeTracker::_mutex;
std::map<std::thread::id, int> ScopeTracker::_tabCts;

