#include "tecnicofs_client_api.h"

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
    memcpy(buffer+1, sessionId, sizeof(int));

    // Faz request de unmount
    write(fd_server, buffer,sizeof(buffer));

    // Recebe resposta
    read(fd_client, &ans, sizeof(int));

    if(ans == -1)
        return -1;

    close(fd_server);
    close(fd_client);

    return 0;
}

int tfs_open(char const *name, int flags) {
    /* TODO: Implement this */
    char buffer[50];
    memset(buffer,0,sizeof(buffer));
    int returnVal;

    buffer[0]='3';

    open_ar request;
    strcpy(request.fileName,name);
    request.sessionId=sessionId;
    request.flags=flags;

    memcpy(buffer+1,&request,sizeof(open_ar));

    write(fd_server,buffer,sizeof(buffer));

    read(fd_client,&returnVal,sizeof(int)); //se der read do server funciona, perguntar ao stor

    return returnVal;
}

int tfs_close(int fhandle) {
    /* TODO: Implement this */
    return -1;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t len) {
    /* TODO: Implement this */
    char bufferTotal[len+8+sizeof(size_t)+1];
    memset(bufferTotal,0,sizeof(buffer));
    ssize_t returnVal;

    bufferTotal[0]='5';

    write_ar request;
    request.sessionId=sessionId;
    request.fhandle=fhandle;
    request.len=len;

    memcpy(bufferTotal+1,&request,sizeof(write_ar));

    memcpy(bufferTotal+sizeof(write_ar)+1,buffer,len);

    write(fd_server,bufferTotal,sizeof(bufferTotal));

    read(fd_client,&returnVal,sizeof(ssize_t));

    return returnVal;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    /* TODO: Implement this */
    return -1;
}

int tfs_shutdown_after_all_closed() {
    /* TODO: Implement this */
    return -1;
}
