#include "cfileso.h"

CFileSO::CFileSO(const std::string& name) : CFile(name), _nameH(MakeHName(Name()))
{
}

void CFileSO::MakeHFile()
{
    std::ofstream hout;
    hout.open(_nameH);

    CFile::WriteIncludes(hout);
    hout << MainDecl() + ";\n";

    hout.close();
}

void CFileSO::WriteDataOut(std::ofstream& out, const ParamModelBase* model)
{
    out << "//Begin CFileSO::WriteDataOut\n";
    const size_t num_pars = model->NumPars();
    for (size_t i=0; i<num_pars; ++i)
        out << "            data[ col++*num_records + row_ct ] = " + model->ShortKey(i) + ";\n";
    out << "//End CFileSO::WriteDataOut\n";
    out << "    \n";
}

void CFileSO::WriteIncludes(std::ofstream& out)
{
    out << "//Begin CFileSO::WriteIncludes\n";
    out << "#include \"" + ds::StripPath(_nameH) + "\"\n";
    out << "//End CFileSO::WriteIncludes\n";
    out << "\n";
}

void CFileSO::WriteInitArgs(std::ofstream& out, const ParamModelBase* inputs,
                   const ParamModelBase* init_conds)
{
    out << "//Begin CFileSO::WriteInitArgs\n";
    const size_t num_inputs = inputs->NumPars(),
            num_ics = init_conds->NumPars();

    out <<
           "    const int num_iters = (int)( pars[0] / tau ),\n"
           "        save_mod_n = pars[1];\n"
           "\n";

    //argv[3] is the file name
    const int NUM_AUTO_ARGS = 2;
    for (size_t i=0; i<num_inputs; ++i)
        out << "    " + inputs->Key(i) + " = pars["
               + std::to_string(i+NUM_AUTO_ARGS) + "];\n";
    out << "\n";
    for (size_t i=0; i<num_ics; ++i)
        out << "    " + init_conds->ShortKey(i) + "0 = pars["
               + std::to_string(i+num_inputs+NUM_AUTO_ARGS) + "];\n";
    out << "//End CFileSO::WriteInitArgs\n";
    out << "\n";
}

void CFileSO::WriteMainBegin(std::ofstream& out)
{
    out <<
           "//Note: data must be pre-allocated by calling funtion\n"
           "" + MainDecl() + "\n"
           "{\n";
}

void CFileSO::WriteMainEnd(std::ofstream& out)
{
    out <<
           "    return 0;\n"
           "}\n";
}

void CFileSO::WriteOutputHeader(std::ofstream& out, const ParamModelBase* variables,
                                   const ParamModelBase* diffs)
{
    out << "//Begin CFileSO::WriteOutputHeader\n";
    const size_t num_fields = variables->NumPars()+diffs->NumPars();
    out <<
           "    const int num_fields = " + std::to_string(num_fields) + ",\n"
           "            num_records = num_iters / save_mod_n;\n"
           "    int row_ct = 0;\n";
    out << "//End CFileSO::WriteOutputHeader\n";
    out << "\n";
}

void CFileSO::WriteSaveBlockBegin(std::ofstream& out)
{
    out << "            int col = 0;\n";
    out << "\n";
}

void CFileSO::WriteSaveBlockEnd(std::ofstream& out)
{
    out << "            ++row_ct;\n";
    out << "\n";
}

std::string CFileSO::MainDecl() const
{
    return "int ds_model(const size_t num_pars, const double* pars, double* data)";
}

const std::string CFileSO::MakeHName(const std::string& name) const
{
    std::string out = name;
    size_t pos = out.find_last_of('.');
    if (pos!=std::string::npos) out.erase(pos);
    out += ".h";
    return out;
}
