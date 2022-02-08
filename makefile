all: balcao cliente medico

balcao: balcao.o var_amb.o
		gcc -g balcao.o var_amb.o -o balcao -lpthread

cliente: cliente.o var_amb.o
		gcc cliente.c -o cliente -lpthread

medico: medico.o var_amb.o
		gcc medico.c -o medico -lpthread

clean:
	rm	-rf	*o cliente
