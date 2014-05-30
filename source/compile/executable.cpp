#include "executable.h"

int Executable::_jobCt = 0;

Executable::Executable(const std::string& name) : _log(Log::Instance()),
#ifdef Q_OS_WIN
    _name(name + ".cpp"),
    _nameExe(name + ".exe"),
#else
    _name(name + ".c"),
    _nameExe(name + ".out"),
#endif
    _state(QProcess::NotRunning)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Executable::Executable", std::this_thread::get_id());
#endif
    connect(&_proc, SIGNAL(readyReadStandardError()), this, SLOT(ReadProcError()));
    connect(&_proc, SIGNAL(readyReadStandardOutput()), this, SLOT(ReadProcOutput()));
    connect(&_proc, SIGNAL(stateChanged(QProcess::ProcessState)),
            this, SLOT(StateChanged(QProcess::ProcessState)));
}

void Executable::Compile(const ParserMgr& parser_mgr)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Executable::Compile", std::this_thread::get_id());
#endif
    MakeCFile(parser_mgr);

#ifdef Q_OS_WIN
    std::string setup("\"" + DDM::VCVarsDir() + "\\vcvars32.bat\"");
//    int setup_code = system(setup.c_str());
//    _log->AddMesg("Executed " + setup + ", returning code "
//                  + std::to_string(setup_code));
    std::string win_name = "\"" + _name + "\"",
            win_name_exe = "\"" + _nameExe + "\"";
    std::replace(win_name.begin(), win_name.end(), '/', '\\');
    std::replace(win_name_exe.begin(), win_name_exe.end(), '/', '\\');
    std::string cl_cmd("\"" + DDM::VCVarsDir() + "\\cl\" /O2 " + win_name + " /Fe" + win_name_exe);
    std::string cmd = setup + " && " + cl_cmd;
    QProcess proc;
    QStringList args;
    args << "/O2"
         << _name.c_str()
        << "/Fe" + QString(_nameExe.c_str());
    proc.start( "cl.exe", args );
#else
    std::string cmd("gcc -O3 -std=c11 " + _name + " -lm -o " + _nameExe);
#endif
    _log->AddMesg("Compiling " + ds::StripPath(_nameExe) + " with " + cmd);
    auto comp_start = std::chrono::system_clock::now();
#ifdef Q_OS_WIN
    int code = -1;
#else
    int code = system(cmd.c_str());
#endif
    auto dur = std::chrono::system_clock::now() - comp_start;
    int dur_ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
    _log->AddMesg(ds::StripPath(_nameExe) + " compilation ended with code " + std::to_string(code)
            + ", with a duration of " + std::to_string(dur_ms) + "ms.");
}

int Executable::Launch(const VecStr& inputs)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Executable::Launch", std::this_thread::get_id());
#endif
    QStringList args;
    for (const auto& it : inputs)
        args.push_back(it.c_str());

    _proc.start(_nameExe.c_str(), args);

    _jobId = ++_jobCt;
    return _jobId;
}

void Executable::ReadProcError()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Executable::ReadProcError", std::this_thread::get_id());
#endif
    _log->AddMesg("Executable::ReadProcError, " + ds::StripPath(_nameExe) + ": "
                  + std::string(_proc.readAllStandardError().data()));
}

void Executable::ReadProcOutput()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Executable::ReadProcOutput", std::this_thread::get_id());
    _log->AddMesg("Executable::ReadProcOutput, " + ds::StripPath(_nameExe) + ": "
                  + std::string(_proc.readAllStandardOutput().data()));
#endif
}

void Executable::StateChanged(QProcess::ProcessState state)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Executable::StateChanged", std::this_thread::get_id());
    _log->AddMesg(ds::StripPath(_nameExe) + " process state changed, new state " + std::to_string(state));
#endif
    QProcess::ProcessState prior_state = _state;

    _state = state;

    if (state==0)
        switch (prior_state)
        {
            case QProcess::NotRunning: //0
                break;
            case QProcess::Starting: //1
                emit Finished(_jobId, false);
                break;
            case QProcess::Running: //2
                emit Finished(_jobId, true);
                break;
        }
}

void Executable::MakeCFile(const ParserMgr& parser_mgr)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Executable::MakeCFile", std::this_thread::get_id());
#endif
    const ParamModelBase* inputs = parser_mgr.Model(ds::INPUTS),
            * variables = parser_mgr.Model(ds::VARIABLES),
            * diffs = parser_mgr.Model(ds::DIFFERENTIALS),
            * init_conds = parser_mgr.Model(ds::INIT_CONDS);
    const size_t num_inputs = inputs->NumPars(),
            num_vars = variables->NumPars(),
            num_diffs = diffs->NumPars(),
            num_ics = init_conds->NumPars();

    std::ofstream out;
    out.open(_name);

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

    out << "const int INPUT_SIZE = " << Input::INPUT_SIZE << ";\n";
    out << "const double tau = " << parser_mgr.ModelStep() << ";\n";
    out << "\n";

    WriteVarDecls(out, parser_mgr);

    WriteFuncs(out, variables);
    WriteFuncs(out, diffs);

    //******************Write entry to main function******************
    out <<
           "int main(int argc, char* argv[])\n"
           "{\n"
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
    out << "\n";

    //Load all input files
    WriteLoadInput(out, variables);

    //Write header information to output file
    WriteOutputHeader(out, variables, diffs);

    //Initialize variables and differentials
    for (size_t i=0; i<num_vars; ++i)
    {
        std::string value = variables->Value(i);
        if (Input::Type(value)==Input::USER || variables->IsFreeze(i))
            out << "    " + variables->ShortKey(i) + " = 0;\n";
        else
        {
            std::string inputv = "input_" + variables->ShortKey(i);
            out << "    " + variables->ShortKey(i) + " = " + inputv + "[0];\n";
        }
    }
    for (size_t i=0; i<num_diffs; ++i)
    {
        if (diffs->IsFreeze(i))
            out << "    " + diffs->ShortKey(i) + " = " + init_conds->ShortKey(i) + "0;\n";
        else
            out << "    " + diffs->ShortKey(i) + " = " + diffs->ShortKey(i) + "0;\n";
    }
    out << "\n";

    //Start the main model loop
    out <<
           "    for (int i=0; i<num_iters; ++i)\n"
           "    {\n";

    //  Execute variable and differential functions
    for (size_t i=0; i<num_vars; ++i)
    {
        if (variables->IsFreeze(i)) continue;
        std::string value = variables->Value(i);
        if (Input::Type(value)==Input::USER)
            out << "        " + variables->ShortKey(i) + "_func();\n";
        else
        {
            std::string var = variables->ShortKey(i),
                    inputv = "input_" + var,
                    spsv = "sps_" + var, //samples per step
                    samps_ct = "ct_" + var;
            out <<
                   "        if (i % " + spsv + " == 0)\n"
                   "            " + var + " = " + inputv + "[++" + samps_ct + "];\n"
                   "        \n";
        }
    }
    for (size_t i=0; i<num_vars; ++i)
        if (Input::Type(variables->Value(i))==Input::USER && !variables->IsFreeze(i))
            out << "        " + variables->ShortKey(i) + " = " + variables->TempKey(i) + ";\n";
    out << "\n";
    for (size_t i=0; i<num_diffs; ++i)
        if (!diffs->IsFreeze(i))
            out << "        " + diffs->ShortKey(i) + "_func();\n";
    for (size_t i=0; i<num_diffs; ++i)
        if (!diffs->IsFreeze(i))
            out << "        " + diffs->ShortKey(i) + " = " + diffs->TempKey(i) + ";\n";

    //  Check whether to save data
    out <<
           "    \n"
           "        if (i%save_mod_n==0)\n"
           "        {\n";

    WriteDataToFile(out, variables);
    WriteDataToFile(out, diffs);

    out << "        }\n";
    WriteConditions(out, parser_mgr.Conditions());

    out << "    }\n"
           "    \n"
           "    fclose(fp);\n"
           "    fprintf(stderr, \"" + ds::StripPath(_nameExe) + " exited without crashing.\\n\");\n"
           "    return 0;\n"
           "}\n";

    out.close();
}

std::string Executable::PreprocessExprn(const std::string& exprn) const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Executable::PreprocessExprn", std::this_thread::get_id());
#endif
    std::string temp = exprn; //Gets progressively processed until it's proper C.

    ds::RemoveWhitespace(temp);

    const std::string OPS = "<>?:!%&|~=+-*/";
    size_t posc = temp.find_first_of('^');
    while (posc != std::string::npos)
    {
        const size_t len = temp.length();
        std::string base = temp.substr(0, posc);
        size_t base_begin = 0;
        int paren = 0;
        for (size_t i=posc-1; i<std::numeric_limits<size_t>::max(); --i)
        {
            char c = temp.at(i);
            if (paren==0 && OPS.find_first_of(c)!=std::string::npos)
            {
                base = temp.substr(i+1,posc-i-1);
                base_begin = i+1;
                break;
            }
            else if (c==')')
                ++paren;
            else if (c=='(')
            {
                --paren;
                if (paren==0)
                {
                    base = temp.substr(i,posc-i);
                    base_begin = i;
                    break;
                }
            }
        }

        std::string expt = temp.substr(posc+1);
        size_t expt_end = len;
        paren = 0;
        for (size_t i=posc+1; i<len; ++i)
        {
            char c = temp.at(i);
            if (paren==0 && OPS.find_first_of(c)!=std::string::npos)
            {
                if (c=='-' && i==posc+1) continue; //unary minus
                expt = temp.substr(posc+1,i-posc);
                expt_end = i+1;
                break;
            }
            else if (c=='(')
                ++paren;
            else if (c==')')
            {
                --paren;
                if (paren==0)
                {
                    expt = temp.substr(posc+1,i-posc);
                    expt_end = i+1;
                    break;
                }
            }
        }

        temp.replace(base_begin, expt_end-base_begin, "exp(" + base + "," + expt + ")");
        posc = temp.find_first_of('^');
    }
    return temp;
}

void Executable::WriteConditions(std::ofstream& out, const ConditionModel* conditions)
{
    const size_t num_conds = conditions->NumPars();
    for (size_t i=0; i<num_conds; ++i)
    {
        out <<
               "        if (" + conditions->Condition((int)i) + ")\n"
               "        {\n";

        const VecStr expressions = conditions->Expressions((int)i);
        for (const auto& it : expressions)
            out << "            " + it + ";\n";

        out << "        }\n";
    }
}
void Executable::WriteFuncs(std::ofstream& out, const ParamModelBase* model)
{
    const size_t num_pars = model->NumPars();
    for (size_t i=0; i<num_pars; ++i)
    {
        std::string value = model->Value(i);
        if (Input::Type(value)==Input::INPUT_FILE) continue;
        out <<
               "inline void " + model->ShortKey(i) + "_func()\n"
               "{\n"
               "    " + PreprocessExprn( model->TempExpression(i) ) + ";\n"
               "}\n";
    }
    out << "\n";
}
void Executable::WriteDataToFile(std::ofstream& out, const ParamModelBase* model)
{
    const size_t num_pars = model->NumPars();
    for (size_t i=0; i<num_pars; ++i)
        out << "            fwrite(&" + model->ShortKey(i) + ", sizeof(double), 1, fp);\n";
    out << "    \n";
}
void Executable::WriteLoadInput(std::ofstream& out, const ParamModelBase* variables)
{
    out << "    size_t br;\n";
    out << "    int num_elts;\n";
    const size_t num_vars = variables->NumPars();
    for (size_t i=0; i<num_vars; ++i)
    {
        std::string value = variables->Value(i);
//#ifdef Q_OS_WIN
//        std::replace(value.begin(), value.end(), '/', '\\');
//#endif
        if (Input::Type(value) != Input::INPUT_FILE) continue;
        std::string var = variables->ShortKey(i),
                fpv = "fp_" + var,
                inputv = "input_" + var,
                spsv = "sps_" + var, //samples per step
                samps_ct = "ct_" + var;
        out <<
               "    FILE* " + fpv + " = fopen(" + value + ", \"rb\");\n"
               "    if (" + fpv + "==0) return 1;\n"
               "    int vnum_" + var + ";\n"
               "    br = fread(&vnum_" + var + ", sizeof(int), 1, " + fpv + ");\n"
               "    int " + spsv + ", " + samps_ct + " = 0;\n"
               "    br = fread(&" + spsv + ", sizeof(int), 1, " + fpv + ");\n"
               "    " + spsv + " = (int)(1.0/(tau * (double)" + spsv + ") + 0.5);\n"
               "    br = fread(&num_elts, sizeof(int), 1, " + fpv + ");\n"
               "    double* " + inputv + " = (double*)malloc(INPUT_SIZE*sizeof(double));\n"
#ifdef Q_OS_WIN
               "    br = fread(" + inputv
                        + ", sizeof(double), (int)std::min(INPUT_SIZE, num_elts), " + fpv + ");\n"
#else
               "    br = fread(" + inputv
                        + ", sizeof(double), (int)fmin(INPUT_SIZE, num_elts), " + fpv + ");\n"
#endif
               "    fclose(" + fpv + ");\n"
               "    \n";
    }
}
void Executable::WriteOutputHeader(std::ofstream& out, const ParamModelBase* variables,
                                   const ParamModelBase* diffs)
{
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
    WriteVarsToFile(out, variables);
    WriteVarsToFile(out, diffs);
    out <<
           "    fwrite(&save_mod_n, sizeof(int), 1, fp);\n"
           "    \n"
           "    const int num_records = num_iters / save_mod_n;\n"
           "    fwrite(&num_records, sizeof(int), 1, fp);\n"
           "    \n";
}
void Executable::WriteVarDecls(std::ofstream& out, const ParserMgr& parser_mgr)
{
    const ParamModelBase* init_conds = parser_mgr.Model(ds::INIT_CONDS);
    const size_t num_ics = init_conds->NumPars();
    for (size_t i=0; i<num_ics; ++i)
        out << "double " + init_conds->ShortKey(i) + "0;\n";
    out << "\n";

    const ParamModelBase* inputs = parser_mgr.Model(ds::INPUTS);
    const size_t num_inputs = inputs->NumPars();
    for (size_t i=0; i<num_inputs; ++i)
        out << "double " + inputs->ShortKey(i) + ";\n";
    out << "\n";

    const ParamModelBase* variables = parser_mgr.Model(ds::VARIABLES);
    const size_t num_vars = variables->NumPars();
    for (size_t i=0; i<num_vars; ++i)
    {
        out << "double " + variables->ShortKey(i) + ";\n";
        if ( Input::Type(variables->Value(i)) == Input::USER && !variables->IsFreeze(i) )
            out << "double " + variables->TempKey(i) + ";\n";
    }
    out << "\n";

    const ParamModelBase* diffs = parser_mgr.Model(ds::DIFFERENTIALS);
    const size_t num_diffs = diffs->NumPars();
    for (size_t i=0; i<num_diffs; ++i)
    {
        out << "double " + diffs->ShortKey(i) + ";\n";
        if (!diffs->IsFreeze(i))
            out << "double " + diffs->TempKey(i) + ";\n";
    }
    out << "\n";
}
void Executable::WriteVarsToFile(std::ofstream& out, const ParamModelBase* variables)
{
    const size_t num_pars = variables->NumPars();
    for (size_t i=0; i<num_pars; ++i)
    {
        out << "    len = " + std::to_string(variables->ShortKey(i).length()) + ";\n";
        out << "    fwrite(&len, sizeof(int), 1, fp);\n";
        out << "    fputs(\"" + variables->ShortKey(i) + "\", fp);\n";
    }
    out << "    \n";
}
