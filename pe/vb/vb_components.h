#pragma once

#include <unordered_map>
#include <deque>
#include <string>
#include <rdapi/rdapi.h>
#include "vb_header.h"


class VBComponents
{
    public:
        typedef std::deque<std::string> EventList;
        struct Component { std::string name; EventList events; };
        typedef std::unordered_map<std::string, Component> Components;

    public:
        VBComponents() = delete;
        static const Component* get(RDContext* ctx, GUID* guid);

    private:
        static void initComponents();
        static std::string guidString(GUID* guid);

    private:
        static Components m_components;
};
