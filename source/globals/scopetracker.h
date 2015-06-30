#ifndef SCOPETRACKER_H
#define SCOPETRACKER_H

#include <chrono>
#include <mutex>
#include <thread>

#include "log.h"

class ScopeTracker
{
    public:
        ScopeTracker(const std::string&& name, std::thread::id tid);
        ~ScopeTracker();

        static void InitThread(std::thread::id tid)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_tabCts.find(tid)==_tabCts.end())
                _tabCts[tid] = 0;
        }

        void Add(const std::string& mesg); //For adding milestones

        int ElapsedMs() const;

    private:
        inline std::string MakeTab(std::thread::id tid)
        {
            return std::string(_tabCts[tid]*ds::TABLEN, ' ');
        }

        std::string _add;
        Log* const _log;
        static std::mutex _mutex;
        const std::string _name;
        const std::chrono::steady_clock::time_point _startTime;
        static std::map<std::thread::id, int> _tabCts;
        const std::string _tab;
        const std::thread::id _tid;
};

#endif // SCOPETRACKER_H
