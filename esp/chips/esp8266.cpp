#include "esp8266.h"
#include "../esp.h"

ESP8266::ESP8266(ESPLoader *loader): ESPCommon(loader) { }

bool ESP8266::test(const LoadRequest &request)
{
    return ESPCommon::test(request);
}
