#ifndef COMPILABLEBASE_H
#define COMPILABLEBASE_H

#include <QObject>

#include "cfilebase.h"

class CompilableBase : public QObject
{
    Q_OBJECT

    public:
        CompilableBase(const std::string& name, CFileBase* cfile);
        virtual ~CompilableBase();

        virtual void Compile(const ParserMgr& parser_mgr) = 0;

        const std::string& Name() const { return _name; }
        const std::string& NameExe() const { return _nameExe; }
        const std::string& NameRaw() const { return _nameRaw; }
        const std::string& Path() const { return _path; }

    protected:
        CFileBase* GetCFile() const { return _cFile; }
        const std::string MakePath(const std::string& name) const;
        const std::string MakeRawName(const std::string& name) const;

        Log* _log;

    private:
        CFileBase* const _cFile;
        const std::string _nameRaw, _path, _name, _nameExe;
};

#endif // COMPILABLEBASE_H
