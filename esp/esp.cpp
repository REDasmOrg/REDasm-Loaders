#include "esp.h"
#include "chips/esp8266.h"
#include <cassert>

ESPLoader::ESPLoader(): Loader() { }
AssemblerRequest ESPLoader::assembler() const { return { "xtensa", "xtensale"}; }
bool ESPLoader::test(const LoadRequest &request) const { return ESP8266::test(request); }

void ESPLoader::init(const LoadRequest &request)
{
    if(ESP8266::test(request))
        m_esp = std::make_unique<ESP8266>(this);
    else
        r_ctx->log("Unknown ESP Chip");

    Loader::init(request);
}

void ESPLoader::load() { if(m_esp) m_esp->load(); }
