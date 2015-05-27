#ifndef MDEFSFILE_H
#define MDEFSFILE_H

#include "mfilebase.h"

class MDefsFile : public MFileBase
{
    public:
        MDefsFile(const std::string& name);
        void Make() const { MFileBase::Make(); } // ### ??? Getting compiler error without this!

    protected:
        virtual void Make(std::ofstream& out) const override;
        virtual std::string Name() const override final;
        virtual std::string NameRun() const
        {
            return "";
//            throw std::exception("MDefsFile::NameRun: Invalid function call");
        }
};

#endif // MDEFSFILE_H
