#ifndef MDEFSFILE_H
#define MDEFSFILE_H

#include "mfilebase.h"

class MDefsFile : public MFileBase
{
    public:
        MDefsFile(const std::string& name);
        void SetMeasure(const std::string& measure) { _measure = measure; }
        void Make() const { MFileBase::Make(); } // ### ??? Getting compiler error without this!

    protected:
        virtual void Make(std::ofstream& out) const override;
        virtual std::string Name() const override final;
        virtual std::string NameRun() const
        {
            return "";
//            throw std::exception("MDefsFile::NameRun: Invalid function call");
        }

    private:
        std::string _measure;
};

#endif // MDEFSFILE_H
