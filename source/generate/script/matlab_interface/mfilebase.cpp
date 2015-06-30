#include "mfilebase.h"

const int MFileBase::NUM_AUTO_ARGS = 2;

MFileBase::MFileBase(const std::string& name)
    : _log(Log::Instance()), _modelMgr(ModelMgr::Instance()),_name(MakeName(name))
{
}

void MFileBase::Make() const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MFileBase::Make", std::this_thread::get_id());
#endif
    std::ofstream out;
    out.open( Name() );
    Make(out);
    out.close();
}

std::string MFileBase::NameDefs() const
{
    std::string out(_name);
    size_t pos = out.find_last_of('.');
    if (pos!=std::string::npos) out.erase(pos);
    out += "_defs.m";
    return out;
}

std::string MFileBase::NameExec() const
{
    std::string out(_name);
    size_t pos = out.find_last_of('.');
    if (pos!=std::string::npos) out.erase(pos);
    out += _suffix;
    return out;
}

std::string MFileBase::NameRun() const
{
    std::string out(NameExec());
    size_t pos = out.find_last_of('.');
    if (pos!=std::string::npos) out.erase(pos);
    out += "_run.m";
    return out;
}

std::string MFileBase::MakeName(const std::string& name) const
{
    std::string out(name);
    const size_t pos = out.find_last_of('.');
    if (pos!=std::string::npos)
        out.erase(pos);
    return out;
}

