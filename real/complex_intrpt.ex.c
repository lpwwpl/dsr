#include "complex_intrpt.h"
sComplexIntrpt* complex_intrpt_new() {
	sComplexIntrpt* complex_intrpt;
	complex_intrpt=(sComplexIntrpt*)op_prg_mem_alloc(sizeof(sComplexIntrpt));
	
	if (complex_intrpt!=OPC_NIL) {
		if (!complex_intrpt_init(complex_intrpt)) {
			op_prg_mem_free(complex_intrpt);
			complex_intrpt=OPC_NIL;
		}
	}
	
	return(complex_intrpt);
}

int complex_intrpt_init(sComplexIntrpt* complex_intrpt) {
	complex_intrpt->intrpt_id_number=0;
	complex_intrpt->intrpt_data_fifo=fifo_new();
	return (complex_intrpt->intrpt_data_fifo!=OPC_NIL);
}

Evhandle complex_intrpt_schedule_self(sComplexIntrpt * complex_intrpt, double time, int intrpt_code, void* intrpt_data) {
	Evhandle evt;
	sComplexIntrptObject* complex_intrpt_object;
	complex_intrpt_object=(sComplexIntrptObject*)op_prg_mem_alloc(sizeof(sComplexIntrptObject));
	complex_intrpt_object->intrpt_code=intrpt_code;
	complex_intrpt_object->intrpt_data=intrpt_data;
	evt=op_intrpt_schedule_self(time,complex_intrpt->intrpt_id_number);
	fifo_multiplex_insert(complex_intrpt->intrpt_data_fifo,complex_intrpt_object,complex_intrpt->intrpt_id_number);
	complex_intrpt->intrpt_id_number++;
	return(evt);
}

Evhandle complex_intrpt_schedule_remote(sComplexIntrpt * complex_intrpt, double time, int intrpt_code, void* intrpt_data, Objid process_objid) {
	Evhandle evt;
	sComplexIntrptObject* complex_intrpt_object;
	complex_intrpt_object=(sComplexIntrptObject*)op_prg_mem_alloc(sizeof(sComplexIntrptObject));
	complex_intrpt_object->intrpt_code=intrpt_code;
	complex_intrpt_object->intrpt_data=intrpt_data;
	evt=op_intrpt_schedule_remote(time,complex_intrpt->intrpt_id_number,process_objid);
	fifo_multiplex_insert(complex_intrpt->intrpt_data_fifo,complex_intrpt_object,complex_intrpt->intrpt_id_number);
	complex_intrpt->intrpt_id_number++;
	return(evt);
}

int complex_intrpt_code(sComplexIntrpt * complex_intrpt) {
	int intrpt_id;
	sComplexIntrptObject* complex_intrpt_object;
	intrpt_id=op_intrpt_code();
	complex_intrpt_object=fifo_multiplex_read(complex_intrpt->intrpt_data_fifo,intrpt_id);
	
	if (complex_intrpt_object!=OPC_NIL) {
		return(complex_intrpt_object->intrpt_code);
	} else {
		printf("Complex intrpt get code: I cannot find the actual intrpt in the fifo","","","");
		return (-1);
	}
}

void* complex_intrpt_data(sComplexIntrpt * complex_intrpt) {
	int intrpt_id;
	sComplexIntrptObject* complex_intrpt_object;
	void* data;
	intrpt_id=op_intrpt_code();
	complex_intrpt_object=fifo_multiplex_read(complex_intrpt->intrpt_data_fifo,intrpt_id);
	
	if (complex_intrpt_object!=OPC_NIL)	{
		data=complex_intrpt_object->intrpt_data;
		complex_intrpt_object->intrpt_data=OPC_NIL;
		return data;
	} else {
		printf("Complex intrpt get data: I cannot find the actual intrpt in the fifo","","","");
		return (OPC_NIL);
	}
}

int complex_intrpt_cancel(sComplexIntrpt * complex_intrpt, Evhandle evt) {
	int intrpt_id;
	sComplexIntrptObject* complex_intrpt_object;
	intrpt_id=op_ev_code(evt);
	complex_intrpt_object=fifo_multiplex_extract(complex_intrpt->intrpt_data_fifo,intrpt_id);
	
	if (complex_intrpt_object!=OPC_NIL)	{
		if(complex_intrpt_object->intrpt_data!=OPC_NIL);
		{
			op_prg_mem_free(complex_intrpt_object->intrpt_data);
		}
		
		free(complex_intrpt_object);
		
		if (op_ev_valid(evt)) {
			if (op_ev_pending(evt) == OPC_FALSE) {
				printf("The complex interruption is not active: may be it already happened\n");
				return(0);
			} else if (op_ev_cancel(evt)==OPC_COMPCODE_SUCCESS)
			{
				printf("The complex interruption event is successfully cancelled\n");
				return(1);
			}
		} else {
			printf("The complex interruption is invalid\n");
			return(0);
		}
	} else {
		op_sim_end("Complex intrpt cancel: I cannot find the wanted intrpt event in the fifo","","","");
		return(0);
	}
}


void complex_intrpt_close(sComplexIntrpt * complex_intrpt) {
	int intrpt_id;
	sComplexIntrptObject* complex_intrpt_object;
	intrpt_id=op_intrpt_code();
	complex_intrpt_object=fifo_multiplex_extract(complex_intrpt->intrpt_data_fifo,intrpt_id);
	if (complex_intrpt_object!=OPC_NIL) {
		if(complex_intrpt_object->intrpt_data!=OPC_NIL);
		{
			op_prg_mem_free(complex_intrpt_object->intrpt_data);
		}
		free(complex_intrpt_object);
	} else 	{
		op_sim_end("Complex intrpt close: I cannot find the actual intrpt in the fifo","","","");
	}
}


void complex_intrpt_destroy(sComplexIntrpt* complex_intrpt) {
	fifo_destroy(complex_intrpt->intrpt_data_fifo);
	op_prg_mem_free(complex_intrpt)
;
}
