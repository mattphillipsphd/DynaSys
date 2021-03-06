#include "mrunmexwm.h"

MRunMEXWM::MRunMEXWM(const std::string& name) : MRunMEX(name)
{
}

void MRunMEXWM::Make(std::ofstream &out) const
{
    const std::string num_input_files = std::to_string( static_cast<const VariableModel*>(
                _modelMgr->Model(ds::VAR))->TypeCount(Input::INPUT_FILE) );

    std::string name_run = ds::StripPath( NameRun() );
    name_run.erase(name_run.find_last_of('.'));
    out <<
           "function [data, yhat] = " + name_run + "(num_records, save_mod_n, ...\n"
           "                      num_intervals, user_mipars, user_mdpars, target, varargin)\n"
           "%Auto-generated by DynaSys version " + ds::VERSION_STR + "\n"
           "\n";

    WriteDefaultPars(out);

    std::string name_defs = ds::StripPath( NameDefs() );
    if (name_defs.find_last_of('.') != std::string::npos)
        name_defs.erase(name_defs.find_last_of('.'));
    out <<
            "xInfo = " + name_defs + ";\n"
            "input_names = xInfo.input_names;\n"
            "inputs = xInfo.inputs;\n"
            "tau = xInfo.tau;\n"
            "num_inputs = length(inputs);\n"
            "\n"
            "for iArg=1:length(varargin)\n"
            "    if isstruct(varargin{iArg})\n"
            "        xInput = varargin{iArg};\n"
            "        keys = fieldnames(xInput);\n"
            "        values = struct2cell(xInput);\n"
            "        \n"
            "        clInput = cell(1, 2*length(keys));\n"
            "        for i=1:length(keys)\n"
            "            clInput{2*i-1} = keys{i};\n"
            "            clInput{2*i} = values{i};\n"
            "        end\n"
            "        \n"
            "        varargin = [varargin clInput];\n"
            "    end\n"
            "end\n"
            "\n"
            "input_data_ct = 0;\n"
            "input_data = cell(1,256);\n"
            "sput = NaN(1,256);\n"
            "for iArg=1:length(varargin)\n"
            "    if ~ischar(varargin{iArg}), continue; end\n"
            "    switch varargin{iArg}\n"
            "        case input_names\n"
            "            iiInput = strcmp(input_names, varargin{iArg});\n"
            "            inputs{iiInput} = varargin{iArg+1};\n"
            "        case 'data'\n"
            "            input_data_ct = input_data_ct + 1;\n"
            "            input_data{input_data_ct} = varargin{iArg+1};\n"
            "            sput(input_data_ct) = varargin{iArg+2}; %samples per unit time\n"
            "   end\n"
            "end\n"
            "if input_data_ct~=" + num_input_files + "\n"
            "    error(['Incorrect number of inputs, " + num_input_files + " required, ' ... \n"
            "           num2str(input_data_ct) ' supplied.']);\n"
            "end\n"
            "\n"
            "inputs{1} = num_records;\n"
            "inputs{2} = save_mod_n;\n"
            "\n"
            "if input_data_ct>0\n"
            "    start_idx = num_inputs - 2*input_data_ct;\n"
            "    for i=1:input_data_ct\n"
            "        inputs{start_idx + 2*i - 1} = input_data{i};\n"
            "        inputs{start_idx + 2*i} = sput(i);\n"
            "    end\n"
            "end\n"
            "\n"
            "num_iters = num_records / tau;\n"
            "mipars = int32( zeros(6+length(user_mipars), 1) );\n"
            "mipars(1) = num_iters;\n"
            "mipars(2) = num_intervals;\n"
            "mipars(3) = num_iters / num_intervals;\n"
            "mipars(4) = 1.0 / tau;\n"
            "mipars(5) = tau * num_iters / num_intervals;\n"
            "mipars(6) = length(target);\n"
            "for i=1:length(user_mipars), mipars(6+i) = int32( user_mipars(i) ); end\n"
            "mdpars = user_mdpars;\n"
            "\n"
            "inputs = [inputs; {mipars mdpars target}'];\n"
            "\n";

    std::string name_mex = ds::StripPath( NameExec() );
    out <<
            "%keyboard;\n"
            "[data, yhat] = " + name_mex + "(inputs{:});\n"
            "\n"
            "end\n";

}
