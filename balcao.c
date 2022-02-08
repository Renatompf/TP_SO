#include "header_files/var_amb.h"
#include "header_files/balcao.h"
#include "header_files/cliente.h"
#include "header_files/medico.h"

int fdSintomas[2]; //VARIAVEL PARA FAZER O REDIRECIONAMENTO NO SENTIDO BALCAO-CLASSIFICADOR
int fdRespostas[2]; //VARIAVEL PARA FAZER O REDIRECIONAMENTO NO SENTIDO CLASSIFICADOR-BALCAO
int status;
//=================================================================== FUNÇÕES CLASSIFICADOR =================================================================================

MensagemClassificador divideRespostaClassificador(char* resposta){ //FUNCAO FEITA PARA DIVIDIR A RESPOSTA QUE VEM DO CLASSIFICADOR
	MensagemClassificador dividida;

	char * token = strtok(resposta, " "); //DIVIDIR A RESPOSTA SEMPRE QUE ENCONTRA UM " "
	int i = 0;
   	while(token!= NULL){ //ENQUANTO A RESPOSTA AINDA NAO TIVER TODA DIVIDIDA
				   		 //COMO A RESPOSTA E CONSTITUIDA POR 'ESPECIALIDADE PRIORDADE(STRING INTEIRO)'
				   		 //NA PRIMEIRA ITERACAO DO CICLO BASTA COPIAR A STRING QUE NOS E DEVOLVIDA NO TOKEN
				   		 //NA SEGUNDA ITERACAO, CONVERTEMOS PARA INTEIRO E METEMOS DENTRO DA ESTRUTURA PARA DEPOIS A DEVOLVER
   		if(i == 0)
   			strcpy(dividida.especialidade, token);
   		else
   			dividida.prioridade = atoi(token);
   		i++;
   		token = strtok(NULL, " ");
   	}

   	return dividida;
}

void iniciaClassificador(){//Funcao para iniciar o Classificador

	//"Transformar" os arrays em pipe
	if(pipe(fdSintomas) < 0 || pipe(fdRespostas) < 0){
		printf("<BALCAO> Algo correu mal na criacao dos pipes anonimos\n");
		exit(-1);
	} 

	//DIVIDIR O PROCESSO EM DOIS PARA SER POSSIVEL FAZER O REDIRECIONAMENTO
	int res = fork(); 
	if(res == -1){
		printf("<BALCAO> Algo correu mal na funcao fork()\n");
		exit(-1);
	}

	if(res == 0){ //NO PROCESSO FILHO;
		//Substitui o stdin
		close(0);
		dup(fdSintomas[0]);
		close(fdSintomas[0]);
		close(fdSintomas[1]); //Fecha extremidade de escrita do pipe (Onde o pai vai escrever)

		//Substitui o stdout
		close(1);
		dup(fdRespostas[1]);
		close(fdRespostas[1]);
		close(fdRespostas[0]); //Fecha extremidade de leitura do pipe (Onde o pai vai ler)

		execl("classificador", "classificador", NULL); //EXECUTAR O PROGRAMA CLASSIFICADOR(FORNECIDO PELOS PROFESSORES)
		perror("Algo correu mal");
		exit(-1);
	}
	close(fdSintomas[0]);
	close(fdRespostas[1]);
}

MensagemClassificador EscreveParaOClassificador(char sintomas[]){

	char resposta[TAM_SINTOMAS];
	memset(resposta, '\0', sizeof(resposta));

	//Enviar resposta para o classificador	
	int nbytes = write(fdSintomas[1], sintomas, strlen(sintomas));
	if( nbytes < 0 ){
		fprintf(stderr, "<BALCAO> Erro ao escrever para o pipe");
		exit(-1);
	}

	//Ler resposta do classificador
	nbytes = read(fdRespostas[0], resposta, sizeof(resposta)); //VAMOS LER DA EXTREMIDADE DE LEITURA
	if( nbytes < 0 ) {
		fprintf(stderr, "<BALCAO> Erro ao ler do pipe de resposta");	
		exit(-1);
	}
	strcat(resposta, "\0");
	sintomas[strlen(sintomas)] = '\0';
	MensagemClassificador respostaClassificador = divideRespostaClassificador(resposta); //DIVIDIMOS A RESPOSTA QUE VEM PROVENIENTE DO CLASSIFICADOR
	return respostaClassificador; //Funcao para escrever para o classificador
}

void EncerrarClassificador(){
	int nbytes = write(fdSintomas[1], "#fim\n", strlen("#fim\n"));
	if( nbytes < 0 ){
		fprintf(stderr, "<BALCAO> Erro ao escrever para do pipe");
		exit(-1);
	}
	wait(&status);
	close(fdSintomas[1]);
	close(fdRespostas[0]); //Funcao para encerrar o classificador
}

//===========================================================================================================================================================================

//============================================================ FUNÇÕES FIFOS =============================================================================

//----------------------------------------------- MEDICO --------------------------------------------------------------
int criaFifoBalcaoMedico(){
	if(mkfifo(FIFO_BALCAO_MEDICO, 00777) == -1){
		if(errno != EEXIST){
			fprintf(stderr, "Erro ao criar o fifo do balcao que comunica com o medico");
			return -1;
		}
	}
	return 1;
}

int abreFifoBalcaoMedico(){
	int fdBalcao = open(FIFO_BALCAO_MEDICO, O_RDWR);
	if(fdBalcao == -1)
		fprintf(stderr, "Erro ao abrir fifo do balcao que comunica com o medico\n");
	return fdBalcao;
}

MensagemMedicoBalcao leFifoBalcaoMedico(){
	MensagemMedicoBalcao med;
	int fd;

	fd = open(FIFO_BALCAO_MEDICO, O_RDONLY);
		if(fd < 0)
			printf("[ERRO] ler do Fifo Balcao que comunica com o Medico");
	
	read(fd, &med, sizeof(MensagemMedicoBalcao));
	return med;

}

int abreFifoMedico(int pidMedico){
	char fifo[TAM_NOME];
	sprintf(fifo, FIFO_MEDICO, pidMedico);
	int fdMedico = open(fifo, O_RDWR);
	if(fdMedico == -1)
		fprintf(stderr, "Erro ao abrir fifo do medico %d\n", pidMedico);
	return fdMedico;
}

void escreveFifoMedico(int pidMedico, char* msg){
	int fd = abreFifoMedico(pidMedico);

	write(fd, msg, strlen(msg));

	close(fd);
}

//----------------------------------------------- CLIENTE --------------------------------------------------------------
int criaFifoBalcaoCliente(){
	if(mkfifo(FIFO_BALCAO_CLIENTE, 00777) == -1){
		if(errno != EEXIST){
			fprintf(stderr, "Erro ao criar o fifo do balcao");
			return -1;
		}
	}
	return 1;
}

int abreFifoBalcaoCliente(){
	int fdBalcao = open(FIFO_BALCAO_CLIENTE, O_RDWR);
	if(fdBalcao == -1)
	
		fprintf(stderr, "Erro ao abrir fifo do balcao\n");
	return fdBalcao;

}

int abreFifoCliente(int pidCliente){
	char fifo[TAM_NOME];
	sprintf(fifo, FIFO_CLIENTE, pidCliente);
	int fdCliente = open(fifo, O_RDWR);
	if(fdCliente == -1)
		fprintf(stderr, "Erro ao abrir fifo do cliente %d\n", pidCliente);
	return fdCliente;
}

MensagemClienteBalcao leFifoBalcaoCliente(int fdCliente){
	MensagemClienteBalcao mensagem;

	int nbytes = read(fdCliente, &mensagem, sizeof(MensagemClienteBalcao));
	if(nbytes != sizeof(MensagemClienteBalcao)){
		fprintf(stderr, "Erro ao ler do Fifo do Balcao que comunica com o Cliente");
		strcpy(mensagem.request,"ERRO");
	}
	return mensagem;
}

void escreveFifoCliente(int pidCliente, char* msg){
	int fd = abreFifoCliente(pidCliente);
	
	write(fd, msg, strlen(msg));
	fflush(stdout);
	close(fd);
}

//========================================================================================================================================================


//=================================================================== FUNÇÕES MEDICO =======================================================================================


void inicializaMedicos(int tamMedicos, medico array_medicos[]){
	for(int i = 0; i < tamMedicos; i++)
		array_medicos[i].pid_m = -1;

}

 
int verificaSeMedicoExiste(int pid, int tamMedicos, medico array_medicos[], pthread_mutex_t *mutex){

	pthread_mutex_lock(mutex);
	for(int i = 0; i < tamMedicos; i++){
		if(pid == array_medicos[i].pid_m){
			pthread_mutex_unlock(mutex);
			return 1;
		}
	}
	pthread_mutex_unlock(mutex);
	return 0;
}

 
void adicionaMedico(medico novoMedico, int tamMedicos, medico array_medicos[], pthread_mutex_t *mutex)
{

	pthread_mutex_lock(mutex);
	for(int i = 0; i < tamMedicos; i++){
		if(array_medicos[i].pid_m == -1){
			strcpy(array_medicos[i].nomeMedico, novoMedico.nomeMedico);
			strcpy(array_medicos[i].especialidadeMedico, novoMedico.especialidadeMedico);
			array_medicos[i].pid_m = novoMedico.pid_m;
			novoMedico.primeiraVez = 0;
			printf("<BALCAO> MEDICO %s Adicionado\n", novoMedico.nomeMedico);
			fflush(stdout);
			pthread_mutex_unlock(mutex);
			return;					// return ou break???
		}
	}
	pthread_mutex_unlock(mutex);
}

 
int escolheMedico(char especialidade[], int tamMedicos, medico array_medicos[], pthread_mutex_t *mutex){
	
	pthread_mutex_lock(mutex);
	for(int i = 0; i < tamMedicos; i++){
		if(array_medicos[i].pid_m != -1)
			if(strcmp(array_medicos[i].especialidadeMedico, especialidade) == 0 && array_medicos[i].emConsulta == 0){
				array_medicos[i].emConsulta = 1;
				pthread_mutex_unlock(mutex);
				return array_medicos[i].pid_m;
			}
	}
	printf("Nao Existe Nenhum Medico de |%s| Disponivel", especialidade);
	pthread_mutex_unlock(mutex);
	return -1;
}

 
int contaEspecialistasEspecialidade(char especialidade[], medico array_medicos[], int nrMedicos, pthread_mutex_t *mutex){
	int conta = 0;

	pthread_mutex_lock(mutex);
	for(int i = 0; i < nrMedicos; i++){
		if(strcmp(array_medicos[i].especialidadeMedico, especialidade) == 0)
			conta++;
	}
	pthread_mutex_unlock(mutex);
	return conta;
}

 
int contaMedicosDisponveisPorEspecialidade(char especialidade[], int tamMedicos, medico array_medicos[], pthread_mutex_t *mutex){
	int conta = 0;

	pthread_mutex_lock(mutex);	
	for(int i = 0; i < tamMedicos; i++){
		if(strcmp(especialidade, array_medicos[i].especialidadeMedico) == 0 && array_medicos[i].emConsulta == 0 && array_medicos[i].pid_m != -1)
			conta++;
	}
	pthread_mutex_unlock(mutex);
	return conta;
}

 
int verificaSePidEspecilistaExiste(int pid, int tamMedicos, medico array_medicos[], pthread_mutex_t *mutex){

	pthread_mutex_lock(mutex);	
	for(int i = 0; i < tamMedicos; i++){
		if(pid == array_medicos[i].pid_m){
			pthread_mutex_unlock(mutex);
			return 1;
		}
	}
	pthread_mutex_unlock(mutex);
	return 0;
}

int contaEspecialistas(int tamMedicos, medico array_medicos[], pthread_mutex_t* mutex)
{
	int conta = 0;
	pthread_mutex_lock(mutex);
	for(int i = 0; i < tamMedicos; i++)
		if(array_medicos[i].pid_m != -1)
			conta++;
	pthread_mutex_unlock(mutex);
	return conta;
}


void ListaEspecialistaEmEspera(int tamMedicos, medico array_medicos[], pthread_mutex_t *mutex){
    int conta = 0;

    printf("----------Especialistas A Espera----------\n");
	pthread_mutex_lock(mutex);	
    for(int i = 0; i < tamMedicos; i++){
        if(array_medicos[i].emConsulta == 0 && array_medicos[i].pid_m != -1){
            printf("\tID: %d\n\tNome: %s\n\tEspecialidade: %s\n",array_medicos[i].pid_m, array_medicos[i].nomeMedico, array_medicos[i].especialidadeMedico);
			conta++;
        }
    }
	pthread_mutex_unlock(mutex);
    if(conta == 0)
        printf("<BALCAO> NAO HA MEDICOS EM ESPERA\n");
    printf("------------------------------------------\n");
}

void ListaEspecialistasEmConsulta(int tamMedicos, medico array_medicos[], pthread_mutex_t *mutex){
    int conta = 0;

    printf("----------Utentes Em Consulta----------\n");
	pthread_mutex_lock(mutex);
    for(int i = 0; i < tamMedicos; i++){
        if(array_medicos[i].emConsulta == 1 && array_medicos[i].pid_m != -1){
            printf("\tID: %d\n\tNome: %s\n\tEspecialidade: %s\n",array_medicos[i].pid_m, array_medicos[i].nomeMedico, array_medicos[i].especialidadeMedico);
			conta++;
        }
    }
	pthread_mutex_unlock(mutex);
    if(conta == 0)
        printf("<BALCAO> NAO HA MEDICOS A DAR CONSULTA\n");
    printf("---------------------------------------\n");
}

 
int tornaEspecialistaLivre(int pid, int tamMedicos, medico array_medicos[], pthread_mutex_t *mutex){

	pthread_mutex_lock(mutex);	
	for(int i = 0; i < tamMedicos; i++){
		if(pid == array_medicos[i].pid_m)
			array_medicos[i].emConsulta = 0;
	}
	pthread_mutex_unlock(mutex);
}

medico procuraMedicoPorPid(int pid, int tamMedicos, medico array_medicos[], pthread_mutex_t *mutex)
{
	int i = 0;

	pthread_mutex_lock(mutex);
	while(pid != array_medicos[i].pid_m && i < tamMedicos){
		i++;
	};
	pthread_mutex_unlock(mutex);

	return array_medicos[i];
}

 
void removeEspecialista(int pid, int tamMedicos, medico array_medicos[], pthread_mutex_t *mutex){
	int i = 0;

	pthread_mutex_lock(mutex);
	for(i = 0; i < tamMedicos; i++){
		if(pid == array_medicos[i].pid_m){
			printf("<BALCAO> O especialista %s com o ID %d foi removido\n", array_medicos[i].nomeMedico, array_medicos[i].pid_m);
			fflush(stdout);
			array_medicos[i].pid_m = -1;
			break;
		}
	}
	
	pthread_mutex_unlock(mutex);
}
//===========================================================================================================================================================================


//=================================================================== FUNÇÕES CLIENTE =======================================================================================

void inicializaClientes(int tamClientes, cliente array_clientes[]){
	for(int i = 0; i < tamClientes; i++){
		array_clientes[i].pid_c = -1;
		array_clientes[i].especPrior.prioridade = 0;
	}
}
 
int verificaSeClienteExiste(int pid, int tamClientes, cliente array_clientes[], pthread_mutex_t* mutex){ //1 se existir, 0 se nao existir

	pthread_mutex_lock(mutex);
	for(int i = 0; i < tamClientes; i++)
		if(array_clientes[i].pid_c == pid){
			pthread_mutex_unlock(mutex);
			return 1;
		}
	pthread_mutex_unlock(mutex);
	return 0;
}

int verificaSeNomeExiste(char nome[], int tamClientes, cliente array_clientes[], pthread_mutex_t* mutex){

	pthread_mutex_lock(mutex);
	for(int i = 0; i < tamClientes; i++){
		if(strcmp(nome, array_clientes[i].nomeCliente) == 0 && array_clientes[i].pid_c != -1){
			pthread_mutex_unlock(mutex);
			return 1;
		}
	}
	pthread_mutex_unlock(mutex);
	return 0;
}
 
int contaClientes(int tamClientes, cliente array_clientes[], pthread_mutex_t* mutex)
{
	int conta = 0;
	pthread_mutex_lock(mutex);
	for(int i = 0; i < tamClientes; i++)
		if(array_clientes[i].pid_c != -1)
			conta++;
	pthread_mutex_unlock(mutex);
	return conta;
}

void adicionaCliente(cliente novoCliente, int tamClientes, cliente array_clientes[], MensagemBalcaoCliente resposta, pthread_mutex_t* mutex)
{
	pthread_mutex_lock(mutex);
	for(int i = 0; i < tamClientes; i++){
		if(array_clientes[i].pid_c == -1){
			array_clientes[i].pid_c = novoCliente.pid_c;
			strcpy(array_clientes[i].nomeCliente, novoCliente.nomeCliente);
			strcpy(array_clientes[i].sintomasCliente, novoCliente.sintomasCliente);
			strcpy(array_clientes[i].especPrior.especialidade, resposta.especialidadePrioridade.especialidade);
			array_clientes[i].especPrior.prioridade = resposta.especialidadePrioridade.prioridade;
			array_clientes[i].primeiraVez = 0;
			array_clientes[i].emConsulta = 0;
			pthread_mutex_unlock(mutex);
			return;
		}
	}
	pthread_mutex_unlock(mutex);
	return;
}

int contaClientesMesmaEspecialidade(char especialidade[], cliente array_clientes[], int nrClientes, int pid_cAtual, pthread_mutex_t* mutex){
	int conta = 0;

	pthread_mutex_lock(mutex);
	for(int i = 0; i < nrClientes; i++){
		if(array_clientes[i].pid_c != pid_cAtual && array_clientes[i].pid_c != -1){
			if(strcmp(array_clientes[i].especPrior.especialidade, especialidade) == 0)
				conta++;
		}
	}
	pthread_mutex_unlock(mutex);
	return conta;
}

int contaClientesDisponiveisEspecialidade(char especialidade[], cliente array_clientes[], int nrClientes, pthread_mutex_t* mutex){
	int conta = 0;

	pthread_mutex_lock(mutex);
	for(int i = 0; i < nrClientes; i++){
		if(array_clientes[i].pid_c != -1 && array_clientes[i].emConsulta == 0 && strcmp(array_clientes[i].especPrior.especialidade, especialidade) == 0){
				conta++;
		}
	}
	pthread_mutex_unlock(mutex);
	return conta;
}

cliente procuraClientePorNome(char ClienteARemover[], int tamClientes, cliente array_clientes[], pthread_mutex_t* mutex){
	int i = 0;
	pthread_mutex_lock(mutex);
	for(i = 0; i < tamClientes; i++){
		if(strcmp(ClienteARemover, array_clientes[i].nomeCliente) == 0){
			pthread_mutex_unlock(mutex);
			return array_clientes[i];
		}
	}
	pthread_mutex_unlock(mutex);
}

void organizaArrayClientesPorPrioridade(int tamClientes, cliente arrayClientesEspecialidade[], pthread_mutex_t* mutex){

	pthread_mutex_lock(mutex);
	for(int i = 0; i < tamClientes-1; i++){
		for(int j = i+1; j < tamClientes; j++){
			if(arrayClientesEspecialidade[j].especPrior.prioridade > arrayClientesEspecialidade[i].especPrior.prioridade){
				cliente aux = arrayClientesEspecialidade[i];
				arrayClientesEspecialidade[i] = arrayClientesEspecialidade[j];
				arrayClientesEspecialidade[j] = aux;
			}
		}
	}
	pthread_mutex_unlock(mutex);
}

void ListaClientesEmEspera(int tamClientes, cliente array_clientes[], pthread_mutex_t* mutex){
    int conta = 0;

    printf("----------Utentes Em Espera----------\n");
	pthread_mutex_lock(mutex);
    for(int i = 0; i < tamClientes; i++){
        if(array_clientes[i].emConsulta == 0 && array_clientes[i].pid_c != -1){
            printf("\tNome: %s\n\tSintomas: %s\n\tEspecialidade: %s\n\tPrioridade: %d\n", array_clientes[i].nomeCliente, array_clientes[i].sintomasCliente, array_clientes[i].especPrior.especialidade, array_clientes[i].especPrior.prioridade);
       		conta++;
		}
    }
	pthread_mutex_unlock(mutex);
    if(conta == 0)
        printf("<BALCAO> NAO HA CLIENTES EM LISTA DE ESPERA\n");
    printf("-------------------------------------\n");
}

void ListaClientesASeremAtendidos(int tamClientes, cliente array_clientes[], pthread_mutex_t* mutex){
    int conta = 0;

    printf("----------Utentes A Serem Atendidos----------\n");
	pthread_mutex_lock(mutex);
    for(int i = 0; i < tamClientes; i++){
        if(array_clientes[i].emConsulta == 1 && array_clientes[i].pid_c != -1){
            printf("\tNome: %s\n\tSintomas: %s\n\tEspecialidade: %s\n\tPrioridade: %d\n", array_clientes[i].nomeCliente, array_clientes[i].sintomasCliente, array_clientes[i].especPrior.especialidade, array_clientes[i].especPrior.prioridade);
			conta++;
		}
    }
	pthread_mutex_unlock(mutex);
    if(conta == 0)
        printf("<BALCAO> NAO HA CLIENTES A SEREM ATENDIDOS\n");
    printf("---------------------------------------------\n");
}

void criaArrayClientesEspecialidade(char especialidade[], int tamClientes, cliente array_clientes[], cliente arrayClientesEspecialidade[], pthread_mutex_t* mutex){
	inicializaClientes(5, arrayClientesEspecialidade);
	int j = 0;
	
	pthread_mutex_lock(mutex);
	for(int i = 0; i < tamClientes && j < 5; i++){
		if(array_clientes[i].pid_c != -1 && array_clientes[i].emConsulta == 0 && strcmp(array_clientes[i].especPrior.especialidade, especialidade) == 0){
			arrayClientesEspecialidade[j] = array_clientes[i];
			j++;			
		}
	}
	pthread_mutex_unlock(mutex);
	organizaArrayClientesPorPrioridade(5, arrayClientesEspecialidade, mutex);
}

void removeCliente(char ClienteARemover[], int tamClientes, cliente array_clientes[], pthread_mutex_t* mutex){
	int i = 0;

	pthread_mutex_lock(mutex);
	for(i = 0; i < tamClientes; i++){
		if(strcmp(ClienteARemover, array_clientes[i].nomeCliente) == 0){
			array_clientes[i].pid_c = -1;
			printf("<BALCAO> O cliente %s foi removido\n", array_clientes[i].nomeCliente);
			fflush(stdout);
			break;
		}
	}
	pthread_mutex_unlock(mutex);
}
 
void terminaClientes(int tamClientes, cliente array_clientes[], pthread_mutex_t* mutex)
{
	pthread_mutex_lock(mutex);
	for(int i = 0; i < tamClientes; i++){
		if(array_clientes[i].pid_c != -1){
			escreveFifoCliente(array_clientes[i].pid_c, "TERMINA");
		}
	}
	pthread_mutex_unlock(mutex);
}

//===========================================================================================================================================================================

void limpaArraySinalVida(int arraySinalVida[], int tamanhoArray, pthread_mutex_t *mutex){

	pthread_mutex_lock(mutex);
	for(int i = 0; i < tamanhoArray; i++)
		arraySinalVida[i] = -1;
	pthread_mutex_unlock(mutex);
}

void inputNoArraySinalVida(int arraySinalVida[], int tamArray, int pid, pthread_mutex_t *mutex){

	pthread_mutex_lock(mutex);
	for(int i = 0; i < tamArray; i++){
		if(arraySinalVida[i] == -1){
			arraySinalVida[i] = pid;
			break;
		}
	}
	pthread_mutex_unlock(mutex);
}

void verificarArraySinalVida(int arraySinalVida[], medico array_medicos[], int tamMedicos, pthread_mutex_t *mutexMedico){
	int flag;
	pthread_mutex_lock(mutexMedico);
	for(int i = 0; i < tamMedicos; i++){
		flag = 0;
		for(int j = 0; j < tamMedicos; j++){
			if(array_medicos[i].pid_m == arraySinalVida[j] && array_medicos[i].pid_m != -1){
				flag = 1;
			}
		}
		if(flag == 0 && array_medicos[i].pid_m != -1)
			removeEspecialista(array_medicos[i].pid_m, tamMedicos, array_medicos, mutexMedico);
	}
	pthread_mutex_unlock(mutexMedico);

}


//============================================================================ FUNÇÕES THREADS ===========================================================================

//----------------------------------------------------------------------------- MEDICO ------------------------------------------------------------------------------	

void* funcaoThreadLeMedico(void* argumentos){
	char msg[40];
	threadDataMedico* tDataMedico = (threadDataMedico*) argumentos;
	do{
		MensagemMedicoBalcao m = leFifoBalcaoMedico();
		if(strcmp(m.request, "termina") != 0 && strcmp(m.request,"desistir") != 0){
			if(verificaSeMedicoExiste(m.med.pid_m, tDataMedico->varAmb->MAX_MEDICOS, tDataMedico->array_medicos, tDataMedico->mutexMedico) == 0){
				if(contaEspecialistas(tDataMedico->varAmb->MAX_MEDICOS, tDataMedico->array_medicos, tDataMedico->mutexMedico) + 1 > tDataMedico->varAmb->MAX_MEDICOS)
				{
					escreveFifoMedico(m.med.pid_m, "Lista cheia");
				}
				else
				{
					adicionaMedico(m.med, tDataMedico->varAmb->MAX_MEDICOS, tDataMedico->array_medicos, tDataMedico->mutexMedico);
					
					escreveFifoMedico(m.med.pid_m, "ADICIONADO");

					inputNoArraySinalVida(tDataMedico->arraySinalVida, tDataMedico->varAmb->MAX_MEDICOS, m.med.pid_m, tDataMedico->mutexSinalVida);	
				}
			}
			else{
				if(strcmp(m.request, "Livre") == 0){
					tornaEspecialistaLivre(m.med.pid_m, tDataMedico->varAmb->MAX_MEDICOS, tDataMedico->array_medicos, tDataMedico->mutexMedico);
					printf("<BALCAO> O medico %s saiu da consulta e ja se encontra livre\n", m.med.nomeMedico);
				}
				if(strcmp(m.request, "VIVO") == 0){
					inputNoArraySinalVida(tDataMedico->arraySinalVida, tDataMedico->varAmb->MAX_MEDICOS, m.med.pid_m, tDataMedico->mutexSinalVida);	
				}
			}
		}
		if(strcmp(m.request,"desistir") == 0){
			removeEspecialista(m.med.pid_m, tDataMedico->varAmb->MAX_MEDICOS, tDataMedico->array_medicos, tDataMedico->mutexMedico);
		}
	}while(*(tDataMedico->continua) != 1);

	pthread_exit(NULL);
}

void terminaMedicos(int tamMedicos, medico array_medicos[], pthread_mutex_t *mutex){

	pthread_mutex_lock(mutex);
	for(int i = 0; i < tamMedicos; i++){
		if(array_medicos[i].pid_m != -1){
			escreveFifoMedico(array_medicos[i].pid_m, "TERMINA");
		}
	}
	pthread_mutex_unlock(mutex);
}

int terminaThreadMedico(int fdBalcaoMedico){
	medico med;
	MensagemMedicoBalcao msgFecharThread;
	strcpy(msgFecharThread.request, "termina");

	write(fdBalcaoMedico, &msgFecharThread, sizeof(MensagemMedicoBalcao));
}

//----------------------------------------------------------------------------- CLIENTE -----------------------------------------------------------------------------

void* funcaoThreadLeCliente(void* argumentos){
	threadDataCliente* TDataCliente = (threadDataCliente*) argumentos;

	do{
		MensagemBalcaoCliente resposta;
		MensagemClienteBalcao mensagemCliente = leFifoBalcaoCliente(TDataCliente->fd);
		if(strcmp(mensagemCliente.request,"termina") != 0 && strcmp(mensagemCliente.request,"desistir") != 0){

			if(verificaSeClienteExiste(mensagemCliente.esteCliente.pid_c, TDataCliente->varAmb->MAX_CLIENTES, TDataCliente->array_clientes, TDataCliente->mutexClientes) == 0){

				if(verificaSeNomeExiste(mensagemCliente.esteCliente.nomeCliente, TDataCliente->varAmb->MAX_CLIENTES, TDataCliente->array_clientes, TDataCliente->mutexClientes) == 0){
					resposta.especialidadePrioridade = EscreveParaOClassificador(mensagemCliente.esteCliente.sintomasCliente);
				
					//Preenche mensagem de resposta			
					resposta.nrEspecialistasEspecialidade = contaEspecialistasEspecialidade(resposta.especialidadePrioridade.especialidade, TDataCliente->array_medicos, TDataCliente->varAmb->MAX_MEDICOS, TDataCliente->mutexMedicos);
					resposta.nrUtentesAFrenteEspecialidade = contaClientesMesmaEspecialidade(resposta.especialidadePrioridade.especialidade, TDataCliente->array_clientes, TDataCliente->varAmb->MAX_CLIENTES, mensagemCliente.esteCliente.pid_c, TDataCliente->mutexClientes);

					if(resposta.nrUtentesAFrenteEspecialidade >= 5 || contaClientes(TDataCliente->varAmb->MAX_CLIENTES, TDataCliente->array_clientes, TDataCliente->mutexClientes) + 1 > TDataCliente->varAmb->MAX_CLIENTES){
						strcpy(resposta.request, "Lista de espera cheia");
					}
					else{
						adicionaCliente(mensagemCliente.esteCliente,TDataCliente->varAmb->MAX_CLIENTES, TDataCliente->array_clientes, resposta, TDataCliente->mutexClientes);
						strcpy(resposta.request, "Adicionado");
						printf("<BALCAO> O CLIENTE %s FOI ADICIONADO\n", mensagemCliente.esteCliente.nomeCliente);
					}
				}
				else{
					strcpy(resposta.request, "Nome ja existe");
				}

			}

			int fdCliente = abreFifoCliente(mensagemCliente.esteCliente.pid_c);
			int nbytes = write(fdCliente, &resposta, sizeof(MensagemBalcaoCliente));
			if(nbytes < sizeof(MensagemBalcaoCliente)){
				fprintf(stderr, "Erro ao ler do fifo do balcao\n");
				exit(-1);
			}
			close(fdCliente);

		}
		if(strcmp(mensagemCliente.request,"desistir") == 0){
			if(verificaSeClienteExiste(mensagemCliente.esteCliente.pid_c, TDataCliente->varAmb->MAX_CLIENTES, TDataCliente->array_clientes, TDataCliente->mutexClientes) == 1){
				removeCliente(mensagemCliente.esteCliente.nomeCliente,TDataCliente->varAmb->MAX_CLIENTES, TDataCliente->array_clientes, TDataCliente->mutexClientes);
			}
		}
	}while(*(TDataCliente->continua) != 1);

	pthread_exit(NULL);
}

int terminaThreadCliente(int fdBalcaoCliente){
	cliente Clientetemp;
	MensagemClienteBalcao msgFecharThread;
	strcpy(msgFecharThread.request, "termina");

	int nbytes = write(fdBalcaoCliente, &msgFecharThread, sizeof(MensagemClienteBalcao));
	if(nbytes < sizeof(MensagemClienteBalcao)){
		fprintf(stderr, "Erro terminar thread cliente\n");
		return -1;
	}
	return nbytes;
}

//----------------------------------------------------------------------------- BALCAO -----------------------------------------------------------------------------


void* funcaoThreadVerificaSinalVida(void* argumentos){
	ThreadRecebeSinalVida* dados = (ThreadRecebeSinalVida*) argumentos;

	do{
		sleep(10);
		verificarArraySinalVida(dados->arraySinalVida, dados->array_medicos, dados->varAmb->MAX_MEDICOS, dados->mutexMedico);
		limpaArraySinalVida(dados->arraySinalVida, dados->varAmb->MAX_MEDICOS, dados->mutexSinalVida);
	}while(*(dados->continua) != 1);

	pthread_exit(NULL);
}

void* funcaoThreadMostraListaEspera(void* argumentos){
	threadMostraListaEspera* tMostraListaEspera = (threadMostraListaEspera*) argumentos;
	do{
		int i = 1;
		for(i; i <= *(tMostraListaEspera->tempo); i++)
			sleep(1);
		ListaClientesEmEspera(tMostraListaEspera->varAmb->MAX_CLIENTES, tMostraListaEspera->array_clientes, tMostraListaEspera->mutexCliente);
	}while(*(tMostraListaEspera->continua)!= 1);
	pthread_exit(NULL);
}

void* funcaoThreadConsulta(void* argumentos){
	threadConsulta* tConsulta = (threadConsulta*) argumentos;
	
	do{
		sleep(10);
		int j = 0;
		for(int i = 0; i < 5; i++){

			int nrMedicosEspecialidade = contaMedicosDisponveisPorEspecialidade(especialidadesDisponiveis[i], tConsulta->varAmb->MAX_MEDICOS, tConsulta->array_medicos, tConsulta->mutexMedico);
			int nrUtentesEspecialidade = contaClientesDisponiveisEspecialidade(especialidadesDisponiveis[i], tConsulta->array_clientes, tConsulta->varAmb->MAX_CLIENTES, tConsulta->mutexCliente);
			
			if(nrMedicosEspecialidade > 0 && nrUtentesEspecialidade > 0){
				
				cliente arrayClientesEspecialidade[5]; 
				criaArrayClientesEspecialidade(especialidadesDisponiveis[i], tConsulta->varAmb->MAX_CLIENTES, tConsulta->array_clientes, arrayClientesEspecialidade, tConsulta->mutexCliente);
				
				while(j < 5 && nrMedicosEspecialidade > 0 && nrUtentesEspecialidade > 0){
					char strMedico[TAM_NOME];
					char strCliente[TAM_NOME];

					//Notifica medico
					int pidMedico = escolheMedico(especialidadesDisponiveis[i], tConsulta->varAmb->MAX_MEDICOS, tConsulta->array_medicos, tConsulta->mutexMedico);
					sprintf(strMedico, STRCONSULTA, arrayClientesEspecialidade[j].pid_c);
					escreveFifoMedico(pidMedico, strMedico);
					
					//notifica cliente
					sprintf(strCliente, STRCONSULTA, pidMedico);
					escreveFifoCliente(arrayClientesEspecialidade[j].pid_c, strCliente);
					arrayClientesEspecialidade[j].emConsulta = 1;

					for(int i = 0; i < tConsulta->varAmb->MAX_CLIENTES; i++)
					{
						if(tConsulta->array_clientes[i].pid_c == arrayClientesEspecialidade[j].pid_c){
							tConsulta->array_clientes[i].emConsulta = 1;
						}
					}

					j++;
					nrUtentesEspecialidade--;
					nrMedicosEspecialidade--;
				}	
			}
		}
	}while(*(tConsulta->continua) != 1);
	pthread_exit(NULL);
}

//========================================================================================================================================================================

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

void help(){ //Balcao
    printf("Comandos:\n\tutentes - para listar os utentes\n\tespecialistas - para lista os especialistas\n\tdelut x - para remover o utente x (x e o nome do cliente)\n\tdelesp x - para remover o especialista x (x e o id do medico)\n\tfreq n - para mudar o tempo da apresentacao das listas de espera para n segundos\n\tencerra - no caso de querer fechar o balcao e todos os seus utentes e medicos\n\t");
}

void interpretaComando(char* comando, variaveisAmbiente varAmb, int* tempo, cliente array_clientes[], medico array_medicos[], pthread_mutex_t* mutexMedico, pthread_mutex_t* mutexCliente){
	char first_arg[TAM_SINTOMAS/2], second_arg[TAM_SINTOMAS/2];

	if(verificaEspaco(comando) == 1){
		sscanf(comando,"%s %s", first_arg, second_arg);
		if(strcmp("freq", first_arg) == 0){
			*(tempo) = atoi(second_arg);
		}
		if(strcmp("delut", first_arg) == 0){
			if(verificaSeNomeExiste(second_arg, varAmb.MAX_CLIENTES, array_clientes, mutexCliente) == 1){
				cliente ClienteARemover = procuraClientePorNome(second_arg, varAmb.MAX_CLIENTES, array_clientes, mutexCliente);
				if(ClienteARemover.emConsulta == 0){
					escreveFifoCliente(ClienteARemover.pid_c , "TERMINA");
					removeCliente(ClienteARemover.nomeCliente, varAmb.MAX_CLIENTES, array_clientes, mutexCliente);
				}
				else
				{
					printf("<BALCAO> O CLIENTE ENCONTRA-SE EM CONSULTA E NAO PODE SER REMOVIDO\n");
				}
			}
			else
				printf("\n<BALCAO> CLIENTE COM O NOME %s NAO EXISTE\n", second_arg);
		}
		if(strcmp("delesp", first_arg) == 0){
			if(verificaSePidEspecilistaExiste(atoi(second_arg), varAmb.MAX_MEDICOS, array_medicos, mutexMedico) == 1)
			{
				medico MedicoARemover = procuraMedicoPorPid(atoi(second_arg), varAmb.MAX_MEDICOS, array_medicos, mutexMedico);
				if(MedicoARemover.emConsulta == 0)
				{
					int fdMedico = abreFifoMedico(atoi(second_arg));
					int nbytes = write(fdMedico, "TERMINA", 7);
					removeEspecialista(atoi(second_arg), varAmb.MAX_MEDICOS, array_medicos, mutexMedico);
				}
				else
				{
					printf("<BALCAO> O MEDICO ENCONTRA-SE EM CONSULTA E NAO PODE SER REMOVIDO\n");
				}
			}
			else
				printf("\n<BALCAO> MEDICO COM O NOME %s NAO EXISTE\n", second_arg);
		}
	}else{
		if(strcmp(comando, "utentes\n") == 0){
			ListaClientesASeremAtendidos(varAmb.MAX_CLIENTES, array_clientes, mutexCliente);
			ListaClientesEmEspera(varAmb.MAX_CLIENTES, array_clientes, mutexCliente);
		}
			
		else if(strcmp(comando, "especialistas\n") == 0){
			ListaEspecialistaEmEspera(varAmb.MAX_MEDICOS, array_medicos, mutexMedico);
			ListaEspecialistasEmConsulta(varAmb.MAX_MEDICOS, array_medicos, mutexMedico);
		}else if(strcmp(comando, "help\n") == 0)
			help();
	}
}

int main(int argc, char* argv[]){
	//Ir buscar as variaveis de ambiente
	variaveisAmbiente varAmb;
	obtemVariaveisAmbiente(&varAmb);

	//Mutex Cliente
	//Mutex Medico
	//Mutex Array Sinal de Vida

	pthread_mutex_t medicosMutex;
	pthread_mutex_t clientesMutex;
	pthread_mutex_t sinalVidaMutex;

	pthread_mutex_init(&medicosMutex, NULL);
	pthread_mutex_init(&clientesMutex, NULL);
	pthread_mutex_init(&sinalVidaMutex, NULL);

	int arraySinalVida[varAmb.MAX_MEDICOS]; //Array para guardar quem envia sinal de vida
	limpaArraySinalVida(arraySinalVida, varAmb.MAX_MEDICOS, &sinalVidaMutex);
	
	int continua = 0; //Variavel de paragem de thread
	int tempo = 30; //Tempo para mostrar a lista de espera

	//Alocacao de memoria
	cliente array_clientes[varAmb.MAX_CLIENTES]; //Clientes
	medico array_medicos[varAmb.MAX_MEDICOS]; //Medicos
	
	inicializaClientes(varAmb.MAX_CLIENTES, array_clientes);
	inicializaMedicos(varAmb.MAX_MEDICOS, array_medicos);

	//CRIAR THREAD LISTA ESPERA
	threadMostraListaEspera tMostraListaEspera;
	pthread_t tidMostraListaEspera;
	tMostraListaEspera.varAmb = &varAmb;
	tMostraListaEspera.array_clientes = array_clientes;
	tMostraListaEspera.continua = &continua;
	tMostraListaEspera.tempo = &tempo;
	tMostraListaEspera.tid = tidMostraListaEspera;
	tMostraListaEspera.mutexCliente = &clientesMutex;

	int res = pthread_create(&tMostraListaEspera.tid, NULL, funcaoThreadMostraListaEspera, (void*) &tMostraListaEspera);
	if(res != 0){
		fprintf(stderr, "[ERRO] Criar Thread Para Mostrar Lista De Espera");
		exit(-1);
	}

	//CRIAR THREAD CONSULTA
	threadConsulta tConsulta;
	pthread_t tidThreadConsulta;
	tConsulta.varAmb = &varAmb;
	tConsulta.array_clientes = array_clientes;
	tConsulta.array_medicos = array_medicos;
	tConsulta.continua = &continua;
	tConsulta.tid = tidMostraListaEspera;
	tConsulta.mutexMedico = &medicosMutex;
	tConsulta.mutexCliente = &clientesMutex;

	res = pthread_create(&tConsulta.tid, NULL, funcaoThreadConsulta, (void*) &tConsulta);
	if(res != 0){
		fprintf(stderr, "[ERRO] Criar Thread Medico");
		exit(-1);
	}

	//CRIAR THREAD VERIFICA SINAL DE VIDA
	ThreadRecebeSinalVida tRecebeSinalVida;
	pthread_t tidRecebeSinalVida;
	tRecebeSinalVida.tid = tidRecebeSinalVida;
	tRecebeSinalVida.varAmb = &varAmb;
	tRecebeSinalVida.array_medicos = array_medicos;
	tRecebeSinalVida.arraySinalVida = arraySinalVida;
	tRecebeSinalVida.continua = &continua;
	tRecebeSinalVida.mutexMedico = &medicosMutex;
	tRecebeSinalVida.mutexSinalVida = &sinalVidaMutex;

	res = pthread_create(&tRecebeSinalVida.tid, NULL, funcaoThreadVerificaSinalVida, (void*)&tRecebeSinalVida);
	if(res != 0){
		fprintf(stderr, "[ERRO] Criar Thread Medico");
		exit(-1);
	}

	iniciaClassificador();

	if(criaFifoBalcaoMedico() == -1){
		EncerrarClassificador();
		exit(-1);
	}

	int fdBalcaoMedico = abreFifoBalcaoMedico();
	if(fdBalcaoMedico == -1){
		EncerrarClassificador();
		exit(-1);
	}
	if(criaFifoBalcaoCliente() == -1){
		EncerrarClassificador();
		exit(-1);
	}
	int fdBalcaoCliente = abreFifoBalcaoCliente();
	if(fdBalcaoCliente == -1){
		EncerrarClassificador();
		close(fdBalcaoMedico);
		unlink(FIFO_BALCAO_MEDICO);
		unlink(FIFO_BALCAO_CLIENTE);
		exit(-1);
	}

// ---------------------------	MEDICO -----------------------------------
	threadDataMedico tDataMedico;
	pthread_t p;
	tDataMedico.varAmb = &varAmb;
	tDataMedico.array_medicos = array_medicos;	
	tDataMedico.p = p;
	tDataMedico.continua = &continua;
	tDataMedico.arraySinalVida = arraySinalVida;
	tDataMedico.mutexMedico = &medicosMutex;
	tDataMedico.mutexSinalVida = &sinalVidaMutex;

	res = pthread_create(&tDataMedico.p, NULL, funcaoThreadLeMedico,(void*) &tDataMedico);
	if(res != 0){
		fprintf(stderr, "[ERRO] Criar Thread Medico");
		EncerrarClassificador();
		close(fdBalcaoMedico);
		close(fdBalcaoCliente);
		unlink(FIFO_BALCAO_MEDICO);
		unlink(FIFO_BALCAO_CLIENTE);
		exit(-1);
	}

// ------------------------------------------------------------------------

// ---------------------------	CLIENTE -----------------------------------
	
	threadDataCliente tDataCliente;
	pthread_t tidCliente;
	tDataCliente.varAmb = &varAmb;
	tDataCliente.array_clientes = array_clientes;
	tDataCliente.array_medicos = array_medicos;
	tDataCliente.tid = tidCliente;
	tDataCliente.continua = &continua;
	tDataCliente.fd = fdBalcaoCliente;
	tDataCliente.mutexMedicos = &medicosMutex;
	tDataCliente.mutexClientes = &clientesMutex;

	res = pthread_create(&tDataCliente.tid, NULL, funcaoThreadLeCliente,(void*) &tDataCliente);
	if(res != 0){
		fprintf(stderr, "[ERRO] Criar Thread Cliente");
		EncerrarClassificador();
		close(fdBalcaoMedico);
		close(fdBalcaoCliente);
		unlink(FIFO_BALCAO_MEDICO);
		unlink(FIFO_BALCAO_CLIENTE);
		exit(-1);
	}

// ------------------------------------------------------------------------
	
	char cmd[TAM_SINTOMAS]; //Comando para ser lido do teclado
	do{
		printf("Comando > ");
		fgets(cmd, sizeof(cmd), stdin);
		interpretaComando(cmd, varAmb, &tempo, array_clientes, array_medicos, &medicosMutex, &clientesMutex);
	}while(strcmp(cmd, "encerra\n") != 0);
	continua = 1;

	  
	terminaMedicos(varAmb.MAX_MEDICOS, array_medicos, &medicosMutex);
	terminaClientes(varAmb.MAX_CLIENTES, array_clientes, &clientesMutex);
	
	//Permina Threads
	terminaThreadCliente(fdBalcaoCliente);
	terminaThreadMedico(fdBalcaoMedico);

	printf("<BALCAO>VOU JUNTAR AS THREADS\n");
	pthread_join(tMostraListaEspera.tid, NULL);
	printf("<BALCAO>JUNTEI A THREAD QUE LISTA A LISTA DE ESPERA\n");
	pthread_join(tDataMedico.p, NULL); 
	printf("<BALCAO>JUNTEI A THREAD DO MEDICO\n");
	pthread_join(tDataCliente.tid, NULL);
	printf("<BALCAO>JUNTEI A THREAD DO CLIENTE\n");
	pthread_join(tRecebeSinalVida.tid, NULL);
	printf("<BALCAO>JUNTEI A THREAD DO SINAL DE VIDA\n");
	pthread_join(tConsulta.tid, NULL);
	printf("<BALCAO>JUNTEI A THREAD DO CONSULTA\n");
	printf("<BALCAO>THREADS JUNTADAS\n");	

	pthread_mutex_destroy(&medicosMutex);
	pthread_mutex_destroy(&clientesMutex);
	pthread_mutex_destroy(&sinalVidaMutex);
	
	EncerrarClassificador();
	printf("<BALCAO>ENCERREI O CLASSIFICADOR\n");

	close(fdBalcaoCliente);
	close(fdBalcaoMedico);
	
	unlink(FIFO_BALCAO_MEDICO);
	unlink(FIFO_BALCAO_CLIENTE);
	
	return 0;
}