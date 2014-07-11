#ifndef SCOPETRACKER_H
#define SCOPETRACKER_H

#include <mutex>
#include <thread>

#include "log.h"

class ScopeTracker
{
    public:
        ScopeTracker(const std::string&& name, std::thread::id tid)
            : _log(Log::Instance()), _name(name), _tab(MakeTab(tid)), _tid(tid)
        {
            _log->AddMesg(_tab + "Entered " + _name);
            std::lock_guard<std::mutex> lock(_mutex);
            ++_tabCts[_tid];
        }
        ~ScopeTracker()
        {
            _log->AddMesg(_tab + "Exited " + _name + _add);
            std::lock_guard<std::mutex> lock(_mutex);
            --_tabCts[_tid];
        }

        static void InitThread(std::thread::id tid)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_tabCts.find(tid)==_tabCts.end())
                _tabCts[tid] = 0;
        }

        void Add(const std::string& mesg); //For adding milestones

    private:
        inline std::string MakeTab(std::thread::id tid)
        {
            return std::string(_tabCts[tid]*ds::TABLEN, ' ');
        }

        std::string _add;
        Log* const _log;
        static std::mutex _mutex;
        const std::string _name;
        static std::map<std::thread::id, int> _tabCts;
        const std::string _tab;
        const std::thread::id _tid;
};

#endif // SCOPETRACKER_H
