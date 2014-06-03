#ifndef MEXFILE_H
#define MEXFILE_H

#include "cfilebase.h"

class MEXFile : public CFileBase
{
    public:
        MEXFile(const std::string& name);

        void MakeMFile(const ParserMgr& parser_mgr);

    protected:
        virtual void WriteDataOut(std::ofstream& out, const ParamModelBase* model) override;
        virtual void WriteIncludes(std::ofstream& out) override;
        virtual void WriteInitArgs(std::ofstream& out, const ParamModelBase* inputs,
                           const ParamModelBase* init_conds) override;
        virtual void WriteMainBegin(std::ofstream& out) override;
        virtual void WriteMainEnd(std::ofstream& out) override;
        virtual void WriteOutputHeader(std::ofstream& out, const ParamModelBase* variables,
                                           const ParamModelBase* diffs) override;
        virtual void WriteSaveBlockBegin(std::ofstream& out) override;
        virtual void WriteSaveBlockEnd(std::ofstream& out) override;

    private:
        std::string MakeMName(const std::string& name) const;

        const std::string _nameM;
};

#endif // MEXFILE_H
