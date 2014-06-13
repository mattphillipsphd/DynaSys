#include "compilablebase.h"

CompilableBase::CompilableBase(const std::string& name, CFileBase* cfile)
    : QObject(nullptr), _log(Log::Instance()), _cFile(cfile),
      _nameRaw( MakeRawName(name) ), _path( MakePath(name) ),
    #ifdef Q_OS_WIN
        _name(name + ".cpp"),
        _nameExe(name + ".exe")
    #else
        _name(Path() + NameRaw() + ".c"),
        _nameExe(Path() + NameRaw() + ".out")
    #endif
{
}
CompilableBase::~CompilableBase()
{
    if (_cFile) delete _cFile;
}

const std::string CompilableBase::MakePath(const std::string& name) const
{
    std::string path(name);
    size_t pos = path.find_last_of('/');
    if (pos==std::string::npos) path = "./";
    else path = path.substr(0,pos+1);
    return path;
}

const std::string CompilableBase::MakeRawName(const std::string& name) const
{
    std::string raw_name( ds::StripPath(name) );
    size_t pos = raw_name.find_last_of('.');
    if (pos == std::string::npos) return raw_name;
    std::string ext = raw_name.substr(pos);
    if (ext==".c" || ext==".cpp" || ext==".o" || ext==".exe")
        raw_name.erase(pos);
    return raw_name;
}
