#pragma once

#include <unordered_map>
#include <redasm/redasm.h>
#include "vb_header.h"

using namespace REDasm;

class VBComponents
{
    public:
        typedef std::deque<String> EventList;
        struct Component { String name; EventList events; };
        typedef std::unordered_map<String, Component> Components;

    public:
        VBComponents() = delete;
        static const Component* get(GUID* guid);

    private:
        static void initComponents();
        static String guidString(GUID* guid);

    private:
        static Components m_components;
};
