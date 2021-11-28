#pragma once

#include <rdapi/rdapi.h>
#include <unordered_map>
#include <optional>
#include <string>

template<size_t bits>
class ElfLoaderT;

class ElfABI
{
    public:
        virtual ~ElfABI() = default;
        virtual std::optional<std::string> plt(size_t idx) const = 0;
        virtual void parse() = 0;
};

template<size_t bits>
class ElfABIT: public ElfABI
{
    private:
        typedef ElfLoaderT<bits> ElfType;

    public:
        ElfABIT(const ElfType* elf);
        std::optional<std::string> plt(size_t idx) const override;
        void parse() override;

    private:
        template<typename RelType> void processJmpSlot(const RelType* rel, const typename ElfType::SHDR* shdr);
        template<typename RelType> void processGlobDat(const RelType* rel, const typename ElfType::SHDR* shdr);
        template<typename RelType> void processRelocation_ARM(const RelType* rel, const typename ElfType::SHDR* shdr);
        template<typename RelType> void processRelocation_386(const RelType* rel, const typename ElfType::SHDR* shdr);
        template<typename RelType> void processRelocations(typename ElfType::SVAL dtag, typename ElfType::SVAL dtagsz);

    private:
        const ElfType* m_elf;
        std::unordered_map<size_t, std::string> m_plt;
};
