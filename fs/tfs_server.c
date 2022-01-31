#include "operations.h"

/*Session table*/
static char session_table[MAX_SESSIONS][MAX_FILE_NAME];

// Number of sessions running atm
int runningSessions=0;

int getSessionId(){ // lock
    if(runningSessions >= MAX_SESSIONS)
        return -1;
    for(int i = 0; i < MAX_SESSIONS; i++)
        if(strlen(session_table[i]) == 0)
            return i;
    return -1;
}

size_t read_all(int fd,void *buffer,size_t to_read){
    size_t to_read_portion;
    size_t alr_read=0;
    while(to_read>0){
        to_read_portion=to_read;
        alr_read+=read(fd,buffer+alr_read,to_read_portion);
        to_read-=to_read_portion;
    }
    return alr_read;
}

size_t write_all(int fd, void* source,size_t to_write){
    size_t to_write_portion;
    size_t alr_written=0;
    while(to_write>0){
        to_write_portion=to_write;
        alr_written+=write(fd,source+alr_written,to_write_portion);
        to_write-=to_write_portion;
    }
    return alr_written;
}

int tfs_mount_server(int sessionId, int fd, int fd_client){
    char buffer[41];
    // Le pipename do client
    read_all(fd,buffer,MAX_PIPENAME);
    
    // Atualizamos tabela de sessoes iniciadas
    memcpy(session_table[sessionId],buffer,MAX_PIPENAME);

    // Servidor devolve ao cliente o seu sessionID
    fd_client=open(session_table[sessionId],O_WRONLY);
    if (fd_client<0)
        return -1;
    write_all(fd_client,&sessionId,sizeof(int));

    runningSessions++;
}

int main(int argc, char **argv) {
    int fd;
    int fd_client;
    char OP_CODE;
    int sessionId;

    if (argc < 2) {
        printf("Please specify the pathname of the server's pipe.\n");
        return 1;
    }

    char *pipename = argv[1];
    printf("Starting TecnicoFS server with pipe called %s\n", pipename);

    /* TO DO */
    unlink(pipename);
    if (mkfifo(pipename,0777)!=0)
        return -1;
    fd = open(pipename,O_RDONLY);
    while(1){
        read_all(fd,&OP_CODE,sizeof(char));
        switch(OP_CODE){
            case '1':
                sessionId = getSessionId();
                tfs_mount_server(sessionId,fd,fd_client);
                break;
            case '2':
                
            default:
                break;
        }
    }

    return 0;
}