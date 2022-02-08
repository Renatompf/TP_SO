#include "header_files/var_amb.h"
#include "header_files/balcao.h"
#include "header_files/cliente.h"
#include "header_files/medico.h"

                // Argv[1] --> Nome Medico
                // Argv[0] --> Especialidade

                //RETIRAR VARIAVEIS DE AMBIENTE

int fdCliente;
ThreadEscreveSinalVida thread;

int verificaEspaco(char* comando){
	int i = 0;
	while(comando[i] != '\0')
	{
		if (comando[i] == ' ' && comando[i+1] != ' ' && comando[i+1] != '\0' && comando[i+1] != '\n')
		{
			return 1;
		}
		++i;
	}
	return 0;
}


void help(){ 
    printf("Comandos:\n\t -> TERMINA - para sair e terminar o medico\n");
}


void criaFifoMedico(char* fifoMedico){
    int res = mkfifo(fifoMedico, 0600);                                                  // É criado o Fifo do Medico
        if(res < 0){
            printf("[ERRO] Criar o %s", fifoMedico);
        } 
}

int abreFifoBalcao(){
	int fdBalcao = open(FIFO_BALCAO_MEDICO, O_RDWR);    
	if(fdBalcao == -1)
		fprintf(stderr, "Erro ao abrir fifo do balcao\n");
	return fdBalcao;
}

int abreFifoMedico(char* fifoMedico){
	int fdMedico = open(fifoMedico, O_RDWR);  
	if(fdMedico == -1)
		fprintf(stderr, "Erro ao abrir fifo do cliente\n");
	return fdMedico;
}

int abreFifoCliente(int pid){
	char fifo[TAM_NOME];
	sprintf(fifo, FIFO_CLIENTE, pid);
	int fdCliente = open(fifo, O_RDWR | O_NONBLOCK);
	if(fdCliente == -1)
		fprintf(stderr, "Erro ao abrir fifo do cliente %d\n", pid);
	return fdCliente;
}

void escreveParaBalcao(int fdBalcao, medico* med, char mensagem[]){
    MensagemMedicoBalcao msg;
    msg.med = *med;
    strcpy(msg.request, mensagem);
    int nBytes = write(fdBalcao, &msg, sizeof(msg));
    if(nBytes < 0 ){
        printf("[ERRO] Erro ao escrever no pipe do Balcao\n");
        exit(0);
    }    
}

void* funcaoThreadEnviaSinalVida(void* argumentos){
    ThreadEscreveSinalVida* dados = (ThreadEscreveSinalVida*) argumentos;
    do{
        sleep(10);
        escreveParaBalcao(dados->fdBalcao, &(dados->mensagem.med), dados->mensagem.request);
    }while(*(dados->continua) != 0);
}

int leDoBalcao(int *fdMedico, medico* med, int fdBalcao, int* r){
    char msg[TAM_MENSAGEM];
    char first_arg[TAM_MENSAGEM/2], second_arg[TAM_MENSAGEM/2];
    int nBytes = read(*fdMedico, msg, sizeof(msg));
    if(med->emConsulta == 0){
        if(nBytes < 0 ){
                printf("[ERRO] Erro ao ler no pipe do Balcao");
                exit(0);
        }
        if(verificaEspaco(msg) == 1){
            sscanf(msg,"%s %s", first_arg, second_arg);
            if(strcmp(first_arg, "Consulta") == 0){
                med->emConsulta = 1;
                fdCliente = abreFifoCliente(atoi(second_arg));
                printf("<MEDICO> VAI ENTRAR NUMA CONSULTA COM O CLIENTE COM O PID %d\n", atoi(second_arg));
            }
        }else{
            if(strcmp("ADICIONADO", msg) == 0){
                pthread_t p;

                thread.tid = p;
                thread.continua = r;
                thread.fdBalcao = fdBalcao;
                thread.mensagem.med = *med;
                strcpy(thread.mensagem.request, "VIVO");

                pthread_create(&thread.tid, NULL, funcaoThreadEnviaSinalVida, (void *) &thread);
            }
            if(strcmp("Lista cheia", msg) == 0){
                printf("\n<MEDICO> Vou terminar porque ja ha muitos medicos\n");
                fflush(stdout);
                *r = 0;
                return 0;
            }
            printf("<BALCAO> %s\n", msg);
        }
        
    }
    else{
        printf("\nResposta do cliente: %s ", msg);
        if(strcmp(msg, "adeus\n") == 0){
            med->emConsulta = 0;
            close(fdCliente);
            fdCliente = 0;
            escreveParaBalcao(fdBalcao, &(*med), "Livre");
        }
    }
    if(strcmp(msg, "TERMINA") == 0){
        printf("\nVou Terminar!");
        fflush(stdout);
        *r = 0;
        return 0;
    }

    fflush(stdout);
    *r = 1;
    return 1;
}

int verificaEspecialidadeMedico(char s[]){
    for(int i = 0; i < 5; i++){
        if(strcmp(especialidadesDisponiveis[i], s) == 0)
            return 1;
    }
    printf("A Especialidade Indicada Nao Esta Disponivel...\n");
    fflush(stdout);
    return 0;
}
    
int main(int argc, char* argv[]){
    fd_set fds;        //Conjunto de Flags dos descritores
    medico med;
    char fifoMedico[40];
    int res, nBytes;
    int fdBalcao, fdMedico;
    threadDataLerBalcao tDataLerBalcao;

    if(access(FIFO_BALCAO_MEDICO, F_OK)!=0){
        printf("[ERRO] O Balcao Nao se Encontra Ativo!!!\n");
        exit(-1);
    }

    if(argc < 2){
        printf("[ERRO] Numero de Argumentos Insuficientes!!!\n");
        exit(-1);
    }

    strcpy(med.nomeMedico, argv[1]);
    if(verificaEspecialidadeMedico(argv[2]))
        strcpy(med.especialidadeMedico, argv[2]);
    else
        exit(-1);
    med.pid_m = getpid();
    med.primeiraVez = 1;
    med.emConsulta = 0;

    sprintf(fifoMedico, FIFO_MEDICO, med.pid_m);
    criaFifoMedico(fifoMedico);
                                                               
    fdBalcao = abreFifoBalcao();
    if(fdBalcao == -1){
        exit(-1);
    }

    fdMedico = abreFifoMedico(fifoMedico); 
    if(fdMedico == -1){
        close(fdBalcao);
        exit(-1);
    }
     
    escreveParaBalcao(fdBalcao, &med, "Primeira comunicacao");
    med.primeiraVez = 0;

    char str[TAM_MENSAGEM];
    int n, r = 0;
 
    do{

        if(med.emConsulta == 0)
            printf("\nComando: ");
        else
            printf("\nPergunta para o Cliente: ");
        fflush(stdout);     

        FD_ZERO(&fds);     //Inicializa Conjunto
        FD_SET(0, &fds);
        FD_SET(fdMedico, &fds);

        n = select(fdMedico+1, &fds, NULL, NULL, NULL); 
        if(n > 0 && FD_ISSET(0, &fds)){
            fgets(str, sizeof(str), stdin);

            if(med.emConsulta == 1){ 
                write(fdCliente, str, sizeof(str));
                if(strcmp(str, "adeus\n") == 0){
                    med.emConsulta = 0;
                    escreveParaBalcao(fdBalcao, &med, "Livre");
                }
            }
            else{
                printf("Recebi o comando > %s\n", str);
                if(strcmp(str, "help\n") == 0)
                    help();
            }
            fflush(stdout); 
        }
        else if(n > 0 && FD_ISSET(fdMedico, &fds))
            leDoBalcao(&fdMedico, &med, fdBalcao, &r);

    }while(strcmp(str, "TERMINA\n") != 0 && r != 0);

    escreveParaBalcao(fdBalcao, &med, "desistir");
    
    close(fdBalcao);
	close(fdMedico);
	unlink(fifoMedico);

}