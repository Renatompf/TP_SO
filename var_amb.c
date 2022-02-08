#include "header_files/var_amb.h"

void obtemVariaveisAmbiente(variaveisAmbiente* varAmb){
	if(getenv("MAXCLIENTES") == NULL)
		varAmb->MAX_CLIENTES = MAXCLIENTES_DEFAULT;
	else
		varAmb->MAX_CLIENTES = atoi(getenv("MAXCLIENTES"));

	if(getenv("MAXMEDICOS") == NULL)
		varAmb->MAX_MEDICOS = MAXMEDICOS_DEFAULT;
	else
		varAmb->MAX_MEDICOS = atoi(getenv("MAXMEDICOS"));
}