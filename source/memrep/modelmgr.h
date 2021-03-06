#ifndef MODELMGR_H
#define MODELMGR_H

#include <QAbstractItemView>

#include "notes.h"
#include "../globals/globals.h"
#include "../globals/scopetracker.h"
#include "../models/conditionmodel.h"
#include "../models/numericmodelbase.h"
#include "../models/tpvtablemodel.h"

class ModelMgr
{
    public:
        enum DIFF_METHOD
        {
            UNKNOWN = -1,
            EULER,
            EULER2,
            RUNGE_KUTTA
        };

        struct ParVariant
        {
            ParVariant(const std::string& title) : title(title)
            {}
            const std::string title;
            std::vector<PairStr> pars;
            std::vector<PairStr> input_files;
            std::string notes;
            static const std::string END_NOTES;
        };

        static ModelMgr* Instance();

        ~ModelMgr();

        void AddModel(ParamModelBase* model);
        void AddCondParameter(const std::string& test, const VecStr& results);
        void AddCondResult(int row, const std::string& result);
        void AddParameter(ds::PMODEL mi, const std::string& key, const std::string& value = "");
        void CreateModels();
        void ClearModels();
        void ClearParameters(ds::PMODEL mi);
        void DeleteParVariant(size_t idx);
        void InsertParVariant(size_t idx, ParVariant* pv);

        void SetCondValue(size_t row, const VecStr& results);
        void SetDiffMethod(DIFF_METHOD diff_method) { _diffMethod = diff_method; }
        void SetIsFreeze(ds::PMODEL mi, size_t idx, bool is_freeze);
        void SetMaximum(ds::PMODEL mi, size_t idx, double val);
        void SetMinimum(ds::PMODEL mi, size_t idx, double val);
        void SetModel(ds::PMODEL mi, ParamModelBase* model);
        void SetModelStep(double step) { _modelStep = step; }
        void SetNotes(Notes *notes);
        void SetNotes(const std::string& text);
        void SetParVariants(const std::vector<ParVariant*>& par_variants);
        void SetPVPar(size_t index, size_t pidx, const std::string& val);
        void SetPVInputFile(size_t index, size_t pidx, const std::string& input_file);
        void SetPVNotes(size_t index, const std::string& notes);
        void SetRange(ds::PMODEL mi, size_t idx, double min, double max);
        void SetTPVModel(TPVTableModel* tpv_model);
        void SetValue(ds::PMODEL mi, size_t idx, const std::string& value);
        void SetView(QAbstractItemView* view, ds::PMODEL mi);

        //Do not use ModelMgr as a general pass through function, for the models,
        //that is what Model(ds::PMODEL mi) is for.
        //Just create getters where a test or actual processing is necessary,
        //OR for symmetry, when there is a corresponding setter
        bool AreModelsInitialized() const;
        VecStr CondResults(size_t row) const;
        DIFF_METHOD DiffMethod() const { return _diffMethod; }
        VecStr DiffVarList() const;
        const Notes* GetNotes() const { return _notes; }
        const ParVariant* GetParVariant(size_t i) const { return _parVariants.at(i); }
        bool IsFreeze(ds::PMODEL mi, size_t idx) const;
        double Maximum(ds::PMODEL mi, size_t idx) const;
        double Minimum(ds::PMODEL mi, size_t idx) const;
        inline const ParamModelBase* Model(ds::PMODEL mi) const { return _models.at(mi); }
        inline double ModelStep() const { return _modelStep; }
        int NumParVariants() const { return _parVariants.size(); }
        double Range(ds::PMODEL mi, size_t idx) const;
        TPVTableModel* TPVModel() { return _tpvModel; }
        std::string Value(ds::PMODEL mi, size_t idx) const;

    private:
        ModelMgr();
#ifdef __GNUG__
        ModelMgr(const ModelMgr&) = delete;
        ModelMgr& operator=(const ModelMgr&) = delete;
        ModelMgr* operator*(ModelMgr*) = delete;
        const ModelMgr* operator*(const ModelMgr*) = delete;
#endif
        static ModelMgr* _instance;

        inline ConditionModel* CondModel();
        inline const ConditionModel* CondModel() const;
        void DeleteParVariants();
        std::vector<ParamModelBase*> MakeModelVec() const;

        DIFF_METHOD _diffMethod;
        Log* const _log;
        std::vector<ParamModelBase*> _models;
        double _modelStep;
        mutable std::mutex _mutex;
        Notes* _notes;
        std::vector<ParVariant*> _parVariants;
        TPVTableModel* _tpvModel;
};

#endif // MODELMGR_H
