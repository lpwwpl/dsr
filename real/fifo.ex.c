#include "fifo.h"
//sFifo* ptr_fifo_test_addresses=OPC_NIL; // fifo structure used for checking and validation purpose
//sFifo* ptr_fifo_addresses=OPC_NIL;		// fifo structure used to find the mac_address or the lpw_address of a node from its objid
//Objid* nodes_objid_table;				// table used to find the objid of a node from its lpw_address
//int validated=0;						// boolean indicating if every address has been validated or not 


void fifo_init(sFifo *fifo) {
	fifo->nbrObjects=0;
	fifo->firstObject=OPC_NIL;
	fifo->lastObject=OPC_NIL;
}

sFifo* fifo_new() {
	sFifo* fifo;
	fifo=(sFifo*)op_prg_mem_alloc(sizeof(sFifo));
	fifo_init(fifo);
	return fifo;
}

int fifo_insert(sFifo* fifo, void* data) {
	sObject* object;
	object=(sObject*)op_prg_mem_alloc(sizeof(sObject));
	if (object!=OPC_NIL) {
		object->data=data;
		object->fifoNumber=0;
		object->next=OPC_NIL;
		if (fifo->nbrObjects<=0) {
			fifo->firstObject=object;
			fifo->lastObject=object;
		} else {
			fifo->lastObject->next=object;
			fifo->lastObject=object;
		}
		fifo->nbrObjects++;
		return 1;
	} else {
		return 0;
	}
}
	
void* fifo_extract(sFifo *fifo) {
	sObject* object;
	void *data;
	
	if (fifo->nbrObjects!=0) {
		object=fifo->firstObject;
		data=object->data;
		fifo->firstObject=object->next;
		op_prg_mem_free(object);
		fifo->nbrObjects--;
		return data;
	} else {
		return OPC_NIL;
	}
}

void* fifo_read(sFifo *fifo) {
	sObject* object;
	void *data;
	if (fifo->nbrObjects!=0) {
		object=fifo->firstObject;
		data=object->data;
		return data;
	} else {
		return OPC_NIL;
	}
}

int fifo_multiplex_insert(sFifo* fifo,void* data,int fifo_number) {
	int result;
	result=fifo_insert(fifo,data);
	
	if (result!=0) {
		fifo->lastObject->fifoNumber=fifo_number;
	}
	return result;
}

void* fifo_multiplex_extract(sFifo* fifo,int fifo_number) {
	sObject* object;
	sObject* previousObject;
	void *data;
	previousObject=OPC_NIL;
	object=fifo->firstObject;
	
	while (object!=OPC_NIL) {
	 	if (object->fifoNumber==fifo_number) {
			data=object->data;
			if (previousObject==OPC_NIL) {
				fifo->firstObject=object->next;
			} else {
				previousObject->next=object->next;
				if (object==fifo->lastObject) {
					fifo->lastObject=previousObject;
				}
			}
			
			op_prg_mem_free(object);
			fifo->nbrObjects--;
			return data;
		} else {
			previousObject=object;
			object=object->next;
		}
	}
	
	return OPC_NIL;
}

void* fifo_multiplex_read(sFifo* fifo,int fifo_number) {
	sObject* object;
	sObject* previousObject;
	void *data;
	previousObject=OPC_NIL;
	object=fifo->firstObject;
	
	while (object!=OPC_NIL) {
	 	if (object->fifoNumber==fifo_number) {
			data=object->data;
			return data;
		} else {
			previousObject=object;
			object=object->next;
		}
	}
	return OPC_NIL;
}

void* fifo_multiplex_extract_first(sFifo *fifo, int *fifo_number) {
	sObject* object;
	void* data;
	
	if (fifo->nbrObjects!=0) {
		object=fifo->firstObject;
		data=object->data;
		*fifo_number=object->fifoNumber;
		fifo->firstObject=object->next;
		op_prg_mem_free(object);
		fifo->nbrObjects--;
		return data;
	} else {
		return OPC_NIL;
	}
}

void* fifo_multiplex_read_first(sFifo *fifo, int *fifo_number) {
	sObject* object;
	void* data;
	
	if (fifo->nbrObjects!=0) {
		object=fifo->firstObject;
		data=object->data;
		*fifo_number=object->fifoNumber;
		return data;
	} else {
		return OPC_NIL;
	}
}

void fifo_destroy(sFifo* fifo) {
	void *data;
	
	do {
		data=fifo_extract(fifo);
		op_prg_mem_free(data);
	} while (data!=OPC_NIL);
	
	op_prg_mem_free(fifo);
}

void fifo_print(sFifo fifo) {
	int* i;
	int nbr;
	sObject* object;
	object=fifo.firstObject;
	nbr=0;

	while(object!=OPC_NIL) {
		i=(int*)(object->data);
		printf("%d...",*i);
		object=object->next;
		nbr++;
	}
	
	printf("end of the %d objects\n",fifo.nbrObjects);
}

