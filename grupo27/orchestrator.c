#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h> 
#include <sys/time.h> //gettimeofday
#include "structs.h"
#include <string.h>
#include <sys/wait.h>

// envia um confirmação para o cliente que a tarefa foi recebida pelo servidor 
int confirmaRececao(int pidCliente, int taskId){

    // Abrir o fifo para o cliente
    char fifoName[100];
    // Isto vai gerar um fifo name do tipo: fifo_server_client_1234
    sprintf(fifoName, "fifo_server_client_%d", pidCliente);

    // Abrir o fifo
    int fd = open(fifoName, O_WRONLY);
    while(fd == -1){
        // Caso nao tenha aberto vai abrir novamente
        fd = open(fifoName, O_WRONLY);
    }

    // Escrever a confirmação
    char confirma[100];
    // Isto gera uma string do tipo Task 1 received
    sprintf(confirma, "Task %d received", taskId);

    // Escrever a confirmação no fifo
    if(write(fd, confirma, sizeof(confirma)) == -1){
        perror("Error writing to fifo");
        exit(1);
    }
    close(fd);

    return 1;
}

// é usada quando o cliente solicita uma atualização do status das tarefas em execução
void status(int pid, struct Task* pedidos, int numPedidos){
    if (fork()==0){
        // Abrir o fifo para o cliente
        char fifoName[100];
        // Isto vai gerar um fifo name do tipo: fifo_server_client_1234
        sprintf(fifoName, "fifo_server_client_%d", pid);

        // Abrir o fifo
        int fd = open(fifoName, O_WRONLY );
        while(fd < 0){
            // Se não conseguir abrir o fifo, tentar novamente
            fd = open(fifoName, O_WRONLY );
        }

        printf("A enviar status\n ");

        char tarefasExecutadas[1024]= "Executed:\n";
        char tarefasAexecutar[1024]= "Scheduled:\n";
        char tarefasemExecucao[1024]= "Running:\n";

        // Ler as tarefas e escrever no fifo uma cópia das mesmas
        int i=0;
        while(i<numPedidos){
            struct Task tarefa = pedidos[i];

            if(tarefa.status == 0){
                // Concatenar a tarefa na string de tarefas a executar
                char stringTarefa[500]; 
                sprintf(stringTarefa, "%d %s\n", tarefa.taskId, tarefa.command);
                strcat(tarefasAexecutar, stringTarefa);
            }
            else if(tarefa.status == 1){
                // Concatenar a tarefa na string de tarefas em execução
                char stringTarefa[500]; 
                sprintf(stringTarefa, "%d %s\n", tarefa.taskId, tarefa.command);
                strcat(tarefasemExecucao, stringTarefa);
            }
            else if(tarefa.status == 2){
                // Concatenar a tarefa na string de tarefas executadas
                char stringTarefa[500]; 
                sprintf(stringTarefa, "%d %s %fms\n", tarefa.taskId, tarefa.command, tarefa.timeElapsed);
                strcat(tarefasExecutadas, stringTarefa);
            }
            i++;
        }

        // construir a mensagem de status, no final terá a mensagem completa
        char mensagem[1000]; 
        strcpy(mensagem, "");
        strcat(mensagem, tarefasAexecutar);
        strcat(mensagem, "\n");
        strcat(mensagem, tarefasemExecucao);
        strcat(mensagem, "\n");
        strcat(mensagem, tarefasExecutadas);
        strcat(mensagem, "\n");


        // Escrever a confirmação no fifo
        if(write(fd, mensagem, sizeof(mensagem)) == -1){
            perror("Error writing to fifo");
            exit(1);
        }

        close(fd);
    }
}

// abre, escreve 
void escreveArquivo(char* path, char* conteudo){
    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd == -1){
        perror("Error opening output file");
        exit(1);
    }

    if(write(fd, conteudo, sizeof(conteudo)) == -1){
        perror("Error writing to file");
        exit(1);
    }
    close(fd);
}

int parseCommands(const char* input, struct Command* comandos) {
    printf("Parsing commands...\n");
    //alocar memoria
    char** commands = (char**)malloc(300 * sizeof(char*));

    if (commands == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    
    char* token = strtok((char*)input, "|");
    int numCommands = 0;

    
    while (token != NULL) {
        
        commands[numCommands] = (char*)malloc(300 * sizeof(char));
        if (commands[numCommands] == NULL) {
            perror("Memory allocation failed");
            exit(EXIT_FAILURE);
        }
        strcpy(commands[numCommands], token);

    
        token = strtok(NULL, "|");
        (numCommands)++;
    }

    for (int i = 0; i < numCommands; i++) {
        struct Command comand;

        char* argsCpy;
        argsCpy = strdup(commands[i]);
        char* path = strtok(argsCpy, " ");
        comand.path = strdup(path);

        char* tempArgs[300] = {};
        int j = 0;
        while (argsCpy != NULL) {
            tempArgs[j] = strdup(argsCpy);
            argsCpy = strtok(NULL, " ");
            
            j++;
        }
        //copiar
        memcpy(comand.args, tempArgs, sizeof(tempArgs));

        comandos[i] = comand;

    }

    return numCommands;
}


int prepareManager(int* writeToMain, int* readFromMain){

    // Abrir os fifos para comunicar com o gestor
    
    printf("Preparing manager...\n");
    *writeToMain = open("fifo_server_manager", O_WRONLY);
    *readFromMain = open("fifo_manager_server", O_RDONLY);
    
    return 1;
}


int executePipeline(struct Command* commands, int numCommands, int taskId, char* ficheiroOutput){
    printf("Executing pipeline of commands...\n");
    printf("Number of commands: %d\n", numCommands);
    fflush(stdout);
    
    int i = 0;
    int status[numCommands];
    int pipes[numCommands-1][2];

    for(i = 0; i < numCommands; i++){

        if(i == 0){
            pipe(pipes[i]);

            switch(fork()){
                case -1:
                    perror("fork");
                    return -1;
                    
                case 0:
                    close(pipes[i][0]);
                    dup2(pipes[i][1], 1);
                    close(pipes[i][1]);

                    // execlp(commands[i].path, commands[i].args, NULL);
                    execvp(commands[i].path, commands[i].args);

                    perror("execvp");
                    _exit(EXIT_FAILURE);

                default:
                    close(pipes[i][1]);
            }
        }
        else if(i == numCommands - 1){
            switch(fork()){
                case -1:
                    perror("fork");
                    return -1;
                    
                case 0:
                    dup2(pipes[i-1][0], 0);
                    close(pipes[i-1][0]);

                    // redireciona o output
                    char outputFileName[100];
                    sprintf(outputFileName, "%s/task_%d_output.txt", ficheiroOutput, taskId);
                    int outputFile = open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);

                    if (outputFile == -1) {
                        perror("open");
                        _exit(EXIT_FAILURE);
                    }
                    
                    dup2(outputFile, 1);
                    
                    execvp(commands[i].path, commands[i].args);

                    close(outputFile);
                    perror("execvp");
                    _exit(EXIT_FAILURE);

                default:
                    close(pipes[i-1][0]);
            }
        }
        else{
            pipe(pipes[i]);
            switch(fork()){
                case -1:
                    perror("fork");
                    return -1;

                case 0:
                    close(pipes[i][0]);
                    dup2(pipes[i-1][0], 0);
                    close(pipes[i-1][0]);
                    dup2(pipes[i][1], 1);
                    close(pipes[i][1]);

                    execvp(commands[i].path, commands[i].args);

                    perror("execvp");
                    _exit(EXIT_FAILURE);

                default:
                    close(pipes[i-1][0]);
                    close(pipes[i][1]);
            }
        }
    }

    for(i = 0; i < numCommands; i++){
        wait(&status[i]);
    }
    return 0;
}

int executaPedido(struct ServerConfig serverConfig){
    // Vamos buscar um pedido à lista de pedidos
    // Para debug vamos mostrar o pedido
    struct Task* tarefasExecutar = (struct Task*)malloc(sizeof(struct Task));
    
    struct Task tarefa;
    int readFromMain;
    int writeToMain;

    int numTasksRunning=0;
    int numTasksExecuted=0;
    int totalTasks = 0;
    int maxNumTasks=1;
    
    readFromMain = open("fifo_manager_server", O_RDWR);
    if(readFromMain == -1) {
        perror("Error opening fifo");
        exit(1);
    }

    while(1){
        // Adiciona pedidos e atualiza estado
        if (read(readFromMain, &tarefa, sizeof(struct Task)) > 0 ){

            // Caso o pedido seja um pedido de status
            if (tarefa.isStatus == 1){
                printf("Pedido de status recebido\n");
                status(tarefa.pid, tarefasExecutar, totalTasks);
            }

            else{
                // Caso recebamos uma tarefa cujo status=0, então adicionamos à lista de tarefas a executar
                if(tarefa.status == 0 && tarefa.isStatus == 0){
                    // Vamos avisar o cliente que recebemos 

                    confirmaRececao(tarefa.pid, tarefa.taskId);
                    // Vamos adicionar o pedido aos pedidos a executar
                    if (totalTasks == maxNumTasks){
                        printf("Aumentando o tamanho da lista de tarefas a executar...\n");
                        maxNumTasks = maxNumTasks+1;
                        tarefasExecutar = (struct Task*)realloc(tarefasExecutar, maxNumTasks*sizeof(struct Task));
                    }

                    printf("Tarefa %d adicionada à lista de tarefas a executar\n", tarefa.taskId);
                    tarefasExecutar[totalTasks] = tarefa;
                    totalTasks++;
                }

                else if(tarefa.status == 1){
                    printf("Tarefa %d vai ser executada\n", tarefa.taskId);
                    numTasksRunning++;
                    
                    tarefasExecutar[tarefa.taskId] = tarefa;
                    
                }

                // Caso recebamos que uma tarefa terminou então vamos atualizar o estado da tarefa e o numero de tarefas a correr
                else if (tarefa.status ==2){
                    printf("Tarefa %d terminou\n", tarefa.taskId);
                    numTasksRunning--;
                    numTasksExecuted++;
                    
                    tarefasExecutar[tarefa.taskId] = tarefa;
                    
                }

            }

        }

        // Executa pedidos
        if(numTasksRunning < serverConfig.numParallelTasks && numTasksExecuted+numTasksRunning < totalTasks){
            // Vamos buscar o pedido à lista de pedidos a executar
            struct Task tarefa = tarefasExecutar[numTasksExecuted+numTasksRunning];
            printf("A executar o pedido: %d ...\n", numTasksExecuted);
            printf("Task id: %d\n", tarefa.taskId);

            
            if(fork()==0){
                struct Task tarefaAExecutar = tarefa;

                struct timeval start_time;
                gettimeofday(&start_time, NULL);

                int pid = fork();
                if (pid == 0){

                    tarefaAExecutar.status = 1;

                    if(write(readFromMain, &tarefaAExecutar, sizeof(struct Task)) == -1){
                        perror("Error writing to fifo");
                        exit(1);
                    }

                    // Vamos executar o comando
                    struct Command commands[300];
                    int numCommands = parseCommands(tarefaAExecutar.command, commands);
                    

                    if (tarefa.commandType==0){
 
                        // criar um fd para o output
                        char outputFileName[100];
                        sprintf(outputFileName, "%s/task_%d_output.txt", serverConfig.outputfolder, tarefaAExecutar.taskId);
                        int outputFd = open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                        if (outputFd == -1){
                            perror("Error opening output file");
                            exit(1);
                        }
                        dup2(outputFd, STDOUT_FILENO);
                        execvp(commands[0].path, commands[0].args);
                        close(outputFd);
                    }

                    else if(tarefa.commandType==1) {
                        printf("Executing pipeline...\n");
                        fflush(stdout);
                        executePipeline(commands, numCommands, tarefaAExecutar.taskId, serverConfig.outputfolder);
                        
                    }
                    exit(1);

                }
                else{
                    waitpid(pid, NULL,0);
                    // Calcular o tempo de execução
                    struct timeval end_time;
                    gettimeofday(&end_time, NULL);
                    double execution_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
                    execution_time = execution_time * 1000;
                    printf("Tempo de execução: %.6f ms\n", execution_time);

                    tarefaAExecutar.timeElapsed = execution_time;
                    tarefaAExecutar.status = 2;
            

                    // Vamos enviar o status da tarefa para o servidor
                    if(write(readFromMain, &tarefaAExecutar, sizeof(struct Task)) == -1){
                        perror("Error writing to fifo");
                        exit(1);
                    }

                    exit(1);
                }
            }
        }    

    }

    return 1;
}

// Servidor cria o client_server fifo para receber os pedidos dos clientes
// O servidor fica a ler ininterruptamente deste fifo
// Quando recebe um pedido, cria um novo fifo com o nome do cliente e envia para o cliente que recebeu o pedido
// Quando receber um status de um cliente, envia para o cliente que fez o pedido

void initServer(struct ServerConfig* config, int argc, char* argv[]){

    // Inicializar as configurações do servidor
    config->outputfolder = "output";
    config->numParallelTasks = 1;
    config->schedPolicy = 0;

    // Verificar se o utilizador passou argumentos
    if (argc > 1){
        // A estrutura é orchestrator output-folder parallel-tasks sched-policy
        config->outputfolder = argv[1];
        config->numParallelTasks = atoi(argv[2]);
    }

}

int prepareServer(int* writeToManager, int* readFromClient){

    // O servidor cria o fifo client_server
    if (mkfifo("fifo_client_server", 0666) == -1){
        // apagar o fifo se ele ja existir
        if(unlink("fifo_client_server") == -1){
            perror("Error deleting fifo");
            exit(1);
        }
        
        
    }
    if (mkfifo("fifo_manager_server", 0666) == -1){
        perror("Error creating fifo");
        exit(1);
    }

    // Abrir o fifo para comunicar com o manager
    *writeToManager = open("fifo_manager_server", O_WRONLY);
    *readFromClient=open("fifo_client_server", O_RDONLY);

    return 1;
}


struct Task taskFromRequest(struct Request request, int taskId){
    struct Task task;
    task.taskId = taskId;
    task.eta = request.eta;
    task.pid = request.pid;
    task.status = 0;
    task.timeElapsed = 0;
    task.isStatus = request.isStatus;
    strcpy(task.command, request.command);
    task.commandType = request.commandType;
    return task;
}

int main(int argc, char* argv[]){
    struct ServerConfig config;
    struct Request request; 
    int numPedidos=0;

    initServer(&config, argc, argv);

    int writeToManager;
    int readFromClient;

    printf("\n\n--Server Config--\n");
    printf("Output Folder: %s\n", config.outputfolder);
    printf("Num Parallel Tasks: %d\n", config.numParallelTasks);
    printf("Scheduling Policy: %d\n", config.schedPolicy);
    printf("-----------------\n\n");


    printf("Orchestrator is running...\nWaiting for requests...\n");

    // Vamos criar o processo de gestor de pedidos
    if (fork()==0){
        executaPedido(config);
    }
    else{
        
        prepareServer(&writeToManager, &readFromClient);
        // Criar os fifos para comunicar com o gestor
        // Ler ininterruptamente do fifo
        while(1){

            // Ler o pedido
            if(read(readFromClient, &request, sizeof(struct Request)) > 0 ){

                    // Vamos criar a tarefa a partir da estrutura request que recebemos
                    struct Task tarefa = taskFromRequest(request, numPedidos);

                    // Vamos enviar a tarefa para o gestor de pedidos
                    if (write(writeToManager, &tarefa, sizeof(tarefa)) == -1){
                        perror("Error writing to fifo");
                        exit(1);
                    }
                    if (request.isStatus == 0) numPedidos++;
            }
        }
        unlink("fifo_client_server");
        return 0;
    }
}