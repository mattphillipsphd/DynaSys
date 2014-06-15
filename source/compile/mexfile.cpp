#include "mexfile.h"

const int MEXFile::NUM_AUTO_ARGS = 2;

MEXFile::MEXFile(const std::string& name) : CFileBase(name), _inputCt(0), _nameM(MakeMName(name))
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MEXFile::MEXFile", std::this_thread::get_id());
#endif
}

void MEXFile::MakeMFile(const ParserMgr& parser_mgr)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MEXFile::MakeMFile", std::this_thread::get_id());
#endif
    std::ofstream mout;
    mout.open(_nameM);

    const ParamModelBase* inputs = parser_mgr.Model(ds::INPUTS),
            * init_conds = parser_mgr.Model(ds::INIT_CONDS),
            * variables = parser_mgr.Model(ds::VARIABLES),
            * diffs = parser_mgr.Model(ds::DIFFERENTIALS);
    const size_t num_inputs = inputs->NumPars(),
            num_ics = init_conds->NumPars(),
            num_columns = num_inputs+num_ics,
            num_vars = variables->NumPars(),
            num_diffs = diffs->NumPars(),
            num_input_files = static_cast<const VariableModel*>(variables)
                                ->TypeCount(Input::INPUT_FILE);

    std::string name_m = ds::StripPath(_nameM);
    name_m.erase(name_m.find_last_of('.'));

    mout <<
            "function [input_names, inputs, output_names, data] = "
                    + name_m + "(num_records, save_mod_n";
    for (size_t i=0, ct=0; i<num_vars; ++i)
        if (!variables->IsFreeze(i) && Input::Type(variables->Value(i))==Input::INPUT_FILE)
        {
            std::string si = std::to_string(ct++);
            mout << ", input" + si + ", sput" + si;
        }
    mout << ")\n"
            "\n";

    //All the input parameter names
    mout <<
           "input_names = cell(" + std::to_string(NUM_AUTO_ARGS+num_columns+num_input_files) + ",1);\n"
           "input_names{1} = 'num_records';\n"
           "input_names{2} = 'save_mod_n';\n";
    size_t inn_ct = NUM_AUTO_ARGS+1;
    for (size_t i=0; i<num_inputs; ++i, ++inn_ct)
        mout << "input_names{" + std::to_string(inn_ct) + "} = '" + inputs->Key(i) + "';\n";
    for (size_t i=0; i<num_ics; ++i, ++inn_ct)
        mout << "input_names{" + std::to_string(inn_ct) + "} = '" + init_conds->ShortKey(i) + "0';\n";
    for (size_t i=0; i<num_vars; ++i)
        if (!variables->IsFreeze(i) && Input::Type(variables->Value(i))==Input::INPUT_FILE)
        {
            mout << "input_names{" + std::to_string(inn_ct++) + "} = 'input_" + variables->Key(i) + "';\n";
            mout << "input_names{" + std::to_string(inn_ct++) + "} = 'sput_" + variables->Key(i) + "';\n";
        }
    mout << "\n";

    //All the inputs
    mout <<
           "if nargin<2\n"
           "    num_records = -1;\n"
           "    save_mod_n = -1;\n";
    for (size_t i=0; i<num_input_files; ++i)
        mout <<
                "    input" + std::to_string(i) + " = [];\n"
                "    sput" + std::to_string(i) + " = [];\n";
    mout <<
           "end\n"
           "\n"
            "inputs = cell(" + std::to_string(NUM_AUTO_ARGS+num_columns+num_input_files) + ",1);\n"
           "inputs{1} = num_records;\n"
           "inputs{2} = save_mod_n;\n";
    size_t in_ct = NUM_AUTO_ARGS+1;
    for (size_t i=0; i<num_inputs; ++i, ++in_ct)
        mout << "inputs{" + std::to_string(in_ct) + "} = " + inputs->Value(i)
                + ";\t%" + inputs->Key(i) + "\n";
    mout << "\n";

        //Inportant to put the parameter names in the namespace because they can be
        //used as initial conditions
    mout <<
            "for i=1:" + std::to_string(num_inputs) + "\n"
            "    eval([input_names{i} ' = ' num2str(inputs{i}) ';']);\n"
            "end\n"
            "\n";

    for (size_t i=0; i<num_ics; ++i, ++in_ct)
        mout << "inputs{" + std::to_string(in_ct) + "} = " + init_conds->Value(i)
                + ";\t%" + init_conds->Key(i) + "\n";
    for (size_t i=0, ct=0; i<num_vars; ++i)
        if (!variables->IsFreeze(i) && Input::Type(variables->Value(i))==Input::INPUT_FILE)
        {
            std::string si = std::to_string(ct++);
            mout << "inputs{" + std::to_string(in_ct++) + "} = input" + si + ";\n";
            mout << "inputs{" + std::to_string(in_ct++) + "} = sput" + si + ";\n";
        }
    mout << "\n";

    //The column names for the output matrix
    mout << "output_names = cell(" + std::to_string(num_vars+num_diffs) + ",1);\n";
    size_t out_ct = 1;
    for (size_t i=0; i<num_vars; ++i, ++out_ct)
        mout << "output_names{" + std::to_string(out_ct) + "} = '" + variables->Key(i) + "';\n";
    for (size_t i=0; i<num_diffs; ++i, ++out_ct)
        mout << "output_names{" + std::to_string(out_ct) + "} = '" + diffs->ShortKey(i) + "';\n";
    mout << "\n";


    name_m.erase( name_m.find_last_of("_m")-1 );
    mout <<
            "if nargin<2\n"
            "   data = [];\n"
            "else\n"
            "   data = " + name_m + "(inputs{:});\n"
            "end"
            "\n"
            "end\n";

    mout.close();
}

void MEXFile::WriteDataOut(std::ofstream& out, const ParamModelBase* model)
{
    out << "//Begin MEXFile::WriteDataOut\n";
    const size_t num_pars = model->NumPars();
    for (size_t i=0; i<num_pars; ++i)
        out << "            out[ col++*num_records + row_ct ] = " + model->ShortKey(i) + ";\n";
    out << "//End MEXFile::WriteDataOut\n";
    out << "    \n";
}
void MEXFile::WriteIncludes(std::ofstream& out)
{
    out << "//Begin MEXFile::WriteIncludes\n";
    out <<
           "#include \"math.h\"\n"
           "#include \"mex.h\"\n"
           "#include \"matrix.h\"\n";
    out << "//End MEXFile::WriteIncludes\n";
    out << "\n";
}
void MEXFile::WriteInitArgs(std::ofstream& out, const ParamModelBase* inputs,
                   const ParamModelBase* init_conds)
{
    out << "//Begin MEXFile::WriteInitArgs\n";
    const size_t num_inputs = inputs->NumPars(),
            num_ics = init_conds->NumPars();

    out <<
           "    const int num_iters = (int)( *mxGetPr(prhs[0]) / tau ),\n"
           "            save_mod_n = (int)*mxGetPr(prhs[1]);\n"
           "\n";

    for (size_t i=0; i<num_inputs; ++i)
        out << "    " + inputs->Key(i) + " = *mxGetPr(prhs["
               + std::to_string(i+NUM_AUTO_ARGS) + "]);\n";
    out << "\n";
    for (size_t i=0; i<num_ics; ++i)
        out << "    " + init_conds->ShortKey(i) + "0 = *mxGetPr(prhs["
               + std::to_string(i+num_inputs+NUM_AUTO_ARGS) + "]);\n";
    out << "//End MEXFile::WriteInitArgs\n";
    out << "\n";

    _inputCt = num_inputs+num_ics+NUM_AUTO_ARGS; // ### Feels like a hack,
        //just make const parser_mgr& a member?
}
void MEXFile::WriteLoadInput(std::ofstream& out, const ParamModelBase* variables)
{
    out << "//Begin MEXFile::WriteLoadInput\n";
    const size_t num_vars = variables->NumPars();
    for (size_t i=0, ct=0; i<num_vars; ++i)
    {
        if (variables->IsFreeze(i)) continue;
        const std::string& value = variables->Value(i);
        if (Input::Type(value) != Input::INPUT_FILE) continue;

        const std::string var = variables->Key(i),
                inputv = "input_" + var,
                sputv = "sput_" + var, //samples per unit time
                samps_ct = "ct_" + var;
        if (ct++!=0) out << "\n";
        out << "    const double* " + inputv + " = mxGetPr(prhs[" + std::to_string(_inputCt++) + "]);\n";
        out << "    int " + sputv + " = (int)*mxGetPr(prhs[" + std::to_string(_inputCt++) + "]);\n";
        out <<
               "    int " + samps_ct + " = 0;\n"
               "    " + sputv + " = (int)(1.0/(tau * (double)" + sputv + ") + 0.5);\n";
    }
    out << "//End MEXFile::WriteLoadInput\n";
    out << "\n";
}
void MEXFile::WriteMainBegin(std::ofstream& out)
{
    out <<
           "void mexFunction( int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[] )\n"
           "{\n"
           "    if (nlhs>1)\n"
           "    {\n"
           "        mexErrMsgIdAndTxt( \"MATLAB:test_dsmodel:maxlhs\", \"Too many output arguments.\");\n"
           "    }\n";
}
void MEXFile::WriteMainEnd(std::ofstream& out)
{
    out << "}\n";
}
void MEXFile::WriteOutputHeader(std::ofstream& out, const ParamModelBase* variables,
                                   const ParamModelBase* diffs)
{
    out << "//Begin MEXFile::WriteOutputHeader\n";
    const size_t num_fields = variables->NumPars()+diffs->NumPars();
    out <<
           "    const int num_fields = " + std::to_string(num_fields) + ",\n"
           "            num_records = num_iters / save_mod_n;\n"
           "    plhs[0] = mxCreateDoubleMatrix((mwSize)num_records, (mwSize)num_fields, mxREAL);\n"
           "    double* out = mxGetPr(plhs[0]);\n"
           "    int row_ct = 0;\n";
    out << "//End MEXFile::WriteOutputHeader\n";
    out << "\n";
}
void MEXFile::WriteSaveBlockBegin(std::ofstream& out)
{
    out << "            int col = 0;\n";
    out << "\n";
}
void MEXFile::WriteSaveBlockEnd(std::ofstream& out)
{
    out << "            ++row_ct;\n";
    out << "\n";
}

std::string MEXFile::MakeMName(const std::string& name) const
{
    std::string out(name);
    size_t pos = out.find_last_of('.');
    if (pos!=std::string::npos) out.erase(pos);
    out += "_m.m";
    return out;
}
