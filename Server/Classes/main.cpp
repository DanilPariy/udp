#include "Constants.h"
#include "RandomHelper.h"
#include "BufferParser.h"
#include "UdpServer.h"

int main()
{
    UdpServer server;
    if (!server.openSocket())
    {
        perror("socket creation/binding failed");
        exit(EXIT_FAILURE);
    }

    server.startSocket();
    
    return 0;
}
