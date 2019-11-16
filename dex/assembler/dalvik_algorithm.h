#pragma once

#include <unordered_map>
#include <unordered_set>
#include <list>
#include <redasm/plugins/assembler/algorithm/algorithm.h>

using namespace REDasm;

class DexLoader;
struct DexDebugInfo;
struct DexEncodedMethod;

class DalvikAlgorithm: public Algorithm
{
    DEFINE_STATES(StringIndexState = UserState, MethodIndexState,
                  PackedSwitchTableState, SparseSwitchTableState, FillArrayDataState,
                  DebugInfoState)

    private:
        typedef std::unordered_map<address_t, std::list<s32> > PackagedCaseMap;
        typedef std::unordered_map<u64, address_t> SparseCaseMap;

    public:
        DalvikAlgorithm();

    protected:
        void validateTarget(const CachedInstruction&) const override;
        void onDecodedOperand(const Operand* op, const CachedInstruction& instruction) override;
        void onDecoded(const CachedInstruction &instruction) override;
        void decodeState(const State *state) override;

    private:
        void stringIndexState(const State* state);
        void methodIndexState(const State* state);
        void packedSwitchTableState(const State* state);
        void sparseSwitchTableState(const State* state);
        void fillArrayDataState(const State* state);
        void debugInfoState(const State* state);
        void emitCaseInfo(address_t address, const PackagedCaseMap& casemap);
        void emitCaseInfo(address_t address, const CachedInstruction &instruction, const SparseCaseMap& casemap);
        void emitArguments(const State* state, const DexEncodedMethod &dexmethod, const DexDebugInfo &dexdebuginfo);
        void emitDebugData(const DexDebugInfo &dexdebuginfo);
        void checkImport(const State *state);
        bool canContinue(const CachedInstruction &instruction) const;

    private:
        DexLoader* m_dexloader;
        std::unordered_set<String> m_imports;
        std::unordered_set<address_t> m_methodbounds;
};
