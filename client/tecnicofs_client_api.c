#include "tecnicofs_client_api.h"
#include <string.h>

int fd_server;
int fd_client;
int sessionId;

int tfs_mount(char const *client_pipe_path, char const *server_pipe_path) {
    /* TODO: Implement this */
    char buffer[41];
    memset(buffer,0,sizeof(buffer));

    // Apaga o pipe do client se existir
    unlink(client_pipe_path);
    
    // Cria o pipe client
    if (mkfifo(client_pipe_path,0777)!=0)
        return -1;
    
    // Abre pipe do server, verificar se pipe server ja existe(?)
    fd_server = open(server_pipe_path,O_WRONLY);

    if(fd_server<0)
        return -1;
    
    buffer[0]='1';
    strcpy(buffer+1,client_pipe_path); 
    
    // Cliente faz pedido ao servidor e espera pela resposta
    write(fd_server,buffer,sizeof(buffer));

    // Abre o pipe do client
    fd_client = open(client_pipe_path,O_RDONLY);

    if (fd_client<0)
        return -1;

    // Cliente le o session id caso seja possivel abrir sessao
    read(fd_client,&sessionId,sizeof(int));

    // Nao foi possivel abrir sessao
    if(sessionId < 0)
        return -1;
    return 0;
}

int tfs_unmount() {
    char buffer[6];
    int ans;

    buffer[0] = '2';
    memcpy(buffer+1, &sessionId, sizeof(int));

    // Faz request de unmount
    write(fd_server, buffer,sizeof(buffer));

    // Recebe resposta
    read(fd_client, &ans, sizeof(int));

    return ans;

    close(fd_server);
    close(fd_client);
    unlink(fd_server);
}

int tfs_open(char const *name, int flags) {
    /* TODO: Implement this */
    char buffer[50];
    int returnVal;

    buffer[0]='3';

    memcpy(buffer+1,&sessionId,sizeof(int));
    memcpy(buffer+1+sizeof(int),name,40);
    memcpy(buffer+1+sizeof(int)+40,&flags,sizeof(int));

    write(fd_server,buffer,sizeof(buffer));

    read(fd_client,&returnVal,sizeof(int)); //se der read do server funciona, perguntar ao stor

    return returnVal;
}

int tfs_close(int fhandle) {
    char buffer[10];
    int ans;

    buffer[0] = '4';

    memcpy(buffer+1,&sessionId, sizeof(int));
    memcpy(buffer+1+sizeof(int), &fhandle, sizeof(int));

    write(fd_server,buffer, sizeof(buffer));

    read(fd_client, &ans, sizeof(int));

    return ans;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t len) {
    
    char bufferTotal[8 + len + sizeof(size_t) + 1];
    memset(bufferTotal,'\0',sizeof(bufferTotal));
    
    ssize_t bytesWritten;

    bufferTotal[0]='5';

    memcpy(bufferTotal + 1, &sessionId, sizeof(int));
    memcpy(bufferTotal + 1 + sizeof(int), &fhandle, sizeof(int));
    memcpy(bufferTotal + 1 + sizeof(int) + sizeof(int), &len, sizeof(size_t));
    memcpy(bufferTotal + 1 + sizeof(int) + sizeof(int) + sizeof(size_t), buffer, len);

    write(fd_server,bufferTotal,sizeof(bufferTotal));

    read(fd_client,&bytesWritten,sizeof(ssize_t));

    return bytesWritten;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    ssize_t bytesRead;
    char bufferTotal[4 + len + sizeof(size_t) + 1];
    memset(bufferTotal,'\0',sizeof(bufferTotal));

    bufferTotal[0] = '6';

    memcpy(bufferTotal + 1, &sessionId, sizeof(int));
    memcpy(bufferTotal + 1 + sizeof(int), &fhandle, sizeof(int));
    memcpy(bufferTotal + 1 + sizeof(int) + sizeof(int), &len, sizeof(size_t));

    write(fd_server,bufferTotal, sizeof(bufferTotal));

    read(fd_client, &bytesRead, sizeof(ssize_t));
    read(fd_client, buffer, bytesRead);

    return bytesRead;
}

int tfs_shutdown_after_all_closed() {
    
    char buffer[6];
    int success;

    buffer[0] = '7';

    memcpy(buffer + 1, &sessionId, sizeof(int));

    write(fd_server, buffer, sizeof(buffer));

    read(fd_server, &success, sizeof(int));

    return success;
}
