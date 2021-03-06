#ifndef INPUT_H
#define INPUT_H

#include <cstring>

#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <string>

#include "../file/defaultdirmgr.h"
#include "../globals/log.h"
#include "../globals/scopetracker.h"

class Input
{
    public:
        static const size_t INPUT_EXP,
                            INPUT_SIZE,
                            INPUT_MASK;

        enum TYPE
        {
            UNKNOWN = -1,
            INPUT_FILE,
            GAMMA_RAND,
            NORM_RAND,
            UNI_RAND,
            USER
        };
        static TYPE Type(const std::string& text);
        static const std::string INPUT_FILE_STR,
                                GAMMA_RAND_STR,
                                NORM_RAND_STR,
                                UNI_RAND_STR;

        explicit Input(double* value, int idx);
        Input(const Input& other);
        ~Input();

        void AddListener(double* listener);
        void GenerateInput(TYPE type);
        void LoadInput(const std::string& file_name);
        void NextInput(int n = 1);
        double NextInputHalf() const;
        void RemoveListener(double* listener);
        double SeeNextInput() const;
        void SeekTo(int ct);

        int Index() const { return _index; }
        int SamplesPerUnitTime() const { return _samplesPerUnitTime; }
        TYPE Type() const { return _type; }

    private:
#ifdef __GNUG__
        Input& operator=(const Input&) = delete;
#endif

        void DeepCopy(const Input& other);
        std::string ExpandFileName(const std::string& file_name) const;
        template<typename T>
        void GenerateRandInput(T& distribution);
        void LoadBinInput(const std::string& file_name);
        void LoadTextInput(const std::string& file_name);
        void ResetInput();
        void UpdateListeners();

        size_t _ct;
        const int _index;
        double* _input;
        Log* const _log;
        int _samplesPerUnitTime;
        TYPE _type;
        std::vector<double*> _listeners;
};

#endif // INPUT_H
