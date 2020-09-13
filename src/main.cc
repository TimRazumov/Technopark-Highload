#include "config.h"
#include "server.h"

int main(int argc, char **argv)
{
    std::string configFileName = config::parseCommandLineForConfigFile(argc, argv);
    config::Settings settings(configFileName);
    StaticServer server(settings);
    return server.Start();
}
