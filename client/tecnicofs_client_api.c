#include "tecnicofs_client_api.h"

int tfs_mount(char const *client_pipe_path, char const *server_pipe_path) {
    /* TODO: Implement this */
    int fd_server;
    int fd_client;
    char buffer[41];
    memset(buffer,0,sizeof(buffer));
    int sessionId;

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

    // Cliente le o session id caso seja possivel abrir sessao
    read(fd_client,&sessionId,sizeof(int));

    // Nao foi possivel abrir sessao
    if(sessionId < 0)
        return -1;
    
    close(fd_server);
    close(fd_client);
    return 0;
}

int tfs_unmount() {
    /* TODO: Implement this */
    return -1;
}

int tfs_open(char const *name, int flags) {
    /* TODO: Implement this */
    return -1;
}

int tfs_close(int fhandle) {
    /* TODO: Implement this */
    return -1;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t len) {
    /* TODO: Implement this */
    return -1;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    /* TODO: Implement this */
    return -1;
}

int tfs_shutdown_after_all_closed() {
    /* TODO: Implement this */
    return -1;
}
