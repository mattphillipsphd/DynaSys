#include "cfile.h"

CFile::CFile(const std::string& name)
  #ifdef Q_OS_WIN
      : CFileBase(name, ".cpp")
  #else
      : CFileBase(name, ".c")
  #endif
{
}

void CFile::WriteDataOut(std::ofstream& out, ds::PMODEL mi)
{
    out << "//Begin CFile::WriteDataOut\n";
    const ParamModelBase* model = _modelMgr->Model(mi);
    const size_t num_pars = model->NumPars();
    for (size_t i=0; i<num_pars; ++i)
        out << "            fwrite(&" + model->ShortKey(i) + ", sizeof(double), 1, fp);\n";
    out << "//End CFile::WriteDataOut\n";
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
           "#include \"math.h\"\n"
           "#include \"stdio.h\"\n"
           "#include \"stdlib.h\"\n"
#endif
           "\n";
}

void CFile::WriteInitArgs(std::ofstream& out)
{
    out << "//Begin CFile::WriteInitArgs\n";
    const ParamModelBase* inputs = _modelMgr->Model(ds::INP),
            * init_conds = _modelMgr->Model(ds::INIT);
    const size_t num_inputs = inputs->NumPars(),
            num_ics = init_conds->NumPars();

    out <<
           "    const int num_iters = (int)( atof(argv[1]) / tau + 0.5),\n"
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
    out << "//End CFile::WriteInitArgs\n";
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

void CFile::WriteOutputHeader(std::ofstream& out)
{
    out << "//Begin CFile::WriteOutputHeader\n";
    const ParamModelBase* variables = _modelMgr->Model(ds::FUNC),
            * diffs = _modelMgr->Model(ds::STATE);
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
    WriteVarsOut(out, ds::FUNC);
    WriteVarsOut(out, ds::STATE);
    out <<
           "    fwrite(&save_mod_n, sizeof(int), 1, fp);\n"
           "    \n"
           "    const int num_records = num_iters / save_mod_n;\n"
           "    fwrite(&num_records, sizeof(int), 1, fp);\n";
    out << "//End CFile::WriteOutputHeader\n";
    out << "\n";
}

void CFile::WriteVarsOut(std::ofstream& out, ds::PMODEL mi)
{
    out << "//Begin CFile::WriteVarsOut\n";
    const ParamModelBase* model = _modelMgr->Model(mi);
    const size_t num_pars = model->NumPars();
    for (size_t i=0; i<num_pars; ++i)
    {
        out << "    len = " + std::to_string(model->ShortKey(i).length()) + ";\n";
        out << "    fwrite(&len, sizeof(int), 1, fp);\n";
        out << "    fputs(\"" + model->ShortKey(i) + "\", fp);\n";
    }
    out << "//End CFile::WriteVarsOut\n";
    out << "    \n";
}
