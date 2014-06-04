#include "cfile.h"

CFile::CFile(const std::string& name) : CFileBase(name)
{
}

void CFile::WriteDataOut(std::ofstream& out, const ParamModelBase* model)
{
    out << "//Begin WriteDataOut\n";
    const size_t num_pars = model->NumPars();
    for (size_t i=0; i<num_pars; ++i)
        out << "            fwrite(&" + model->ShortKey(i) + ", sizeof(double), 1, fp);\n";
    out << "//End WriteDataOut\n";
    out << "    \n";
}

void CFile::WriteIncludes(std::ofstream& out)
{
    out <<
#ifdef Q_OS_WIN
           "#include <cstdio>\n"
           "#include <cstdlib>\n"
           "#include <cmath>\n"
           "#include <algorithm>\n"
#else
           "#include \"stdio.h\"\n"
           "#include \"stdlib.h\"\n"
           "#include \"math.h\"\n"
#endif
           "\n";
}

void CFile::WriteInitArgs(std::ofstream& out, const ParamModelBase* inputs,
                   const ParamModelBase* init_conds)
{
    out << "//Begin WriteInitArgs\n";
    const size_t num_inputs = inputs->NumPars(),
            num_ics = init_conds->NumPars();

    out <<
           "    const int num_iters = (int)( atof(argv[1]) / tau ),\n"
           "        save_mod_n = atoi(argv[2]);\n"
           "\n";

    //argv[3] is the file name
    const int NUM_AUTO_ARGS = 4;
    for (size_t i=0; i<num_inputs; ++i)
        out << "    " + inputs->Key(i) + " = atof(argv["
               + std::to_string(i+NUM_AUTO_ARGS) + "]);\n";
    out << "\n";
    for (size_t i=0; i<num_ics; ++i)
        out << "    " + init_conds->ShortKey(i) + "0 = atof(argv["
               + std::to_string(i+num_inputs+NUM_AUTO_ARGS) + "]);\n";
    out << "//End WriteInitArgs\n";
    out << "\n";
}

void CFile::WriteMainBegin(std::ofstream& out)
{
    out <<
           "int main(int argc, char* argv[])\n"
           "{\n";
}

void CFile::WriteMainEnd(std::ofstream& out)
{
    out <<
           "    fclose(fp);\n"
           "    fprintf(stderr, \"" + ds::StripPath(Name()) + " exited without crashing.\\n\");\n"
           "    return 0;\n"
           "}\n";
}

void CFile::WriteOutputHeader(std::ofstream& out, const ParamModelBase* variables,
                                   const ParamModelBase* diffs)
{
    out << "//Begin WriteOutputHeader\n";
    out <<
           "    FILE* fp;\n"
           "    fp = fopen(argv[3], \"wb\");\n"
           "    if (fp==0) return 1;"
           "    \n"
           "    const int vnum = " + std::to_string(ds::VersionNum()) + ";\n"
           "    fwrite(&vnum, sizeof(int), 1, fp);\n"
           "    \n"
           "    const int num_fields = "
                    + std::to_string(variables->NumPars() + diffs->NumPars()) + ";\n"
           "    fwrite(&num_fields, sizeof(int), 1, fp);\n"
           "    \n";

    out << "    int len;\n";
    WriteVarsOut(out, variables);
    WriteVarsOut(out, diffs);
    out <<
           "    fwrite(&save_mod_n, sizeof(int), 1, fp);\n"
           "    \n"
           "    const int num_records = num_iters / save_mod_n;\n"
           "    fwrite(&num_records, sizeof(int), 1, fp);\n";
    out << "//End WriteOutputHeader\n";
    out << "\n";
}

void CFile::WriteVarsOut(std::ofstream& out, const ParamModelBase* variables)
{
    out << "//Begin WriteVarsOut\n";
    const size_t num_pars = variables->NumPars();
    for (size_t i=0; i<num_pars; ++i)
    {
        out << "    len = " + std::to_string(variables->ShortKey(i).length()) + ";\n";
        out << "    fwrite(&len, sizeof(int), 1, fp);\n";
        out << "    fputs(\"" + variables->ShortKey(i) + "\", fp);\n";
    }
    out << "//End WriteVarsOut\n";
    out << "    \n";
}
