#include "operations.h"

/*Session table*/
static char session_table[MAX_SESSIONS][MAX_FILE_NAME];

// Number of sessions running atm
int runningSessions=0;

int fd_client;

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

int sessionIdExists(int sessionId){
    if (strlen(session_table[sessionId])==0){
        return 0;
    }
    return 1;
}

int tfs_mount_server(int fd_server){
    int sessionId = getSessionId();
    char buffer[41];
    // Le pipename do client
    read(fd_server,buffer,MAX_PIPENAME);
    
    // Atualizamos tabela de sessoes iniciadas
    memcpy(session_table[sessionId],buffer,MAX_PIPENAME);

    // Servidor devolve ao cliente o seu sessionID
    fd_client=open(session_table[sessionId],O_WRONLY);
    if (fd_client<0)
        return -1;
    write(fd_client,&sessionId,sizeof(int));

    runningSessions++;
}

void tfs_open_server(int fd_server){
    int returnVal;
    open_ar open_struct;
    read(fd_server,&open_struct,sizeof(open_ar));
    if (!sessionIdExists(open_struct.sessionId)){
        return -1;
    }

    returnVal=tfs_open(open_struct.fileName,open_struct.flags);
    write(fd_client,&returnVal,sizeof(int));

}


void tfs_write_server(int fd_server){
    ssize_t returnVal;
    write_ar write_struct;
    read(fd_server,&write_struct,sizeof(write_ar));
    char buffer[write_struct.len];
    read(fd_server,buffer,write_struct.len);
    if (!session_table[write_struct.sessionId]){
        return -1;
    }

    returnVal=tfs_write(write_struct.fhandle,buffer,write_struct.len);
    write(fd_client,&returnVal,sizeof(ssize_t));

}


int main(int argc, char **argv) {
    int fd;
    char OP_CODE;
    int sessionId;
    int r;

    if (argc < 2) {
        printf("Please specify the pathname of the server's pipe.\n");
        return 1;
    }

    char *pipename = argv[1];
    printf("Starting TecnicoFS server with pipe called %s\n", pipename);
    tfs_init();
    unlink(pipename);
    if (mkfifo(pipename,0777)!=0)
        return -1;
    fd = open(pipename,O_RDONLY);
    if (fd<0){
        return -1;
    }
    while(1){
        r=read(fd,&OP_CODE,sizeof(char));
        if (r==0){
            close(fd);
            fd = open(pipename,O_RDONLY);
        }
        switch(OP_CODE){
            case '1':
                tfs_mount_server(fd);
                break;
            case '3':
                tfs_open_server(fd);
                break;
            case '5':
                tfs_write_server(fd);
            default:
                break;
        }
    }

    return 0;
}