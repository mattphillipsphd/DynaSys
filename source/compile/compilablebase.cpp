#include "compilablebase.h"

CompilableBase::CompilableBase(const std::string& name, CFileBase* cfile)
    : QObject(nullptr), _log(Log::Instance()), _cFile(cfile), _nameRaw( MakeRawName(name) ),
    #ifdef Q_OS_WIN
        _name(name + ".cpp"),
        _nameExe(name + ".exe")
    #else
        _name(NameRaw() + ".c"),
        _nameExe(NameRaw() + ".out")
    #endif
{
}
CompilableBase::~CompilableBase()
{
    if (_cFile) delete _cFile;
}

const std::string CompilableBase::MakeRawName(const std::string& name)
{
    std::string out( ds::StripPath(name) );
    size_t pos = out.find_last_of('.');
    if (pos == std::string::npos) return out;
    std::string ext = out.substr(pos);
    if (ext==".c" || ext==".cpp" || ext==".o" || ext==".exe")
        out.erase(pos);
    return out;
}
