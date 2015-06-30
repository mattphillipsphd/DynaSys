#ifndef CFILESO_H
#define CFILESO_H

#include "cfile.h"

//This is for linux shared objects
class CFileSO : public CFile
{
    public:
        CFileSO(const std::string& name);



    protected:
        virtual void MakeHFile() override;

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
        std::string MainDecl() const;
        const std::string MakeHName(const std::string& name) const;

        const std::string _nameH;
};

#endif // CFILESO_H
