

struct Request{
    char command[300];
    int eta; //tempo estimado de conclusao
    int pid; //pid do processo 
    int commandType; 
    // Flag caso queriamos o status do programa
    int isStatus;
}request; 

struct Task{
    int taskId; //identificador unico da tarefa
    // 0-> not started, 1-> running, 2-> finished
    int eta; //tempo estimado de conclusao
    int pid; //pid do processo 
    int status; //indica o status da tarefa
    float timeElapsed; //epresenta o tempo decorrido desde o inicio da execucao da tarefa.
    int isStatus;
    int commandType;
    char command[300]; //armazena o comando associado a tarefa
}task;

struct Command{
    char* args[300];
    char* path;
}command;

struct ServerConfig{
    char* outputfolder;
    int numParallelTasks; //representa o numero maximo de tarefas que o servidor pode executar em paralelo
    int schedPolicy; //indica a politica de escalonamento utilizada pelo servidor para gerenciar as tarefas
}serverconfig;



