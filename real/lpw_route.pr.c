/* Process model C form file: lpw_route.pr.c */
/* Portions of this file copyright 1992-2003 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
const char lpw_route_pr_c [] = "MIL_3_Tfile_Hdr_ 100A 30A op_runsim 7 49B0CFE4 49B0CFE4 1 lpw Administrator 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 8f3 1                                                                                                                                                                                                                                                                                                                                                                                                       ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>



/* Header Block */

#include <math.h>
#include <opnet.h>
#include <stdlib.h>
#include "fifo.h"
#include "lpw_support.h"
#include "complex_intrpt.h"

#define FROM_UPPER_LAYER_STRM 1
#define FROM_MAC_LAYER_STRM 0
#define TO_MAC_LAYER_STRM 0
#define TO_UPPER_LAYER_STRM 1

#define DATA_PACKET_TYPE 5
#define REQUEST_PACKET_TYPE 7
#define REPLY_PACKET_TYPE 11 
#define ERROR_PACKET_TYPE 13

#define ACK_CODE 1
#define REPLY_CODE 5
#define ERROR_CODE 2
#define BROADCAST -1

#define OPTION 10000
#define MAX_SIZE_ROUTE 8

#define UPPER_LAYER_ARRIVAL (op_intrpt_type() == OPC_INTRPT_STRM && op_intrpt_strm() == FROM_UPPER_LAYER_STRM)
#define MAC_LAYER_ARRIVAL (op_intrpt_type() == OPC_INTRPT_STRM && op_intrpt_strm() == FROM_MAC_LAYER_STRM)
#define ACK (op_intrpt_type() == OPC_INTRPT_SELF && complex_intrpt_code(my_complex_intrpt_ptr)== ACK_CODE)
#define REPLY (op_intrpt_type() == OPC_INTRPT_SELF && complex_intrpt_code(my_complex_intrpt_ptr) == REPLY_CODE)

#define DATA_LIFE_TIME 5

double WAIT_REPLY_MIN_TIME;		
double WAIT_REPLY_MAX_TIME;	
double REQUEST_LIFE_TIME;
double HOP_DELAY;
double WAIT_NON_PROPA_REPLY_TIME;

typedef struct
	{
	int route[MAX_SIZE_ROUTE];	// the route = array of integer
	int size_route;				// the size of the route
	} sRoute;

typedef struct
	{
	int sequence_number;	// the sequence number of the request
	double scheduling_time; // the time when a new request should be sent
	double waiting_time;	// the total time we have to wait before scheduling_time
	Evhandle evt;
	} sRequestSent;

typedef struct
	{
	int sequence_number; 	// the sequence number of the reply
	Packet* pk;				// the reply_packet
	Evhandle evt;
	} sReply;

double WAIT_ACK_TIME;
int NUMBER_OF_NODES;
double TRANSMISSION_RANGE;
int packet_id = 0;

//void lpw_no_loop_in_route(sRoute* route);
void lpw_tables_init();
void lpw_pre_init() ;
void lpw_user_parameter_init();
void lp_route_init(sRoute* cache,int n);

int lpw_in_transmission_range(Packet* pk_ptr);
void lpw_transmit_request(int destination_lpw_address);
void lpw_handle_request(Packet * pk_ptr);
int lpw_request_already_seen(int source_lpw_address,int destination_lpw_address,int sequence_number);
void lpw_forward_request(Packet* pk_ptr);
void lpw_transmit_reply_from_target(Packet* pk_ptr);
//void lpw_transmit_reply_from_relay(Packet *pk_ptr);
int lpw_reply_already_seen(int source_lpw_address,int destination_lpw_address,int sequence_number);
void lpw_forward_reply(Packet* pk_ptr);
void lpw_forward_data(Packet* pk_ptr);
void lpw_send_to_mac(Packet* pk_ptr, int destination_mac_address);

void  lpw_handle_reply(Packet* pk_ptr);
void lpw_insert_route_in_cache(Packet* pk_ptr);
void lpw_transmit_data(Packet* pk_ptr, int destination_lpw_address);
void lpw_handle_data(Packet* pk_ptr);
void lpw_upper_layer_data_arrival (Packet* data_pk_ptr, int destination_lpw_address);
void lpw_insert_buffer(Packet* pk_ptr, int dest_lpw_address);
int lpw_buffer_empty(int dest_lpw_address);
Packet* lpw_extract_buffer(int dest_lpw_address);
//void lpw_schedule_no_ack_event(Packet* pk_ptr);

Stathandle stat_mean_hops_by_route;

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
	Objid	                  		my_objid;
	Objid	                  		my_node_objid;
	Objid	                  		my_network_objid;
	int	                    		my_mac_address;
	int	                    		my_lpw_address;
	char *	                 		my_name;
	sRoute *	               		route_cache;
	sRequestSent *	         		request_sent;
	int	                    		request_sequence_number;
	int**	                  		request_seen;
	int**	                  		reply_seen;
	sComplexIntrpt *	       		my_complex_intrpt_ptr;
	sFifo	                  		received_packet_id_fifo;
	sFifo	                  		reply_fifo;
	} lpw_route_state;

#define pr_state_ptr            		((lpw_route_state*) (OP_SIM_CONTEXT_PTR->mod_state_ptr))
#define my_objid                		pr_state_ptr->my_objid
#define my_node_objid           		pr_state_ptr->my_node_objid
#define my_network_objid        		pr_state_ptr->my_network_objid
#define my_mac_address          		pr_state_ptr->my_mac_address
#define my_lpw_address          		pr_state_ptr->my_lpw_address
#define my_name                 		pr_state_ptr->my_name
#define route_cache             		pr_state_ptr->route_cache
#define request_sent            		pr_state_ptr->request_sent
#define request_sequence_number 		pr_state_ptr->request_sequence_number
#define request_seen            		pr_state_ptr->request_seen
#define reply_seen              		pr_state_ptr->reply_seen
#define my_complex_intrpt_ptr   		pr_state_ptr->my_complex_intrpt_ptr
#define received_packet_id_fifo 		pr_state_ptr->received_packet_id_fifo
#define reply_fifo              		pr_state_ptr->reply_fifo

/* These macro definitions will define a local variable called	*/
/* "op_sv_ptr" in each function containing a FIN statement.	*/
/* This variable points to the state variable data structure,	*/
/* and can be used from a C debugger to display their values.	*/
#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE
#if defined (OPD_PARALLEL)
#  define FIN_PREAMBLE_DEC	lpw_route_state *op_sv_ptr; OpT_Sim_Context * tcontext_ptr;
#  define FIN_PREAMBLE_CODE	\
		if (VosS_Mt_Perform_Lock) \
			VOS_THREAD_SPECIFIC_DATA_GET (VosI_Globals.simi_mt_context_data_key, tcontext_ptr, SimT_Context *); \
		else \
			tcontext_ptr = VosI_Globals.simi_sequential_context_ptr; \
		op_sv_ptr = ((lpw_route_state *)(tcontext_ptr->mod_state_ptr));
#else
#  define FIN_PREAMBLE_DEC	lpw_route_state *op_sv_ptr;
#  define FIN_PREAMBLE_CODE	op_sv_ptr = pr_state_ptr;
#endif


/* Function Block */


#if !defined (VOSD_NO_FIN)
enum { _op_block_origin = __LINE__ };
#endif
/*
initial the network,count the number of nodes and assign the mac address and route address to nodes.
*/
void lpw_pre_init() {
	char address_field[100];

	// init the objid of the current dsr process
	my_objid = op_id_self();
	my_node_objid=op_topo_parent(my_objid);
	my_network_objid=op_topo_parent(my_node_objid);
	my_name=(char*)op_prg_mem_alloc(25*sizeof(char));
	op_ima_obj_attr_get(my_node_objid,"name",my_name);

	op_ima_obj_attr_get(my_objid,"Mac Address Field",address_field);
	if  (strcmp(address_field,"name") != 0) {
		op_ima_obj_attr_get(my_node_objid,address_field,&my_mac_address);
	} else {
		op_ima_obj_attr_get(my_node_objid,"name",address_field);
		my_mac_address = atoi(address_field);
	}
	op_ima_obj_attr_get(my_objid,"Lpw Address",&my_lpw_address);
	if (my_lpw_address == LPW_SUPPORT_USE_THE_MAC_ADDRESS) {	
		my_lpw_address = lpw_support_declare_node_addresses(my_node_objid,my_mac_address,my_mac_address);
		printf("%d,%d\n",my_lpw_address,my_mac_address);
	} else if (my_lpw_address==LPW_SUPPORT_AUTOMATIC_ASSIGNATION) {
		my_lpw_address=lpw_support_declare_node_addresses(my_node_objid,LPW_SUPPORT_AUTOMATIC_ASSIGNATION,my_mac_address);
	}
}

/*
initial the parameters
*/
void lpw_user_parameter_init() {
	my_objid=op_id_self();
	my_node_objid=op_topo_parent(my_objid);
	my_network_objid=op_topo_parent(my_node_objid);
	op_ima_obj_attr_get(my_objid,"Lpw Wait Reply Min Time",&WAIT_REPLY_MIN_TIME);
	op_ima_obj_attr_get(my_objid,"Lpw Wait Reply Max Time",&WAIT_REPLY_MAX_TIME);
	op_ima_sim_attr_get(OPC_IMA_DOUBLE,"Lpw Transmission Range",&TRANSMISSION_RANGE);	
	op_ima_obj_attr_get(my_objid,"Lpw Request Life Time",&REQUEST_LIFE_TIME);
	op_ima_obj_attr_get(my_objid,"Lpw Wait Ack Time",&WAIT_ACK_TIME);
	WAIT_NON_PROPA_REPLY_TIME=WAIT_REPLY_MIN_TIME/MAX_SIZE_ROUTE;
	HOP_DELAY=WAIT_NON_PROPA_REPLY_TIME/(MAX_SIZE_ROUTE*1.5);
}

/*
initial the route cache table
*/
void lp_route_init(sRoute* cache, int n) {
	int i;
	for(i=0;i<MAX_SIZE_ROUTE;i++) {
		cache[n].route[i]=-1;
	}
	cache[n].size_route=0;
}

/*
 * initial the tables of reply and request.
 */
void lpw_tables_init() {
	int i,j;

	NUMBER_OF_NODES=lpw_support_number_of_nodes();

	route_cache=(sRoute*)op_prg_mem_alloc(NUMBER_OF_NODES*sizeof(sRoute));
	for (i=0;i<NUMBER_OF_NODES;i++)	{
		lp_route_init(route_cache,i);
	}

	request_sent=(sRequestSent*)op_prg_mem_alloc(NUMBER_OF_NODES*sizeof(sRequestSent));
	
	for (i=0;i<NUMBER_OF_NODES;i++)	{
		request_sent[i].sequence_number=0;
		request_sent[i].scheduling_time=0;
		request_sent[i].waiting_time=0;
	}
	
	request_sequence_number=0;
	request_seen=(int**)op_prg_mem_alloc(NUMBER_OF_NODES*sizeof(int*));
	
	for (i=0;i<NUMBER_OF_NODES;i++)	{
		request_seen[i]=(int*)op_prg_mem_alloc(NUMBER_OF_NODES*sizeof(int));; 
		for (j=0;j<NUMBER_OF_NODES;j++) {
			request_seen[i][j]=-1;	
		}
	}
	
	reply_seen=(int**)op_prg_mem_alloc(NUMBER_OF_NODES*sizeof(int*));
	
	for (i=0;i<NUMBER_OF_NODES;i++)	{
		reply_seen[i]=(int*)op_prg_mem_alloc(NUMBER_OF_NODES*sizeof(int));; 
		for (j=0;j<NUMBER_OF_NODES;j++) {
			reply_seen[i][j]=-1;	
		}
	}
	
	my_complex_intrpt_ptr=complex_intrpt_new();
	fifo_init(&reply_fifo);
	fifo_init(&received_packet_id_fifo);
}

/*
  check if the packet is whitin the transmission range of the sending node
*/
int lpw_in_transmission_range(Packet* pk_ptr) {
	Objid tr_source;
	double x,y,z;
	double latitude,longitude,altitude;
	double x_source,y_source,z_source;
	double distance;
	Compcode comp_code;

	op_pk_nfd_get(pk_ptr,"TR_source",&tr_source);

	comp_code=op_ima_node_pos_get (my_node_objid,&latitude,&longitude, &altitude, &x, &y, &z);
	comp_code=op_ima_node_pos_get (tr_source,&latitude,&longitude, &altitude, &x_source, &y_source, &z_source);
	distance= sqrt((x-x_source)*(x-x_source) + (y-y_source)*(y-y_source) + (z-z_source)*(z-z_source));

	if (distance <= 0.0) {
		return(0);
	} else if (distance <= TRANSMISSION_RANGE) {
		return(1);
	} else {
		return(0);
	}
}

/*
transmit the request for the destination_lpw_address
*/
void lpw_transmit_request(int destination_lpw_address) {
	Packet* pk_ptr;
	double timer;
	double random;
	Distribution* reply_waiting_dist;
	int *destination_lpw_address_ptr;
	Ici* ici_ack;
	
	pk_ptr = op_pk_create_fmt("Lpw_Request");
	op_pk_nfd_set(pk_ptr,"Type",REQUEST_PACKET_TYPE);

	op_pk_nfd_set(pk_ptr,"SRC",my_lpw_address);
	op_pk_nfd_set(pk_ptr,"DEST",destination_lpw_address);
	op_pk_nfd_set(pk_ptr,"Node_0",my_lpw_address);
	op_pk_nfd_set(pk_ptr,"Size_Route",1);
	op_pk_nfd_set(pk_ptr,"Creation_Time",op_sim_time());

	timer=request_sent[destination_lpw_address].waiting_time;

	printf("Transmit Request\n");
	//&&(NON_PROPA_REQUEST_MECHANISM)
	if ((timer == 0)) {
		op_pk_nfd_set(pk_ptr,"Seg_Left",0);
		timer = WAIT_NON_PROPA_REPLY_TIME;
		
	} else {
		op_pk_nfd_set(pk_ptr,"Seg_Left",MAX_SIZE_ROUTE-2);
		if((timer==WAIT_NON_PROPA_REPLY_TIME)||(timer==0)) {
			timer = WAIT_REPLY_MIN_TIME;
		} else {
			if (timer != WAIT_REPLY_MAX_TIME) {
				reply_waiting_dist=op_dist_load("uniform",0.0,1.0);
				random=op_dist_outcome(reply_waiting_dist);
				timer=timer+random*timer;
				if (timer > WAIT_REPLY_MAX_TIME) timer=WAIT_REPLY_MAX_TIME;
			}
		}	
	}
	
	op_pk_nfd_set(pk_ptr,"Seq_number",request_sequence_number++);
	request_sent[destination_lpw_address].sequence_number=request_sequence_number-1;	
	request_sent[destination_lpw_address].waiting_time = timer;
	op_pk_nfd_set(pk_ptr,"TR_source",my_node_objid);
	destination_lpw_address_ptr=(int*)op_prg_mem_alloc(sizeof(int));
	*destination_lpw_address_ptr = destination_lpw_address;
	request_sent[destination_lpw_address].scheduling_time = op_sim_time() + timer;
	request_sent[destination_lpw_address].evt = complex_intrpt_schedule_self(my_complex_intrpt_ptr,op_sim_time()+timer,ACK_CODE,destination_lpw_address_ptr);
   
	lpw_request_already_seen(my_lpw_address,destination_lpw_address,request_sent[destination_lpw_address].sequence_number);
	lpw_send_to_mac(pk_ptr,BROADCAST);
}

/*
 handle the request packet
*/
void lpw_handle_request(Packet * pk_ptr) {

	double creation_time;
	int size_route;
	int seg_left;
	int source_lpw_address;
	int destination_lpw_address;
	int sequence_number;
	
	op_pk_nfd_get (pk_ptr,"SRC",&source_lpw_address);
	op_pk_nfd_get (pk_ptr,"DEST",&destination_lpw_address); 
	op_pk_nfd_get (pk_ptr,"Seq_number",&sequence_number);
	op_pk_nfd_get(pk_ptr,"Creation_Time",&creation_time); 
	op_pk_nfd_get (pk_ptr,"Size_Route",&size_route); 
	op_pk_nfd_get (pk_ptr,"Seg_Left",&seg_left);

	printf("handle Request\n");
	
	if (!lpw_request_already_seen(source_lpw_address,destination_lpw_address,sequence_number)) {	
	
		// if the current node is the destination of the request
		if (my_lpw_address == destination_lpw_address) {		
		
			lpw_transmit_reply_from_target(pk_ptr);
		} else {
		
//			if  (route_cache[destination_lpw_address].size_route>0) {
			
//				lpw_transmit_reply_from_relay(pk_ptr);
//			} else 
			if ((creation_time + REQUEST_LIFE_TIME >	op_sim_time()) && (seg_left>0)) {	
			
				//forward the request.
				lpw_forward_request(pk_ptr);
			} else {	
				op_pk_destroy(pk_ptr);
            }
		}
	} else {	
		op_pk_destroy(pk_ptr);
	}
}

/*
 check whether the request is already in the cathe
*/
int lpw_request_already_seen(int source_lpw_address,int destination_lpw_address,int sequence_number) {

	if (request_seen[source_lpw_address][destination_lpw_address] >= sequence_number) {
		return(1);
    } else {	
		request_seen[source_lpw_address][destination_lpw_address] = sequence_number;
		return (0);
	}
}

/*
  forward request packet.
*/
void lpw_forward_request(Packet* pk_ptr) {
	int size_route;
	int seg_left;
	char field[10];

	op_pk_nfd_get (pk_ptr,"Size_Route",&size_route); 
	op_pk_nfd_get (pk_ptr,"Seg_Left",&seg_left);

	sprintf(field,"Node_%d",size_route);
	op_pk_nfd_set (pk_ptr,field,my_lpw_address);
	
	// decrement by one the Seg_left field of the packet
	op_pk_nfd_set(pk_ptr,"Seg_Left",--seg_left);
	
	// increment by one the Size_Route field of the packet
	op_pk_nfd_set(pk_ptr,"Size_Route",++size_route);
	
	op_pk_nfd_set(pk_ptr,"TR_source",my_node_objid);

	printf("forward request\n");
	lpw_send_to_mac(pk_ptr,BROADCAST);
}

/*
 the node's address is the destination address of request,generate the reply packet from the destination address
*/
void lpw_transmit_reply_from_target(Packet* pk_ptr) {
	Packet* reply_pk_ptr;
	int request_source_lpw_address;
	int node_lpw_address;
	int relay_lpw_address;
	int relay_mac_address;
	char field[10];
	int seg_left,size_route;
	int sequence_number;
	int i;

	printf("Transmit reply from target\n");
	
	reply_pk_ptr = op_pk_create_fmt("Lpw_Reply");
	op_pk_nfd_set(pk_ptr,"Type",REPLY_PACKET_TYPE);

	op_pk_nfd_set(reply_pk_ptr,"SRC",my_lpw_address);
	op_pk_nfd_get(pk_ptr,"SRC",&request_source_lpw_address);
	op_pk_nfd_set(reply_pk_ptr,"Type",REPLY_PACKET_TYPE);
	
	op_pk_nfd_set(reply_pk_ptr,"DEST",request_source_lpw_address);
	op_pk_nfd_set(reply_pk_ptr,"Reply_From_Target",1);

	op_pk_nfd_get(pk_ptr,"Seq_number",&sequence_number);
	op_pk_nfd_set(reply_pk_ptr,"Seq_number",sequence_number);

	op_pk_nfd_get(pk_ptr,"Size_Route",&size_route);
	
	for (i=0;i<size_route;i++) {
	
		sprintf(field,"Node_%d",i);
		op_pk_nfd_get(pk_ptr,field,&node_lpw_address);
		op_pk_nfd_set(reply_pk_ptr,field,node_lpw_address);
	}

	sprintf(field,"Node_%d",i);
	op_pk_nfd_set(reply_pk_ptr,field,my_lpw_address);
	size_route++;

	sprintf(field,"Node_%d",size_route-2);
	op_pk_nfd_get(reply_pk_ptr,field,&relay_lpw_address);
	op_pk_nfd_set(reply_pk_ptr,"RELAY",relay_lpw_address);

	seg_left=size_route-2;
	op_pk_nfd_set(reply_pk_ptr,"Seg_Left",seg_left);
	op_pk_nfd_set(reply_pk_ptr,"Size_Route",size_route);
	op_pk_nfd_set(reply_pk_ptr,"TR_source",my_node_objid);

	lpw_reply_already_seen(my_lpw_address,request_source_lpw_address,sequence_number);

	relay_mac_address = lpw_support_get_mac_from_lpw_address(relay_lpw_address);
	lpw_send_to_mac(reply_pk_ptr,relay_mac_address);

	op_pk_destroy(pk_ptr);
}

/*
void lpw_transmit_reply_from_relay(Packet *pk_ptr) {

	Packet* reply_pk_ptr;
	int request_source_lpw_address; 
	int request_destination_lpw_address;
	int relay_lpw_address;
	int relay_mac_address;
	int node_lpw_address;
	char field[10];
	int seg_left;
	int size_route;
	int sequence_number;
	sRoute reply_route;
	double delay;
	sReply* reply;
	int* intPtr;
	int i;
	Distribution* delay_reply_dist;
	
	printf("Transmit reply from reply\n");
	
	op_pk_nfd_get(pk_ptr,"Size_Route",&size_route);
	op_pk_nfd_get(pk_ptr,"DEST",&request_destination_lpw_address);

	i = (size_route + route_cache[request_destination_lpw_address].size_route);
	printf("\n\ntransmit reply from relay:   %d\n\n",i);
	
	if ((size_route + route_cache[request_destination_lpw_address].size_route) < MAX_SIZE_ROUTE) {
	
		printf("in the reply from relay\n");
		reply_pk_ptr = op_pk_create_fmt("Lpw_Reply");
		op_pk_nfd_set(reply_pk_ptr,"Type",REPLY_PACKET_TYPE);
		op_pk_nfd_set(reply_pk_ptr,"SRC",request_destination_lpw_address);
		op_pk_nfd_get(pk_ptr,"SRC",&request_source_lpw_address);
		op_pk_nfd_set(reply_pk_ptr,"DEST",request_source_lpw_address);
		op_pk_nfd_set(reply_pk_ptr,"Reply_From_Target",0);
		op_pk_nfd_get(pk_ptr,"Seq_number",&sequence_number);
		op_pk_nfd_set(reply_pk_ptr,"Seq_number",sequence_number);

		for (i = 0;i < size_route;i++) {
		
			sprintf(field,"Node_%d",i);
			op_pk_nfd_get(pk_ptr,field,&node_lpw_address);
			reply_route.route[i] = node_lpw_address;
		}

		for (i = size_route;i < size_route + route_cache[request_destination_lpw_address].size_route;i++) {
			reply_route.route[i] = route_cache[request_destination_lpw_address].route[i-size_route];
		}
	
		reply_route.size_route=size_route + route_cache[request_destination_lpw_address].size_route;
	
		lpw_no_loop_in_route(&reply_route);
		i=0;
		do {
			i++;
		} while ((reply_route.route[i] != my_lpw_address) && (i < reply_route.size_route));
			
		if (i < reply_route.size_route) {
		
			relay_lpw_address = reply_route.route[i-1];
			sprintf(field,"Node_%d",i-1);
			op_pk_nfd_set(reply_pk_ptr,"RELAY",relay_lpw_address);				
			seg_left = i-1;
			op_pk_nfd_set(reply_pk_ptr,"Seg_Left",seg_left);

			for (i = 0;i < reply_route.size_route;i++) {
				sprintf(field,"Node_%d",i);
				op_pk_nfd_set(reply_pk_ptr,field,reply_route.route[i]);
			}
		
			op_pk_nfd_set(reply_pk_ptr,"Size_Route",reply_route.size_route);		
			op_pk_nfd_set(reply_pk_ptr,"TR_source",my_node_objid);
		
			lpw_reply_already_seen(my_lpw_address,request_source_lpw_address,sequence_number);

			delay_reply_dist=op_dist_load("uniform",0.0,1.0);
			delay=op_dist_outcome(delay_reply_dist);
			delay=HOP_DELAY*((double)(reply_route.size_route-1)+delay);
			
			intPtr=(int*)op_prg_mem_alloc(sizeof(int));
			*intPtr=request_destination_lpw_address*OPTION+request_source_lpw_address;
						
			reply = fifo_multiplex_extract(&reply_fifo,*intPtr);
			if(reply != NULL) {				
				complex_intrpt_cancel(my_complex_intrpt_ptr,reply->evt);
				op_pk_destroy(reply->pk);
				op_prg_mem_free(reply);
			}
			
			reply=(sReply*)op_prg_mem_alloc(sizeof(sReply));
			reply->pk=reply_pk_ptr;
			reply->sequence_number=sequence_number;
			
			reply->evt=complex_intrpt_schedule_self(my_complex_intrpt_ptr,op_sim_time(),REPLY_CODE,intPtr);
			fifo_multiplex_insert(&reply_fifo,reply,*intPtr);
		} else {
			op_pk_destroy(reply_pk_ptr);
		}
	}

	op_pk_destroy(pk_ptr);	
}
*/

int lpw_reply_already_seen(int source_lpw_address,int destination_lpw_address,int sequence_number) {

	if (reply_seen[source_lpw_address][destination_lpw_address]>=sequence_number) {
		return(1);
    } else {	
		reply_seen[source_lpw_address][destination_lpw_address]=sequence_number;	
		return (0);
	}
}

/*
  forward the reply packet received by the node
*/
void lpw_forward_reply(Packet* pk_ptr) {
	int seg_left;
	char field[10];
	int relay_lpw_address;
	int relay_mac_address;

	printf("forward reply \n");
	
	op_pk_nfd_get (pk_ptr,"Seg_Left",&seg_left);
	seg_left--;
	op_pk_nfd_set(pk_ptr,"Seg_Left",seg_left);

	sprintf(field,"Node_%d",seg_left);
	op_pk_nfd_get (pk_ptr,field,&relay_lpw_address);
	op_pk_nfd_set(pk_ptr,"RELAY",relay_lpw_address);

	op_pk_nfd_set(pk_ptr,"TR_source",my_node_objid);

	relay_mac_address=lpw_support_get_mac_from_lpw_address(relay_lpw_address);
	lpw_send_to_mac(pk_ptr,relay_mac_address);
}

/*
  forward the data packet received by the node
*/
void lpw_forward_data(Packet* pk_ptr) {
	int size_route;
	int seg_left;
	int next_relay_lpw_address;
	int next_relay_mac_address;
	char field[10];

	printf("forward data\n");
	
	op_pk_nfd_get (pk_ptr,"Seg_Left",&seg_left);
	op_pk_nfd_get (pk_ptr,"Size_Route",&size_route);

	sprintf(field,"Node_%d",size_route-seg_left);
	op_pk_nfd_get(pk_ptr,field,&next_relay_lpw_address);
	op_pk_nfd_set(pk_ptr,"RELAY",next_relay_lpw_address);
	op_pk_nfd_set(pk_ptr,"Seg_Left",--seg_left);
	op_pk_nfd_set(pk_ptr,"TR_source",my_node_objid);

	next_relay_mac_address=lpw_support_get_mac_from_lpw_address(next_relay_lpw_address);
	lpw_send_to_mac(pk_ptr,next_relay_mac_address);
}

/*
  communicate the address and the packet to send to the mac layer
*/
void lpw_send_to_mac(Packet* pk_ptr, int destination_mac_address) {
	int packet_type;
	Ici* iciptr;
	Packet* data_pk_ptr;
	int reply_source;
	int reply_dest;

	op_pk_nfd_get(pk_ptr,"Type",&packet_type);
	if (packet_type == DATA_PACKET_TYPE) {
		op_pk_nfd_get(pk_ptr,"data",&data_pk_ptr);
		op_pk_nfd_set(pk_ptr,"data",data_pk_ptr);
	} else {

	}

	// creatre and install an ici to communicate the mac address of the packet destination to the MAC layer
	iciptr = op_ici_create("Lpw_Wlan_Dest_Ici");
	op_ici_attr_set(iciptr,"Packet_Destination",destination_mac_address);
	op_ici_install(iciptr);
   
	printf("send to mac\n\n",packet_type);
	// send the packet to the MAC layer
	op_pk_send(pk_ptr,TO_MAC_LAYER_STRM);
}  

/*
 handle received reply packet
*/
void  lpw_handle_reply(Packet* pk_ptr) {
	int source_lpw_address;
	int destination_lpw_address;
	int sequence_number;
	int relay_lpw_address;
	int relay_mac_address;
	double creation_time;
	int size_route;
	int seg_left;
	int node_lpw_address;
	int current_node_index;
	Packet* data_pk_ptr;
	int from_target;
	char field[10];
	int i;

	printf("handle reply\n");
	op_pk_nfd_get (pk_ptr,"SRC",&source_lpw_address);
	op_pk_nfd_get (pk_ptr,"DEST",&destination_lpw_address); 
	op_pk_nfd_get (pk_ptr,"RELAY",&relay_lpw_address);
	op_pk_nfd_get (pk_ptr,"Seq_number",&sequence_number);
	op_pk_nfd_get (pk_ptr,"Reply_From_Target",&from_target);
	op_pk_nfd_get(pk_ptr,"Size_Route",&size_route);
	op_pk_nfd_get(pk_ptr,"Seg_Left",&seg_left);

	if ((!lpw_reply_already_seen(source_lpw_address,destination_lpw_address,sequence_number))) {
	
		// if the destination address is equal the node's address then cancel the request packet and begin transmit the data
		if (my_lpw_address == destination_lpw_address) {		
		
			lpw_insert_route_in_cache(pk_ptr);	
			if (request_sent[source_lpw_address].waiting_time!=0) {						
				if(complex_intrpt_cancel(my_complex_intrpt_ptr,request_sent[source_lpw_address].evt)) {
					printf("well done!\n");
				} else {
					op_sim_end("Amazing error!\n","","","");
				}
				request_sent[source_lpw_address].scheduling_time = 0.0;
				request_sent[source_lpw_address].waiting_time = 0.0;
			}
			
			data_pk_ptr=lpw_extract_buffer(source_lpw_address);
		   
			if (data_pk_ptr!=OPC_NIL){
				printf("&&&&&&&&&&&&&&&&&&&&&&&&\n");
				lpw_transmit_data(data_pk_ptr,source_lpw_address);
			}			

			op_pk_destroy(pk_ptr);
		} else if (my_lpw_address == relay_lpw_address) {
			// insert a truncated part of the route contained in the reply in its route_cache
			lpw_insert_route_in_cache(pk_ptr);
			lpw_forward_reply(pk_ptr);
		} else {
			op_pk_destroy(pk_ptr);
		}
	} else {
		op_pk_destroy(pk_ptr);
	}
}

/*
void lpw_no_loop_in_route(sRoute* route) {
	int i,j;
	int lpw_address1,lpw_address2;
	int size_route;
	int loop_detected;
	
	do {
		size_route=route->size_route;
		loop_detected=0;
		i=0;
	
		do {
			j=size_route-1;
			lpw_address1=route->route[i];
			do {
				lpw_address2=route->route[j];
				if ((lpw_address1==lpw_address2)&&(i!=j)) {
					loop_detected=1;
					j++; 
				} else {
					j--;
				}
			} while ((!loop_detected)&&(i<j));
			
			i++;	
		} while ((!loop_detected)&&(i<size_route-1));
	
		if (loop_detected) {
			for (j;j<size_route;j++) {
				route->route[i]=route->route[j];
				i++;
			}
			route->size_route=size_route-(j-i);
		}
	} while (loop_detected);
}
*/

/*
  insert every routes (leading to the request destination and to the intermediate nodes) coming 
  from a reply  packet in the node cache of the node
*/
void lpw_insert_route_in_cache(Packet* pk_ptr) {
	int destination_lpw_address;
	int node_lpw_address;
	int current_node_index;
	int seg_left;
	char field[10];
	int size_route;
	int node_listed;
	int i,j;
	
	printf("insert route table\n");

	op_pk_nfd_get(pk_ptr,"Size_Route",&size_route);
	op_pk_nfd_get(pk_ptr,"Seg_Left",&seg_left);

	i=0;
	current_node_index=-1;
	do {
		sprintf(field,"Node_%d",i);
		op_pk_nfd_get(pk_ptr,field,&node_lpw_address);
		if (node_lpw_address == my_lpw_address) {
			current_node_index = i;
		} else { 
			i++;
		}
	} while ((current_node_index==-1) && (i<size_route));

	if (current_node_index != -1) {
		
		for (i = seg_left+1;i < size_route;i++) {
			sprintf(field,"Node_%d",i);
			op_pk_nfd_get(pk_ptr,field,&destination_lpw_address);
			sprintf(field,"Node_%d",current_node_index);
			op_pk_nfd_get(pk_ptr,field,&node_lpw_address);
			
			if (node_lpw_address != my_lpw_address) {
				op_sim_end("Amazing error: the first address of the route should be mine","","","");
			} else {
				route_cache[destination_lpw_address].route[0]=node_lpw_address;
			}
			
			for (j = 1;j <= (i-seg_left);j++) {
				sprintf(field,"Node_%d",j+seg_left);
				op_pk_nfd_get(pk_ptr,field,&node_lpw_address);
				route_cache[destination_lpw_address].route[j] = node_lpw_address;
			}
			
			route_cache[destination_lpw_address].size_route = i-seg_left+1;
		}
	
		for (i = 0;i < NUMBER_OF_NODES;i++) {
			size_route = route_cache[i].size_route;
			for (j=0;j<size_route;j++) {
				printf("%d...",route_cache[i].route[j]);
			}			
		}
	} else {
		op_sim_end("Amazing error: the current node is not in the reply path","","","");
	}
}
	
int lpw_data_already_seen(int pk_id) {

	double* time;
	int tmp_int; 
	int already_received;

	time=fifo_multiplex_extract(&received_packet_id_fifo,pk_id);

	if (time==OPC_NIL) {
		time=(double *)malloc(sizeof(double));
		*time=op_sim_time();
		fifo_multiplex_insert(&received_packet_id_fifo,time,pk_id);
		already_received=0;
	} else {
		*time=op_sim_time();
		fifo_multiplex_insert(&received_packet_id_fifo,time,pk_id);
		already_received=1;
	}

	time=OPC_NIL;
	
	do {
		if (time!=OPC_NIL) free(time);
		time=fifo_multiplex_read_first(&received_packet_id_fifo,&tmp_int);
		if ((time!=OPC_NIL)&&((*time+DATA_LIFE_TIME)<op_sim_time())) {
			time=fifo_multiplex_extract_first(&received_packet_id_fifo,&tmp_int);
		}
	} while ((time!=OPC_NIL)&&((*time+DATA_LIFE_TIME)<op_sim_time()));

	return (already_received);
}

/*
 * fills in and sends the data packet destined to the specified destination
 */
void lpw_transmit_data(Packet* pk_ptr, int destination_lpw_address) {
	int size_route;
	int relay_mac_address;
	char field[10];
	int i;

	printf("Transmit data \n");
	
	op_pk_nfd_set(pk_ptr,"DEST",destination_lpw_address);
	op_pk_nfd_set(pk_ptr,"SRC",my_lpw_address);

	size_route = route_cache[destination_lpw_address].size_route;
	op_pk_nfd_set(pk_ptr,"Size_Route",size_route);
	op_pk_nfd_set(pk_ptr,"Seg_Left",size_route-2);

	for (i = 0;i < size_route;i++) {
		sprintf(field,"Node_%d",i);
		op_pk_nfd_set(pk_ptr,field,route_cache[destination_lpw_address].route[i]);
	}

	op_pk_nfd_set(pk_ptr,"RELAY",route_cache[destination_lpw_address].route[1]);
	relay_mac_address = lpw_support_get_mac_from_lpw_address(route_cache[destination_lpw_address].route[1]);

	lpw_data_already_seen(packet_id);
	op_pk_nfd_set (pk_ptr,"packet_ID",packet_id++);
	op_pk_nfd_set(pk_ptr,"TR_source",my_node_objid);
	lpw_send_to_mac(pk_ptr,relay_mac_address);
}

/*
 * handle the data packet,the data is the upper layer's data.
 */
void lpw_handle_data(Packet* pk_ptr) {
	int source_lpw_address;
	int destination_lpw_address;
	int relay_lpw_address;
	int sequence_number;
	int current_node_index;
	int size_route;
	Objid trsource;
	int seg_left;
	int pk_id;
	Packet* data_pk_ptr;

	
	printf("handle data\n");
	op_pk_nfd_get (pk_ptr,"SRC",&source_lpw_address);
	op_pk_nfd_get (pk_ptr,"DEST",&destination_lpw_address); 
	op_pk_nfd_get (pk_ptr,"RELAY",&relay_lpw_address);
	op_pk_nfd_get (pk_ptr,"TR_source",&trsource);
//	op_pk_nfd_get (pk_ptr,"Seq_number",&sequence_number);
	op_pk_nfd_get(pk_ptr,"Size_Route",&size_route);
	op_pk_nfd_get (pk_ptr,"packet_ID",&pk_id);

	if (!lpw_data_already_seen(pk_id)) {
		if ((my_lpw_address == destination_lpw_address) && (my_lpw_address == relay_lpw_address)) {
			op_pk_nfd_get (pk_ptr, "data", &data_pk_ptr);
			printf("linpeiwen\n");
			// send the data to the upper layer
			op_pk_send(data_pk_ptr,TO_UPPER_LAYER_STRM); 
			op_pk_destroy(pk_ptr);
		// if the current node is the destination but not the last relay,maybe there is a shorter path,but i ignore it.
		} else if ((my_lpw_address == destination_lpw_address) && (my_lpw_address != relay_lpw_address)) {
			op_pk_nfd_get (pk_ptr, "data", &data_pk_ptr);
			printf("zengping\n");
			// send the data to the upper layer
			op_pk_send(data_pk_ptr,TO_UPPER_LAYER_STRM); 
			op_pk_destroy(pk_ptr);
		} else if ((my_lpw_address == relay_lpw_address) && (my_lpw_address != destination_lpw_address)) {
			printf("weidi\n");
			lpw_forward_data(pk_ptr);		
		} else if ((my_lpw_address!=destination_lpw_address)&&(my_lpw_address!=relay_lpw_address)) {
			op_pk_destroy(pk_ptr);
		}
	} else {
		op_pk_destroy(pk_ptr);
	}
}

/*
  the upper layer strm is come,then handle the packet strm
*/
void lpw_upper_layer_data_arrival (Packet* data_pk_ptr, int destination_lpw_address) {
	int size_route;
	Ici * ici_dest_address;
	Packet* pk_ptr;

	pk_ptr=op_pk_create_fmt("Lpw_Data");
	op_pk_nfd_set(pk_ptr,"Type",DATA_PACKET_TYPE);
	op_pk_nfd_set (pk_ptr, "data", data_pk_ptr);

	size_route=route_cache[destination_lpw_address].size_route;

	printf("lpw_upper_layer_data_arrival\n");
	if ((size_route != 0) && (lpw_buffer_empty(destination_lpw_address))) {
		lpw_transmit_data(pk_ptr,destination_lpw_address);
	} else if((size_route != 0) && (!lpw_buffer_empty(destination_lpw_address))) {
		lpw_insert_buffer(pk_ptr,destination_lpw_address);
	} else {
		if (request_sent[destination_lpw_address].waiting_time==0) {
			lpw_transmit_request(destination_lpw_address);
			lpw_insert_buffer(pk_ptr,destination_lpw_address);
		} else {
			lpw_insert_buffer(pk_ptr,destination_lpw_address);
		}
	}
}

/*
  insert into the buffer for the  given destination int destination_lpw_address,the buffer store the data packet
*/
void lpw_insert_buffer(Packet* pk_ptr,int destination_lpw_address) {
	if(op_subq_pk_insert(destination_lpw_address,pk_ptr,OPC_QPOS_TAIL) != OPC_QINS_OK) {
		op_pk_destroy(pk_ptr);
	} else {		
	}
}

/*
 check the buffer for the  given destination int destination_lpw_address is empty or not
*/
int lpw_buffer_empty(int destination_lpw_address) {
	return (op_subq_empty(destination_lpw_address));
}

/*
  extract a data packet from the buffer for the  given destination int destination_lpw_address 
*/
Packet* lpw_extract_buffer(int destination_lpw_address) {
	Packet* pk_ptr;	
	if(op_subq_empty(destination_lpw_address) == OPC_FALSE) {
		pk_ptr = op_subq_pk_remove(destination_lpw_address,OPC_QPOS_HEAD);	
	} else {
		pk_ptr = OPC_NIL;
	}
	return pk_ptr;
}

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
	void lpw_route (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Obtype lpw_route_init (int * init_block_ptr);
	VosT_Address lpw_route_alloc (VOS_THREAD_INDEX_ARG_COMMA VosT_Obtype, int);
	void lpw_route_diag (OP_SIM_CONTEXT_ARG_OPT);
	void lpw_route_terminate (OP_SIM_CONTEXT_ARG_OPT);
	void lpw_route_svar (void *, const char *, void **);


	VosT_Fun_Status Vos_Define_Object (VosT_Obtype * obst_ptr, const char * name, unsigned int size, unsigned int init_obs, unsigned int inc_obs);
	VosT_Address Vos_Alloc_Object_MT (VOS_THREAD_INDEX_ARG_COMMA VosT_Obtype ob_hndl);
	VosT_Fun_Status Vos_Poolmem_Dealloc_MT (VOS_THREAD_INDEX_ARG_COMMA VosT_Address ob_ptr);
#if defined (__cplusplus)
} /* end of 'extern "C"' */
#endif




/* Process model interrupt handling procedure */


void
lpw_route (OP_SIM_CONTEXT_ARG_OPT)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	FIN_MT (lpw_route ());
	if (1)
		{
		Packet* pk_ptr;
		Ici* ici_dest_address;
		Ici* ici_ack;
		int destination_lpw_address;
		int relay_lpw_address;
		int relay_mac_address;
		int packet_type;
		sReply* reply;
		int i;
		int *intPtr;
		double theTime;
		//sNoAck* no_ack;
		//sNoAck* no_ack_first;int tmp;


		FSM_ENTER ("lpw_route")

		FSM_BLOCK_SWITCH
			{
			/*---------------------------------------------------------*/
			/** state (pre) enter executives **/
			FSM_STATE_ENTER_UNFORCED_NOLABEL (0, "pre", "lpw_route [pre enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_route [pre enter execs], state0_enter_exec)
				{
				printf("route layer\n");
				
				lpw_support_start();
				// preinitialize the process
				lpw_pre_init();
				op_intrpt_schedule_self(op_sim_time(),0);
				}

				FSM_PROFILE_SECTION_OUT (lpw_route [pre enter execs], state0_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (1,"lpw_route")


			/** state (pre) exit executives **/
			FSM_STATE_EXIT_UNFORCED (0, "pre", "lpw_route [pre exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_route [pre exit execs], state0_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_route [pre exit execs], state0_exit_exec)


			/** state (pre) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "pre", "init")
				/*---------------------------------------------------------*/



			/** state (init) enter executives **/
			FSM_STATE_ENTER_FORCED (1, "init", state1_enter_exec, "lpw_route [init enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_route [init enter execs], state1_enter_exec)
				{
				lpw_user_parameter_init();
				
				lpw_support_validate_addresses();
				
				lpw_tables_init();
				}

				FSM_PROFILE_SECTION_OUT (lpw_route [init enter execs], state1_enter_exec)

			/** state (init) exit executives **/
			FSM_STATE_EXIT_FORCED (1, "init", "lpw_route [init exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_route [init exit execs], state1_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_route [init exit execs], state1_exit_exec)


			/** state (init) transition processing **/
			FSM_TRANSIT_FORCE (2, state2_enter_exec, ;, "default", "", "init", "idle")
				/*---------------------------------------------------------*/



			/** state (idle) enter executives **/
			FSM_STATE_ENTER_UNFORCED (2, "idle", state2_enter_exec, "lpw_route [idle enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_route [idle enter execs], state2_enter_exec)
				{
				}

				FSM_PROFILE_SECTION_OUT (lpw_route [idle enter execs], state2_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (5,"lpw_route")


			/** state (idle) exit executives **/
			FSM_STATE_EXIT_UNFORCED (2, "idle", "lpw_route [idle exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_route [idle exit execs], state2_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_route [idle exit execs], state2_exit_exec)


			/** state (idle) transition processing **/
			FSM_PROFILE_SECTION_IN (lpw_route [idle trans conditions], state2_trans_conds)
			FSM_INIT_COND (UPPER_LAYER_ARRIVAL)
			FSM_TEST_COND (REPLY)
			FSM_TEST_COND (MAC_LAYER_ARRIVAL)
			FSM_TEST_COND (ACK)
			FSM_DFLT_COND
			FSM_TEST_LOGIC ("idle")
			FSM_PROFILE_SECTION_OUT (lpw_route [idle trans conditions], state2_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 3, state3_enter_exec, ;, "UPPER_LAYER_ARRIVAL", "", "idle", "Upper Layer")
				FSM_CASE_TRANSIT (1, 5, state5_enter_exec, ;, "REPLY", "", "idle", "Reply")
				FSM_CASE_TRANSIT (2, 4, state4_enter_exec, ;, "MAC_LAYER_ARRIVAL", "", "idle", "Mac Layer")
				FSM_CASE_TRANSIT (3, 6, state6_enter_exec, ;, "ACK", "", "idle", "Ack")
				FSM_CASE_TRANSIT (4, 2, state2_enter_exec, ;, "default", "", "idle", "idle")
				}
				/*---------------------------------------------------------*/



			/** state (Upper Layer) enter executives **/
			FSM_STATE_ENTER_FORCED (3, "Upper Layer", state3_enter_exec, "lpw_route [Upper Layer enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_route [Upper Layer enter execs], state3_enter_exec)
				{
				
				pk_ptr = op_pk_get(FROM_UPPER_LAYER_STRM);
				
				ici_dest_address = op_intrpt_ici();
				op_ici_attr_get(ici_dest_address,"Packet_Destination",&destination_lpw_address);
				printf("Upper Layer Arrival!!!!!!!!!!!!!!!!!!\n",destination_lpw_address);
				
				lpw_upper_layer_data_arrival(pk_ptr,destination_lpw_address);
				}

				FSM_PROFILE_SECTION_OUT (lpw_route [Upper Layer enter execs], state3_enter_exec)

			/** state (Upper Layer) exit executives **/
			FSM_STATE_EXIT_FORCED (3, "Upper Layer", "lpw_route [Upper Layer exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_route [Upper Layer exit execs], state3_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_route [Upper Layer exit execs], state3_exit_exec)


			/** state (Upper Layer) transition processing **/
			FSM_TRANSIT_FORCE (2, state2_enter_exec, ;, "default", "", "Upper Layer", "idle")
				/*---------------------------------------------------------*/



			/** state (Mac Layer) enter executives **/
			FSM_STATE_ENTER_FORCED (4, "Mac Layer", state4_enter_exec, "lpw_route [Mac Layer enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_route [Mac Layer enter execs], state4_enter_exec)
				{
				pk_ptr = op_pk_get(FROM_MAC_LAYER_STRM);
				
				if(lpw_in_transmission_range(pk_ptr)) {
					op_pk_nfd_get(pk_ptr,"Type",&packet_type);
					switch(packet_type) {
						case DATA_PACKET_TYPE:			
							lpw_handle_data(pk_ptr);
							break;
						case REPLY_PACKET_TYPE:
							lpw_handle_reply(pk_ptr);
							break;
						case REQUEST_PACKET_TYPE:
							lpw_handle_request(pk_ptr);
							break;
				//		case ERROR_PACKET_TYPE:
				//			lpw_handle_error(pk_ptr);
				//			break;
						default:
							op_pk_destroy(pk_ptr);
					}
				} else {
					op_pk_destroy(pk_ptr);
				}
				
				
				}

				FSM_PROFILE_SECTION_OUT (lpw_route [Mac Layer enter execs], state4_enter_exec)

			/** state (Mac Layer) exit executives **/
			FSM_STATE_EXIT_FORCED (4, "Mac Layer", "lpw_route [Mac Layer exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_route [Mac Layer exit execs], state4_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_route [Mac Layer exit execs], state4_exit_exec)


			/** state (Mac Layer) transition processing **/
			FSM_TRANSIT_FORCE (2, state2_enter_exec, ;, "default", "", "Mac Layer", "idle")
				/*---------------------------------------------------------*/



			/** state (Reply) enter executives **/
			FSM_STATE_ENTER_FORCED (5, "Reply", state5_enter_exec, "lpw_route [Reply enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_route [Reply enter execs], state5_enter_exec)
				{
				
				intPtr=complex_intrpt_data(my_complex_intrpt_ptr);
				
				reply=(sReply*)fifo_multiplex_extract(&reply_fifo,*intPtr);
				
				op_prg_mem_free(intPtr);
				
				if (reply!=NULL) {
					op_pk_nfd_get(reply->pk,"RELAY",&relay_lpw_address);
					relay_mac_address=lpw_support_get_mac_from_lpw_address(relay_lpw_address);
					lpw_send_to_mac(reply->pk,relay_mac_address);
					op_prg_mem_free(reply);
				} else {
					op_sim_end("Amazing error: no reply information were found in the fifo","","","");
				}
				
				complex_intrpt_close(my_complex_intrpt_ptr);
				}

				FSM_PROFILE_SECTION_OUT (lpw_route [Reply enter execs], state5_enter_exec)

			/** state (Reply) exit executives **/
			FSM_STATE_EXIT_FORCED (5, "Reply", "lpw_route [Reply exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_route [Reply exit execs], state5_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_route [Reply exit execs], state5_exit_exec)


			/** state (Reply) transition processing **/
			FSM_TRANSIT_FORCE (2, state2_enter_exec, ;, "default", "", "Reply", "idle")
				/*---------------------------------------------------------*/



			/** state (Ack) enter executives **/
			FSM_STATE_ENTER_FORCED (6, "Ack", state6_enter_exec, "lpw_route [Ack enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_route [Ack enter execs], state6_enter_exec)
				{
				
				intPtr = complex_intrpt_data(my_complex_intrpt_ptr);
				
				
				if (request_sent[*intPtr].waiting_time!=0) {
				
					lpw_transmit_request(*intPtr);
				}
				
				op_prg_mem_free(intPtr);
				complex_intrpt_close(my_complex_intrpt_ptr);
				
				
				
				}

				FSM_PROFILE_SECTION_OUT (lpw_route [Ack enter execs], state6_enter_exec)

			/** state (Ack) exit executives **/
			FSM_STATE_EXIT_FORCED (6, "Ack", "lpw_route [Ack exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_route [Ack exit execs], state6_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_route [Ack exit execs], state6_exit_exec)


			/** state (Ack) transition processing **/
			FSM_TRANSIT_FORCE (2, state2_enter_exec, ;, "default", "", "Ack", "idle")
				/*---------------------------------------------------------*/



			}


		FSM_EXIT (0,"lpw_route")
		}
	}




void
lpw_route_diag (OP_SIM_CONTEXT_ARG_OPT)
	{
	/* No Diagnostic Block */
	}




void
lpw_route_terminate (OP_SIM_CONTEXT_ARG_OPT)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = __LINE__;
#endif

	FIN_MT (lpw_route_terminate ())

	Vos_Poolmem_Dealloc_MT (OP_SIM_CONTEXT_THREAD_INDEX_COMMA pr_state_ptr);

	FOUT
	}


/* Undefine shortcuts to state variables to avoid */
/* syntax error in direct access to fields of */
/* local variable prs_ptr in lpw_route_svar function. */
#undef my_objid
#undef my_node_objid
#undef my_network_objid
#undef my_mac_address
#undef my_lpw_address
#undef my_name
#undef route_cache
#undef request_sent
#undef request_sequence_number
#undef request_seen
#undef reply_seen
#undef my_complex_intrpt_ptr
#undef received_packet_id_fifo
#undef reply_fifo

#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE

#define FIN_PREAMBLE_DEC
#define FIN_PREAMBLE_CODE

VosT_Obtype
lpw_route_init (int * init_block_ptr)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	VosT_Obtype obtype = OPC_NIL;
	FIN_MT (lpw_route_init (init_block_ptr))

	Vos_Define_Object (&obtype, "proc state vars (lpw_route)",
		sizeof (lpw_route_state), 0, 20);
	*init_block_ptr = 0;

	FRET (obtype)
	}

VosT_Address
lpw_route_alloc (VOS_THREAD_INDEX_ARG_COMMA VosT_Obtype obtype, int init_block)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	lpw_route_state * ptr;
	FIN_MT (lpw_route_alloc (obtype))

	ptr = (lpw_route_state *)Vos_Alloc_Object_MT (VOS_THREAD_INDEX_COMMA obtype);
	if (ptr != OPC_NIL)
		ptr->_op_current_block = init_block;
	FRET ((VosT_Address)ptr)
	}



void
lpw_route_svar (void * gen_ptr, const char * var_name, void ** var_p_ptr)
	{
	lpw_route_state		*prs_ptr;

	FIN_MT (lpw_route_svar (gen_ptr, var_name, var_p_ptr))

	if (var_name == OPC_NIL)
		{
		*var_p_ptr = (void *)OPC_NIL;
		FOUT
		}
	prs_ptr = (lpw_route_state *)gen_ptr;

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
	if (strcmp ("my_mac_address" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_mac_address);
		FOUT
		}
	if (strcmp ("my_lpw_address" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_lpw_address);
		FOUT
		}
	if (strcmp ("my_name" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_name);
		FOUT
		}
	if (strcmp ("route_cache" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->route_cache);
		FOUT
		}
	if (strcmp ("request_sent" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->request_sent);
		FOUT
		}
	if (strcmp ("request_sequence_number" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->request_sequence_number);
		FOUT
		}
	if (strcmp ("request_seen" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->request_seen);
		FOUT
		}
	if (strcmp ("reply_seen" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->reply_seen);
		FOUT
		}
	if (strcmp ("my_complex_intrpt_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_complex_intrpt_ptr);
		FOUT
		}
	if (strcmp ("received_packet_id_fifo" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->received_packet_id_fifo);
		FOUT
		}
	if (strcmp ("reply_fifo" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->reply_fifo);
		FOUT
		}
	*var_p_ptr = (void *)OPC_NIL;

	FOUT
	}

