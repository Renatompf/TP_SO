#include "header_files/var_amb.h"
#include "header_files/balcao.h"
#include "header_files/cliente.h"
#include "header_files/medico.h"

int fdMedico = 0;

cliente inicializaCliente(char* nomecliente){
	cliente c;
	c.pid_c = getpid();
	strcpy(c.nomeCliente, nomecliente);
	c.primeiraVez = 1;
	c.emConsulta = 0;
	printf("Indique quais sao os seus sintomas: ");
	fgets(c.sintomasCliente, sizeof(c.sintomasCliente),stdin);
	return c;
}

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

void help(){ //Clientes
    printf("Comandos:\n\t -> adeus - para sair e terminar o cliente\n");
}


//============================================== FUNÇÕES FIFOS =================================================
int criaFifoCliente(cliente* client, char* fifo){
	int fifocliente;
	sprintf(fifo, FIFO_CLIENTE, client->pid_c);

	if(mkfifo(fifo, 00777) == -1){
		if(errno != EEXIST){
			fprintf(stderr, "Erro ao criar o fifo do cliente com o pid %d\n");
			return -1;
		}
	}
	return 1;
}

int abreFifoBalcao(){
	int fdBalcao = open(FIFO_BALCAO_CLIENTE, O_RDWR);
	if(fdBalcao == -1)
		fprintf(stderr, "Erro ao abrir fifo do balcao\n");
	return fdBalcao;
}

int abreFifoCliente(char* fifo){
	int fdCliente = open(fifo, O_RDWR);
	if(fdCliente == -1)
		fprintf(stderr, "Erro ao abrir fifo do cliente\n");
	return fdCliente;
}

int abreFifoMedico(int pid){
	char fifo[TAM_NOME];
	sprintf(fifo, FIFO_MEDICO, pid);
	int fdMedico = open(fifo, O_RDWR | O_NONBLOCK);
	if(fdMedico == -1)
		fprintf(stderr, "Erro ao abrir fifo do medico %d\n", pid);
	return fdMedico;
}

int escreveFifoBalcao(int fdBalcao, cliente* client, char request[]){
	MensagemClienteBalcao message;
	message.esteCliente = *client;
	if(client->primeiraVez == 1){
		strcpy(message.request,request);
		int nbytes = write(fdBalcao, &message, sizeof(MensagemClienteBalcao));
		if(nbytes != sizeof(MensagemClienteBalcao))
			fprintf(stderr, "Erro ao escrever no fifo do balcao\n");
		return nbytes;
	}
	else{
		strcpy(message.request,request);
		int nbytes = write(fdBalcao, &message, sizeof(MensagemClienteBalcao));
		if(nbytes != sizeof(MensagemClienteBalcao))
			fprintf(stderr, "Erro ao escrever no fifo do balcao\n");
		return nbytes;
	}
}

int lerFifoCliente(int fdCliente, cliente* client, int* continua){
	int nbytes;

	if(client->emConsulta == 0){
		if(client->primeiraVez == 1){
			MensagemBalcaoCliente retorno;
			nbytes = read(fdCliente, &retorno, sizeof(MensagemBalcaoCliente));
			if(nbytes != sizeof(MensagemBalcaoCliente))
				fprintf(stderr, "Erro ao ler no fifo do balcao\n");
			if(strcmp(retorno.request, "Nome ja existe") == 0)
			{
				printf("\n<CLIENTE> ESTE NOME JA EXISTE E POR ISSO O CLIENTE VAI TERMINAR\n", retorno.request);
				*continua = 1;
				return 0;
			}
			else if(strcmp(retorno.request, "Lista de espera cheia") == 0)
			{
				printf("\n<CLIENTE> A LISTA DE ESPERA ESTA CHEIA E POR ISSO O CLIENTE VAI TERMINAR\n", retorno.request);
				*continua = 1;
				return 0;
			}
			else{
				printf("Li %s %d\n", retorno.especialidadePrioridade.especialidade, retorno.especialidadePrioridade.prioridade);
				client->primeiraVez = 0;
				strcpy(client->especPrior.especialidade, retorno.especialidadePrioridade.especialidade);
				client->especPrior.prioridade = retorno.especialidadePrioridade.prioridade;
				printf("Especialidade: %s\nPrioridade: %d\nNumero de Medicos Disponiveis Para Especialidade: %d\nNumero de Utentes A Frente Para Especialidade: %d\n\n",
				client->especPrior.especialidade, client->especPrior.prioridade, retorno.nrEspecialistasEspecialidade, retorno.nrUtentesAFrenteEspecialidade);
				return nbytes;
			}	
		}else{
			char msg[TAM_SINTOMAS];
			char first_arg[TAM_SINTOMAS/2], second_arg[TAM_SINTOMAS/2];
			nbytes = read(fdCliente, msg, sizeof(msg));
			fflush(stdout);
			if(verificaEspaco(msg) == 1){
				sscanf(msg,"%s %s", first_arg, second_arg);
				if(strcmp(first_arg, "Consulta") == 0){
					client->emConsulta = 1;
					fdMedico = abreFifoMedico(atoi(second_arg));
					printf("\n<CLIENTE> VAI ENTRAR NUMA CONSULTA COM O MEDICO COM O ID %d\n", atoi(second_arg));
				} 
			}
			else{
				if(strcmp(msg, "TERMINA") == 0){
					*continua = 1;
					printf("\n<CLIENTE> O BALCAO VAI ENCERRAR, O CLIENTE TAMBEM\n");
					fflush(stdout);
					return 0;
				}
			}
		}
	}
	else{
		char msg[TAM_MENSAGEM];
		nbytes = read(fdCliente, msg, sizeof(msg));
		printf("\nResposta do Especialista: %s\n");
		if(strcmp(msg, "TERMINA") == 0 || strcmp(msg, "adeus\n") == 0){
			*continua = 1;
			fflush(stdout);
			return 0;
		}
	}
}
//==============================================================================================================


int main(int argc, char* argv[]){
	char fifocliente[TAM_NOME];
	fd_set fds;
	int continua = 0;

	if(argc != 2){
		fprintf(stderr,"<CLIENTE>Formato de utilizacao: ./cliente <NomeCliente>\n<CLIENTE>Numero de argumentos errados\n");
		exit(-1);
	}

	if(access(FIFO_BALCAO_CLIENTE, F_OK) != 0){
		fprintf(stderr, "O Balcao nao se encontra ativo\n");
		exit(-1);
	}

	//Inicializacao do cliente e criacao do respetivo FIFO
	cliente client = inicializaCliente(argv[1]);
	if(criaFifoCliente(&client, fifocliente) == -1)
		exit(-1);
	//Abertura dos fifos
	int fdBalcao = abreFifoBalcao();
	if(fdBalcao == -1)
		exit(-1);
	int fdcliente = abreFifoCliente(fifocliente);
	if(fdcliente == -1){
		close(fdBalcao);
		exit(-1);
	}

	//Leitura e escrita de dados dos fifos
	int nbytes = escreveFifoBalcao(fdBalcao, &client, "Primeira comunicacao");

	char cmd[TAM_MENSAGEM]; //Comando para ser lido do teclado
	do{
		if(client.emConsulta == 0)
			printf("Comando: ");
		else
			printf("Pergunta para o Especialista: ");
		fflush(stdout);
		
		FD_ZERO(&fds);
		FD_SET(0, &fds);
		FD_SET(fdcliente, &fds);

		int res = select(fdcliente+1, &fds, NULL, NULL, NULL);
		if(res > 0 && FD_ISSET(0, &fds)){ //Leu do teclado
			fgets(cmd, sizeof(cmd), stdin);
			if(client.emConsulta == 1)
				write(fdMedico, cmd, sizeof(cmd));
			else{
				printf("Recebi o comando > %s\n", cmd);
				if(strcmp(cmd, "help\n") == 0)
					help();
			}
		}
		else if(res > 0 && FD_ISSET(fdcliente, &fds)) //leu do fifo dele
			nbytes = lerFifoCliente(fdcliente, &client, &continua);
		
	}while(strcmp(cmd, "adeus\n") != 0 && continua == 0);

	if(fdMedico != 0)
		close(fdMedico);

	escreveFifoBalcao(fdBalcao, &client, "desistir");
	
	close(fdBalcao);
	close(fdcliente);
	unlink(fifocliente);
	return 0;
}