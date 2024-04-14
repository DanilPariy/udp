#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>

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
