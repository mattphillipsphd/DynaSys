#include "executable.h"

int Executable::_jobCt = 0;

Executable::Executable(const std::string& name) : Compilable(name),
    _state(QProcess::NotRunning)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Executable::Executable", std::this_thread::get_id());
#endif
    connect(&_proc, SIGNAL(readyReadStandardError()), this, SLOT(ReadProcError()));
    connect(&_proc, SIGNAL(readyReadStandardOutput()), this, SLOT(ReadProcOutput()));
    connect(&_proc, SIGNAL(stateChanged(QProcess::ProcessState)),
            this, SLOT(StateChanged(QProcess::ProcessState)));
}
Executable::~Executable()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Executable::~Executable", std::this_thread::get_id());
#endif
}

int Executable::Launch()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Executable::Launch", std::this_thread::get_id());
#endif
    QStringList args;
    for (const auto& it : _arguments)
        args.push_back(it.c_str());

    _proc.start(NameExe().c_str(), args);

    _jobId = ++_jobCt;
    return _jobId;
}
int Executable::Launch(const VecStr& inputs)
{
    SetArguments(inputs);
    return Launch();
}
void Executable::WaitComplete()
{
    _proc.waitForFinished(-1);
}

void Executable::ReadProcError()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Executable::ReadProcError", std::this_thread::get_id());
#endif
    _log->AddMesg("Executable::ReadProcError, " + ds::StripPath(NameExe()) + ": "
                  + std::string(_proc.readAllStandardError().data()));
}

void Executable::ReadProcOutput()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Executable::ReadProcOutput", std::this_thread::get_id());
    _log->AddMesg("Executable::ReadProcOutput, " + ds::StripPath(NameExe()) + ": "
                  + std::string(_proc.readAllStandardOutput().data()));
#endif
}

void Executable::StateChanged(QProcess::ProcessState state)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Executable::StateChanged", std::this_thread::get_id());
    _log->AddMesg(ds::StripPath(NameExe()) + " process state changed, new state " + std::to_string(state));
#endif
    QProcess::ProcessState prior_state = _state;

    _state = state;

    if (state==0)
        switch (prior_state)
        {
            case QProcess::NotRunning: //0
                break;
            case QProcess::Starting: //1
                emit Finished(_jobId, false);
                break;
            case QProcess::Running: //2
                emit Finished(_jobId, true);
                break;
        }
}
