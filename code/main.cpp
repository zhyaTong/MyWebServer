#include "server/WebServer.h"

int main(){
    WebServer server(8080,3,60000,true,16);
    server.Start();
}