#include "operations.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>


/*Session table*/
static char session_table[MAX_SESSIONS][MAX_FILE_NAME];

pthread_t thread_table[MAX_SESSIONS];

pthread_mutex_t mutex;

pthread_cond_t client_cond, server_cond;

int clientptr=0;
int serverptr=0;
int count=0;

static char buffer_thread[MAX_SESSIONS][2048];

// Number of sessions running atm
int runningSessions=0;

int fd_client;
char *pipename;

int exitFlag=0;


void* client(){
    //char buffer_client[50];
    while(1){
        pthread_mutex_lock(&mutex);
        while(!count){
            pthread_cond_wait(&client_cond,&mutex);
        }
        //strcpy(buffer_client,buffer_thread[0][clientptr]);
        clientptr++;
        if (clientptr==MAX_SESSIONS)
            clientptr=0;
        count--;
        pthread_cond_signal(&server_cond);
        pthread_mutex_unlock(&mutex);
    }
} 

int getSessionId(){
    // if (pthread_mutex_lock(&mutex)!=0){
    //     return -1;
    // }
    if(runningSessions >= MAX_SESSIONS)
        return -1;
    for(int i = 0; i < MAX_SESSIONS; i++)
        if(strlen(session_table[i]) == 0)
            return i;
    // if (pthread_mutex_unlock(&mutex)!=0){
    //     return -1;
    // }
    return -1;
}

size_t read_all(int fd,void *buffer,size_t to_read){
    size_t to_read_portion;
    size_t alr_read=0;
    while(to_read>0){
        to_read_portion=to_read;
        alr_read+=(size_t)read(fd,buffer+alr_read,to_read_portion);
        to_read-=to_read_portion;
    }
    return alr_read;
}

size_t write_all(int fd, void* source,size_t to_write){
    size_t to_write_portion;
    size_t alr_written=0;
    while(to_write>0){
        to_write_portion=to_write;
        alr_written+=(size_t)write(fd,source+alr_written,to_write_portion);
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
    // if (pthread_join(thread_table[sessionId],NULL)!=0){
    //         return -1;
    // }
    char buffer[41];
    // Le pipename do client
    read_all(fd_server,buffer,MAX_PIPENAME);
    
    memcpy(buffer_thread[sessionId],buffer,sizeof(buffer));

    // Atualizamos tabela de sessoes iniciadas
    memcpy(session_table[sessionId],buffer,MAX_PIPENAME);

    // Servidor devolve ao cliente o seu sessionID
    fd_client=open(session_table[sessionId],O_WRONLY);
    if (fd_client<0)
        return -1;
    write_all(fd_client,&sessionId,sizeof(int));

    runningSessions++;
    return 0;
}

int tfs_unmount_server(int fd_server){
    int sessionId;
    int success = 0;
    // Le session id
    read_all(fd_server, &sessionId, sizeof(int));

    if (!sessionIdExists(sessionId)){
        return -1;
    }
    // Apagar pipe do client da tabela de sessoes
    memset(session_table[sessionId],0,MAX_PIPENAME);

    runningSessions--;

    // Avisa o cliente do sucesso da operacao
    write_all(fd_client, &success, sizeof(int));
    if(close(fd_client)<0){
        return -1;
    }
    return 0;
}


int tfs_open_server(int fd_server){
    int returnVal;

    int sessionId;
    char fileName[40];
    int flags;

    read_all(fd_server,&sessionId, sizeof(int));
    read_all(fd_server,fileName, 40);
    read_all(fd_server,&flags, sizeof(int));

    if (!sessionIdExists(sessionId)){
        return -1;
    }

    returnVal=tfs_open(fileName,flags);
    write_all(fd_client,&returnVal,sizeof(int));
    return 0;

}

int tfs_close_server(int fd_server){
    int sessionId;
    int fhandle;
    int returnVal;

    read_all(fd_server,&sessionId, sizeof(int));
    read_all(fd_server,&fhandle,sizeof(int));

    if (!sessionIdExists(sessionId)){
        return -1;
    }
    returnVal = tfs_close(fhandle);

    write_all(fd_client,&returnVal, sizeof(int));
    return 0;
}


int tfs_write_server(int fd_server){
    ssize_t bytesWritten;
    int sessionId;
    int fhandle;
    size_t len;


    read_all(fd_server,&sessionId,sizeof(int));
    read_all(fd_server,&fhandle, sizeof(int));
    read_all(fd_server,&len, sizeof(size_t));

    char buffer[len];
    memset(buffer,0,len);

    read_all(fd_server,buffer,len);
    if (!sessionIdExists(sessionId)){
        return -1;
    }

    bytesWritten=tfs_write(fhandle,buffer,len);
    write_all(fd_client,&bytesWritten,sizeof(ssize_t));
    return 0;
}

int tfs_read_server(int fd_server){
    int sessionId;
    int fhandle;
    size_t len;

    ssize_t bytesRead;

    read_all(fd_server, &sessionId, sizeof(int));
    read_all(fd_server,&fhandle, sizeof(int));
    read_all(fd_server,&len, sizeof(size_t));

    if (!sessionIdExists(sessionId)){
        return -1;
    }

    char buffer[len];

    bytesRead = tfs_read(fhandle, buffer, len);

    write_all(fd_client,&bytesRead, sizeof(ssize_t));
    write_all(fd_client,buffer,(size_t) bytesRead);
    return 0;

}

int tfs_shutdown_after_all_closed_server(int fd_server){

    int sessionId;
    int success;

    read_all(fd_server, &sessionId, sizeof(int));

    success = tfs_destroy_after_all_closed();


    if(success)
        exitFlag=1;

    // Apagar pipe do client da tabela de sessoes
    memset(session_table[sessionId],0,MAX_PIPENAME);

    write_all(fd_client, &success, sizeof(int));
    if (close(fd_client)<0){
        return -1;
    }
    if (close(fd_server)<0){
        return -1;
    }

    if (unlink(pipename)<0){
        return -1;
    }
    return 0;
}

int main(int argc, char **argv) {
    int fd;
    char OP_CODE;
    size_t r;

    if (argc < 2) {
        printf("Please specify the pathname of the server's pipe.\n");
        return 1;
    }

    pipename = argv[1];
    printf("Starting TecnicoFS server with pipe called %s\n", pipename);
    tfs_init();

    unlink(pipename);

    if (mkfifo(pipename,0777)!=0)
        return -1;

    fd = open(pipename,O_RDONLY);
    if (fd<0){
        return -1;
    }

    // for (int i=0;i<MAX_SESSIONS;i++){
    //     if (pthread_create(&thread_table[i],NULL,&client,NULL)!=0){
    //         return -1;
    //     }
    // }

    while(!exitFlag){
        r=read_all(fd,&OP_CODE,sizeof(char));
        if (r==0){
            if (close(fd)<0){
                return -1;
            }
            fd = open(pipename,O_RDONLY);
            if (fd<0){
               return -1;
            }   
        }
        switch(OP_CODE){
            case '1':
                tfs_mount_server(fd);
                break;
            case '2':
                tfs_unmount_server(fd);
                break;
            case '3':
                tfs_open_server(fd);
                break;
            case '4':
                tfs_close_server(fd);
                break;
            case '5':
                tfs_write_server(fd);
                break;
            case '6':
                tfs_read_server(fd);
                break;
            case '7':
                return tfs_shutdown_after_all_closed_server(fd);
            default:
                break;
        }
    }

    return 0;
}