#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "structs.h"
#include <string.h>
#include <sys/stat.h>

// Abrir o fifo client_server para enviar pedidos ao servidor
// Criar um fifo com o seu pid e envia o pedido ao servidor
// Ler interruptamente do fifo com o seu pid
// Quando recebe um status, imprime-o

void debugPedido(struct Request request){
    printf("Comando: %s\n", request.command);
    printf("ETA: %d\n", request.eta);
    printf("PID: %d\n", request.pid);
}


void escreveServer(struct Request request){
    // Abrir o fifo client_server
    int fifo_fd = open("fifo_client_server", O_WRONLY);
    if(fifo_fd == -1){
        perror("Error opening fifo");
        exit(1);
    }
    // Escrever o pedido no fifo
    if(write(fifo_fd, &request, sizeof(request)) == -1){
        perror("Error writing to fifo");
        exit(1);
    }
    // Fechar o fifo
    close(fifo_fd);
}
int recebeServer(){
    char fifoName[100];
    sprintf(fifoName, "fifo_server_client_%d", getpid());


    if (mkfifo(fifoName, 0666) == -1){
        perror("Error creating fifo");
        exit(1);
    }
    // Abrir o fifo para o cliente
    int fd = open(fifoName, O_RDONLY );
    if(fd == -1){
        perror("Error opening fifo");
        exit(1);
    }
    return fd;
}


int main(int argc, char* argv[]){

    // Caso o cliente receba apenas dois argumentos e o segundo seja um status então o cliente imprime o status
    if (argc == 2 && strcmp(argv[1], "status") == 0){
        // Abrir o fifo com o pid do cliente
        // Ler o status
        struct Request request;
        request.isStatus = 1;
        request.pid = getpid();

        escreveServer(request);
        
        // Só abre o fifo para ler 
        int fd = recebeServer();
        
        char status[1000];
        if(read(fd, status, sizeof(status)) == -1){
            perror("Error reading from fifo");
            exit(1);
        }
        printf("%s\n", status);

        char fifoName[100];
        sprintf(fifoName, "fifo_server_client_%d", getpid());
        unlink(fifoName);

    } // Caso o cliente receba cinco argumentos e o segundo seja execute
    else if (argc == 5 && strcmp(argv[1], "execute")==0)
    {

        char* comando = argv[4];
        int eta = atoi(argv[2]);
        int commandType=-1;
        if (strcmp(argv[3], "-u") == 0 ){
            commandType =0;
        }
        else if (strcmp(argv[3], "-p") == 0){
            commandType = 1;
        }

        int pid = getpid();
        
        // Struct que contem as informações da tarefa
        struct Request request;
        strcpy(request.command, comando);
        request.eta = eta;
        request.pid = pid;
        request.isStatus = 0;
        request.commandType = commandType;

        escreveServer(request);

        // Criar o fifo com o nome do cliente para receber informações do servidor
      
        int fd = recebeServer();
        // Ler a resposta do servidor
        char resposta[100];
        if(read(fd, resposta, sizeof(resposta)) == -1){
            perror("Error reading from fifo");
            exit(1);
        }

        printf("%s\n", resposta);

        
        char fifoName[100];
        sprintf(fifoName, "fifo_server_client_%d", getpid());
        unlink(fifoName);
    }
    

    return 0;
}