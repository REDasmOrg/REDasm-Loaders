#pragma once

#include "../pe_resources.h"
#include "borland_types.h"

class BorlandVersion
{
    public:
        BorlandVersion(PackageInfoHeader* packageinfo, const PEResources::ResourceItem& resourceitem, u64 size);
        bool isDelphi() const;
        bool isCpp() const;
        String getSignature() const;

    private:
        bool contains(const String& s) const;

    private:
        PackageInfoHeader* m_packageinfo;
        PEResources::ResourceItem m_resourceitem;
        u64 m_size;
};
