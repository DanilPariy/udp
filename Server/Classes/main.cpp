#include "Constants.h"
#include "RandomHelper.h"
#include "ConfigManager.h"
#include "BufferParser.h"
#include "ServerManager.h"

int main()
{
    if (!ServerManager::getInstance()->openSocket())
    {
        perror("socket creation/binding failed");
        exit(EXIT_FAILURE);
    }

    ServerManager::getInstance()->startSocket();
    
    return 0;
}
