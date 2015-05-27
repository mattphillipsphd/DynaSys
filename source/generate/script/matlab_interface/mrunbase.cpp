#include "mrunbase.h"

MRunBase::MRunBase(const std::string& name)
    : MFileBase(name)
{
}

std::string MRunBase::Name() const
{
    return NameRun();
}

void MRunBase::WriteDefaultPars(std::ofstream& out) const
{
    out <<
            "if nargin<1 || isempty(num_records), num_records = 1000; end\n"
           "if nargin<2 || isempty(save_mod_n), save_mod_n = 1; end\n"
            "\n";
}
