#include "inputmgr.h"

InputMgr* InputMgr::_instance = nullptr;

InputMgr* InputMgr::Instance()
{
    if (!_instance) _instance = new InputMgr();
    return _instance;
}

InputMgr::~InputMgr()
{
    delete _instance;
}

int InputMgr::AssignInput(double* listener, const std::string& type_str, int idx)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("InputMgr::AssignInput", std::this_thread::get_id());
#endif
    int input_idx(-1);
    Input::TYPE type = Input::Type(type_str);
    switch (type)
    {
        case Input::UNI_RAND:
        case Input::GAMMA_RAND:
        case Input::NORM_RAND:
        {
            input_idx = EmplaceInput(listener, idx);
            if (input_idx != -1)
                _inputs[input_idx].GenerateInput(type);
            break;
        }
        case Input::INPUT_FILE:
        {
            input_idx = EmplaceInput(listener, idx);
            std::string file_name = type_str;
            file_name.erase(
                std::remove_if(file_name.begin(), file_name.end(), [&](std::string::value_type c)
                {
                    return c=='"';
                }),
                    file_name.end());
            if (input_idx != -1)
            {
                _inputs[input_idx].LoadInput(file_name);
                _stepCts[input_idx] = 0;
            }
            break;
        }
        case Input::USER:
            break;
        case Input::UNKNOWN:
            throw std::runtime_error("ModelMgr::AssignInput: Unknown input.");
    }
    return input_idx;
}
void InputMgr::ClearInputs()
{
    _inputs.clear();
    _stepCts.clear();
}
void InputMgr::InputEval()
{
    const size_t num_inputs = _inputs.size( );
    for (size_t i=0; i<num_inputs; ++i)
    {
        Input& input = _inputs[i];
        if (++_stepCts[i]
                % (int)(1.0 / (_modelMgr->ModelStep()*input.SamplesPerUnitTime()) + 0.5)
                == 0)
            input.NextInput();
    }
}
void InputMgr::JumpToSample(int n)
{
    const size_t num_inputs = _inputs.size();
    for (size_t i=0; i<num_inputs; ++i)
    {
        const int ct = (int)( (double)n / (_modelMgr->ModelStep()) + 0.5 );
        _inputs[i].SeekTo(ct);
    }
}
void InputMgr::RemoveListener(int idx, double* listener)
{
    for (auto& it : _inputs)
        if (it.Index() == idx)
            it.RemoveListener(listener);
}
Input::TYPE InputMgr::Type(size_t i) const
{
    assert(i<_inputs.size());
    return _inputs.at(i).Type();
}
//double* InputMgr::Value(size_t i) const
//{
//    assert(i<_inputs.size());
//    return _inputs.at(i).Value();
//}

InputMgr::InputMgr() : _modelMgr(ModelMgr::Instance())
{
}

int InputMgr::EmplaceInput(double* data, int idx)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("InputMgr::EmplaceInput", std::this_thread::get_id());
#endif
    auto it = std::find_if(_inputs.begin(), _inputs.end(), [=](const Input& inp)
    {
        return inp.Index() == idx;
    });
    int out_idx;
    if (it==_inputs.end())
    {
        _inputs.emplace_back( Input(data, idx) );
        _stepCts.push_back(0);
        out_idx = (int)(_inputs.size() - 1);
    }
    else
    {
        (*it).AddListener(data);
        out_idx = -1;
    }

    return out_idx;
}
