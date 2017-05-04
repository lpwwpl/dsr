/* Process model C form file: lpw_route_inf.pr.c */
/* Portions of this file copyright 1992-2003 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
const char lpw_route_inf_pr_c [] = "MIL_3_Tfile_Hdr_ 100A 30A op_runsim 7 49B0C00F 49B0C00F 1 lpw Administrator 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 8f3 1                                                                                                                                                                                                                                                                                                                                                                                                       ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>



/* Header Block */

#include "lpw_support.h"
//#include "fifo.h"
#define FROM_SRC_STRM 1
#define TO_SINK_STRM 1
#define FROM_ROUTING_LAYER_STRM 0
#define TO_ROUTING_LAYER_STRM 0

#define SRC_ARRIVAL (op_intrpt_type() == OPC_INTRPT_STRM && op_intrpt_strm() == FROM_SRC_STRM)
#define RCV_ARRIVAL (op_intrpt_type() == OPC_INTRPT_STRM && op_intrpt_strm() == FROM_ROUTING_LAYER_STRM)

int number_of_nodes;
#define FENCE 0xCCDEADCC


typedef struct {
	int source;
	int dest;
	Packet* data;
	int checksum;
	int size;
	int fencehead;
	int fencefoot;
} myudp;

/* encode and decode , now i don't work it.
int encodeAndDecode(char* source,char* dest,char* key,char type);
int is_InputKeyRight(char *inputkey);
void InitS(unsigned char *s);
void InitT(unsigned char *t,char *inputkey);
void Swap(unsigned char *s,int first,int last);
void InitPofS(unsigned char *s,unsigned char *t);
int Crypt(char *source,char *dest,char *key);
*/
void route2Application(Packet* pk_ptr);
void application2Route(Packet* pk_ptr,int lpw_dest_address);


/* End of Header Block */


#if !defined (VOSD_NO_FIN)
#undef	BIN
#undef	BOUT
#define	BIN		FIN_LOCAL_FIELD(_op_last_line_passed) = __LINE__ - _op_block_origin;
#define	BOUT	BIN
#define	BINIT	FIN_LOCAL_FIELD(_op_last_line_passed) = 0; _op_block_origin = __LINE__;
#else
#define	BINIT
#endif /* #if !defined (VOSD_NO_FIN) */



/* State variable definitions */
typedef struct
	{
	/* Internal state tracking for FSM */
	FSM_SYS_STATE
	/* State Variables */
	Distribution *	         		address_dist;
	int	                    		my_lpw_address;
	Objid	                  		my_objid;
	Objid	                  		my_node_objid;
	Objid	                  		my_network_objid;
	int	                    		TRANSMIT;
	} lpw_route_inf_state;

#define pr_state_ptr            		((lpw_route_inf_state*) (OP_SIM_CONTEXT_PTR->mod_state_ptr))
#define address_dist            		pr_state_ptr->address_dist
#define my_lpw_address          		pr_state_ptr->my_lpw_address
#define my_objid                		pr_state_ptr->my_objid
#define my_node_objid           		pr_state_ptr->my_node_objid
#define my_network_objid        		pr_state_ptr->my_network_objid
#define TRANSMIT                		pr_state_ptr->TRANSMIT

/* These macro definitions will define a local variable called	*/
/* "op_sv_ptr" in each function containing a FIN statement.	*/
/* This variable points to the state variable data structure,	*/
/* and can be used from a C debugger to display their values.	*/
#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE
#if defined (OPD_PARALLEL)
#  define FIN_PREAMBLE_DEC	lpw_route_inf_state *op_sv_ptr; OpT_Sim_Context * tcontext_ptr;
#  define FIN_PREAMBLE_CODE	\
		if (VosS_Mt_Perform_Lock) \
			VOS_THREAD_SPECIFIC_DATA_GET (VosI_Globals.simi_mt_context_data_key, tcontext_ptr, SimT_Context *); \
		else \
			tcontext_ptr = VosI_Globals.simi_sequential_context_ptr; \
		op_sv_ptr = ((lpw_route_inf_state *)(tcontext_ptr->mod_state_ptr));
#else
#  define FIN_PREAMBLE_DEC	lpw_route_inf_state *op_sv_ptr;
#  define FIN_PREAMBLE_CODE	op_sv_ptr = pr_state_ptr;
#endif


/* Function Block */


#if !defined (VOSD_NO_FIN)
enum { _op_block_origin = __LINE__ };
#endif
void destroy_packet(Packet* pk) {
	op_pk_destroy(pk);
}

void route2Application(Packet* my_udp_ptr) {

	int checksum;
	int size;
	int* ptr;
	int i;
	int j;
	int fencehead;
	int fencefoot;
	Packet* pk_ptr;
	int theChecksum;
	
	checksum = 0;
	
	ptr = (int*)op_prg_mem_alloc(4*sizeof(int));
	pk_ptr = op_pk_create_fmt("Lpw_Upper_Data");

	op_pk_nfd_get(my_udp_ptr,"checksum",&theChecksum);
	op_pk_nfd_get(my_udp_ptr,"size",&size);
	op_pk_nfd_get(my_udp_ptr,"fencehead",&fencehead);
	op_pk_nfd_get(my_udp_ptr,"fencefoot",&fencefoot);
	
//  *(ptr) = checksum;
	*(ptr + 1) = size;
	*(ptr + 2) = fencehead;
	
	printf("%d,%d\n",size,fencehead);
	
	//caculate the checksum by counting the amount of "1" in size and fencehead
	for (i = 0; i < 2; i++) {	
		for (j = 0; j < 32; j++) {		
			checksum = checksum + ((*(ptr + 1 + i) >> j) & 1);
		}
	}	
	
	//check the data 
	if(checksum != theChecksum) {
		printf("transmit data ererr.some data maybe changed\n");
	} else if(fencehead != FENCE) {
		printf("transmit data ererr.some data maybe changed\n");
	} else if(fencefoot != FENCE) {
		printf("transmit data ererr.some data maybe changed\n");
	}


	op_pk_nfd_get(my_udp_ptr,"data",pk_ptr);
	op_pk_send(pk_ptr,TO_SINK_STRM);
}

void application2Route(Packet* pk_ptr,int lpw_dest_address){

	int totalSize;
	int size;	
	int fencehead;
	int fencefoot;
	int checksum;
	int i;
	int j;
	int* start;
	Packet* pk;

	size = op_pk_total_size_get(pk_ptr);
	start  = (int*)op_prg_mem_alloc(4*sizeof(int)+size);
	
	checksum = 0;	
	fencehead = 0xCCDEADCC;	
	fencefoot = 0xCCDEADCC;		
	*(start+2) = fencehead;
	*(start+1) = size;
	printf("%d,%d\n",size,fencehead);
	
	//caculate the checksum by counting the amount of "1" in size and fencehead
	for (i = 0; i < 2; i++) {	
		for (j = 0; j < 32; j++) {		
			checksum = checksum + ((*(start + 1 + i) >> j) & 1);
		}
	}	
	
	*(start) = checksum;
	*(int *)((char *)start+size+12) = fencefoot;	

	printf("%d\n",checksum);
	
	pk=op_pk_create_fmt("APP_TO_ROUTE");
	op_pk_nfd_set(pk,"source",my_lpw_address);
	op_pk_nfd_set(pk,"dest",lpw_dest_address);
	op_pk_nfd_set(pk,"data",pk_ptr);
	op_pk_nfd_set(pk,"checksum",checksum);
	op_pk_nfd_set(pk,"size",size);
	op_pk_nfd_set(pk,"fencehead",fencehead);
	op_pk_nfd_set(pk,"fencefoot",fencefoot);
	
	op_pk_send(pk,TO_ROUTING_LAYER_STRM);
}

/* Encode and Decode 
	DSA algorithm
*/
/*
int encodeAndDecode(char* source,char* dest,char* key,char type) {
	int size=0;
	if (type=='e'||type=='E'){

		if((size=Crypt(source,dest,key)))
			return size;
		else
			return -1;
	}
	
	if (type=='d'||type=='D') {
		if((size=Crypt(source,dest,key)))
			return size;
		else
			return -1;
    }
	
	return -1;
}

int is_InputKeyRight(char *inputkey) {
 
	if(strlen(inputkey) <= 256 && strlen(inputkey) > 0)
		return 1;
	else
		return 0;
}

void InitS(unsigned char *s) {
	int i;
	for(i=0;i<256;i++)
		s[i]=i;
}

void InitT(unsigned char *t,char *inputkey) {
	int i;
	for(i=0;i<256;i++) {
		t[i]=inputkey[i%strlen(inputkey)];
	}
}

void Swap(unsigned char *s,int first,int last) {
	unsigned char temp;
	temp=s[first];
	s[first]=s[last];
	s[last]=temp;
}

void InitPofS(unsigned char *s,unsigned char *t) {
	int i;
	int j=0;
	
	for(i=0;i<256;i++) {
		j=(j+s[i]+t[i])%256;
		Swap(s,i,j);
	}
}

int Crypt(char *source,char *dest,char *key) {
	unsigned char s[256]={0};
	unsigned char t[256]={0};
	int buf;

	int i=0;
	int j=0;
	int k=0;
	int temp;
	int size;
	int counter=0;
	int last=0;

	size=strlen(source);
	InitS(s);
	InitT(t,key);
	InitPofS(s,t);
	buf=source[0];

	while(1) {
		if(buf=='\0'){break;}
		i=(i+1)%256;
		j=(j+s[i])%256;
		Swap(s,i,j);
		temp=(s[i]+s[j])%256;
		k=s[temp];
		dest[counter] = k^buf;
		counter++;
		buf=source[counter];
	}
	
	last=counter+1;
	dest[last] = '\0';

	return counter;
}
*/

/* End of Function Block */

/* Undefine optional tracing in FIN/FOUT/FRET */
/* The FSM has its own tracing code and the other */
/* functions should not have any tracing.		  */
#undef FIN_TRACING
#define FIN_TRACING

#undef FOUTRET_TRACING
#define FOUTRET_TRACING

#if defined (__cplusplus)
extern "C" {
#endif
	void lpw_route_inf (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Obtype lpw_route_inf_init (int * init_block_ptr);
	VosT_Address lpw_route_inf_alloc (VOS_THREAD_INDEX_ARG_COMMA VosT_Obtype, int);
	void lpw_route_inf_diag (OP_SIM_CONTEXT_ARG_OPT);
	void lpw_route_inf_terminate (OP_SIM_CONTEXT_ARG_OPT);
	void lpw_route_inf_svar (void *, const char *, void **);


	VosT_Fun_Status Vos_Define_Object (VosT_Obtype * obst_ptr, const char * name, unsigned int size, unsigned int init_obs, unsigned int inc_obs);
	VosT_Address Vos_Alloc_Object_MT (VOS_THREAD_INDEX_ARG_COMMA VosT_Obtype ob_hndl);
	VosT_Fun_Status Vos_Poolmem_Dealloc_MT (VOS_THREAD_INDEX_ARG_COMMA VosT_Address ob_ptr);
#if defined (__cplusplus)
} /* end of 'extern "C"' */
#endif




/* Process model interrupt handling procedure */


void
lpw_route_inf (OP_SIM_CONTEXT_ARG_OPT)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	FIN_MT (lpw_route_inf ());
	if (1)
		{
		Packet* pk_ptr;
		int destination_lpw_address;
		Ici* iciptr;
		Ici* ici_dest_ptr;
		double packet_delay;
		char* theData;
		int number_of_nodes;
		myudp* myudp_pk_ptr;


		FSM_ENTER ("lpw_route_inf")

		FSM_BLOCK_SWITCH
			{
			/*---------------------------------------------------------*/
			/** state (pre) enter executives **/
			FSM_STATE_ENTER_UNFORCED_NOLABEL (0, "pre", "lpw_route_inf [pre enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_route_inf [pre enter execs], state0_enter_exec)
				{
				op_intrpt_schedule_self(op_sim_time(),0);
				}

				FSM_PROFILE_SECTION_OUT (lpw_route_inf [pre enter execs], state0_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (1,"lpw_route_inf")


			/** state (pre) exit executives **/
			FSM_STATE_EXIT_UNFORCED (0, "pre", "lpw_route_inf [pre exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_route_inf [pre exit execs], state0_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_route_inf [pre exit execs], state0_exit_exec)


			/** state (pre) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "pre", "init")
				/*---------------------------------------------------------*/



			/** state (init) enter executives **/
			FSM_STATE_ENTER_FORCED (1, "init", state1_enter_exec, "lpw_route_inf [init enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_route_inf [init enter execs], state1_enter_exec)
				{
				my_objid = op_id_self();
				my_node_objid = op_topo_parent(my_objid);
				my_network_objid = op_topo_parent(my_node_objid);
				
				my_lpw_address = lpw_support_get_lpw_address(my_node_objid);
				number_of_nodes = lpw_support_number_of_nodes();
				
				op_ima_obj_attr_get(my_objid,"Transmit",&TRANSMIT);
				address_dist = op_dist_load("uniform_int",0,(number_of_nodes-1));
				}

				FSM_PROFILE_SECTION_OUT (lpw_route_inf [init enter execs], state1_enter_exec)

			/** state (init) exit executives **/
			FSM_STATE_EXIT_FORCED (1, "init", "lpw_route_inf [init exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_route_inf [init exit execs], state1_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_route_inf [init exit execs], state1_exit_exec)


			/** state (init) transition processing **/
			FSM_TRANSIT_FORCE (2, state2_enter_exec, ;, "default", "", "init", "idle")
				/*---------------------------------------------------------*/



			/** state (idle) enter executives **/
			FSM_STATE_ENTER_UNFORCED (2, "idle", state2_enter_exec, "lpw_route_inf [idle enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_route_inf [idle enter execs], state2_enter_exec)
				{
				}

				FSM_PROFILE_SECTION_OUT (lpw_route_inf [idle enter execs], state2_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (5,"lpw_route_inf")


			/** state (idle) exit executives **/
			FSM_STATE_EXIT_UNFORCED (2, "idle", "lpw_route_inf [idle exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_route_inf [idle exit execs], state2_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_route_inf [idle exit execs], state2_exit_exec)


			/** state (idle) transition processing **/
			FSM_PROFILE_SECTION_IN (lpw_route_inf [idle trans conditions], state2_trans_conds)
			FSM_INIT_COND (SRC_ARRIVAL)
			FSM_TEST_COND (RCV_ARRIVAL)
			FSM_DFLT_COND
			FSM_TEST_LOGIC ("idle")
			FSM_PROFILE_SECTION_OUT (lpw_route_inf [idle trans conditions], state2_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 3, state3_enter_exec, ;, "SRC_ARRIVAL", "", "idle", "tx")
				FSM_CASE_TRANSIT (1, 4, state4_enter_exec, ;, "RCV_ARRIVAL", "", "idle", "rx")
				FSM_CASE_TRANSIT (2, 2, state2_enter_exec, ;, "default", "", "idle", "idle")
				}
				/*---------------------------------------------------------*/



			/** state (tx) enter executives **/
			FSM_STATE_ENTER_FORCED (3, "tx", state3_enter_exec, "lpw_route_inf [tx enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_route_inf [tx enter execs], state3_enter_exec)
				{
				
				pk_ptr = op_pk_get(FROM_SRC_STRM);
				
				if(TRANSMIT) {
				 
				//	ici_dest_ptr = op_intrpt_ici();
				//	op_ici_attr_get(ici_dest_ptr,"Packet_Destination",&destination_lpw_address);
					
					// the destination address of data packet is random(not including self node address)
				    // destination address maybe design in the application layer,not in transport layer's concern
					destination_lpw_address = (int)op_dist_outcome(address_dist);
					while(destination_lpw_address == my_lpw_address) {
						destination_lpw_address = (int)op_dist_outcome(address_dist);
					}
					iciptr = op_ici_create("Lpw_Dest_Ici");
					op_ici_attr_set(iciptr,"Packet_Destination",destination_lpw_address);
					op_ici_install(iciptr);
					
					application2Route(pk_ptr,destination_lpw_address);
				} else {
					op_pk_destroy(pk_ptr);
				}
				
				
				}

				FSM_PROFILE_SECTION_OUT (lpw_route_inf [tx enter execs], state3_enter_exec)

			/** state (tx) exit executives **/
			FSM_STATE_EXIT_FORCED (3, "tx", "lpw_route_inf [tx exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_route_inf [tx exit execs], state3_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_route_inf [tx exit execs], state3_exit_exec)


			/** state (tx) transition processing **/
			FSM_TRANSIT_FORCE (2, state2_enter_exec, ;, "default", "", "tx", "idle")
				/*---------------------------------------------------------*/



			/** state (rx) enter executives **/
			FSM_STATE_ENTER_FORCED (4, "rx", state4_enter_exec, "lpw_route_inf [rx enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_route_inf [rx enter execs], state4_enter_exec)
				{
				myudp_pk_ptr = op_pk_get(FROM_ROUTING_LAYER_STRM);
				printf("$$$$$$$$$$$$$$$\n");
				//packet_delay = (op_sim_time() - op_pk_creation_time_get(pk_ptr));
				//op_pk_send(pk_ptr,TO_SINK_STRM);
				route2Application(myudp_pk_ptr);
				}

				FSM_PROFILE_SECTION_OUT (lpw_route_inf [rx enter execs], state4_enter_exec)

			/** state (rx) exit executives **/
			FSM_STATE_EXIT_FORCED (4, "rx", "lpw_route_inf [rx exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_route_inf [rx exit execs], state4_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_route_inf [rx exit execs], state4_exit_exec)


			/** state (rx) transition processing **/
			FSM_TRANSIT_FORCE (2, state2_enter_exec, ;, "default", "", "rx", "idle")
				/*---------------------------------------------------------*/



			}


		FSM_EXIT (0,"lpw_route_inf")
		}
	}




void
lpw_route_inf_diag (OP_SIM_CONTEXT_ARG_OPT)
	{
	/* No Diagnostic Block */
	}




void
lpw_route_inf_terminate (OP_SIM_CONTEXT_ARG_OPT)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = __LINE__;
#endif

	FIN_MT (lpw_route_inf_terminate ())

	Vos_Poolmem_Dealloc_MT (OP_SIM_CONTEXT_THREAD_INDEX_COMMA pr_state_ptr);

	FOUT
	}


/* Undefine shortcuts to state variables to avoid */
/* syntax error in direct access to fields of */
/* local variable prs_ptr in lpw_route_inf_svar function. */
#undef address_dist
#undef my_lpw_address
#undef my_objid
#undef my_node_objid
#undef my_network_objid
#undef TRANSMIT

#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE

#define FIN_PREAMBLE_DEC
#define FIN_PREAMBLE_CODE

VosT_Obtype
lpw_route_inf_init (int * init_block_ptr)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	VosT_Obtype obtype = OPC_NIL;
	FIN_MT (lpw_route_inf_init (init_block_ptr))

	Vos_Define_Object (&obtype, "proc state vars (lpw_route_inf)",
		sizeof (lpw_route_inf_state), 0, 20);
	*init_block_ptr = 0;

	FRET (obtype)
	}

VosT_Address
lpw_route_inf_alloc (VOS_THREAD_INDEX_ARG_COMMA VosT_Obtype obtype, int init_block)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	lpw_route_inf_state * ptr;
	FIN_MT (lpw_route_inf_alloc (obtype))

	ptr = (lpw_route_inf_state *)Vos_Alloc_Object_MT (VOS_THREAD_INDEX_COMMA obtype);
	if (ptr != OPC_NIL)
		ptr->_op_current_block = init_block;
	FRET ((VosT_Address)ptr)
	}



void
lpw_route_inf_svar (void * gen_ptr, const char * var_name, void ** var_p_ptr)
	{
	lpw_route_inf_state		*prs_ptr;

	FIN_MT (lpw_route_inf_svar (gen_ptr, var_name, var_p_ptr))

	if (var_name == OPC_NIL)
		{
		*var_p_ptr = (void *)OPC_NIL;
		FOUT
		}
	prs_ptr = (lpw_route_inf_state *)gen_ptr;

	if (strcmp ("address_dist" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->address_dist);
		FOUT
		}
	if (strcmp ("my_lpw_address" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_lpw_address);
		FOUT
		}
	if (strcmp ("my_objid" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_objid);
		FOUT
		}
	if (strcmp ("my_node_objid" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_node_objid);
		FOUT
		}
	if (strcmp ("my_network_objid" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_network_objid);
		FOUT
		}
	if (strcmp ("TRANSMIT" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->TRANSMIT);
		FOUT
		}
	*var_p_ptr = (void *)OPC_NIL;

	FOUT
	}

