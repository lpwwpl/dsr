/* Process model C form file: lpw_routing_layer.pr.c */
/* Portions of this file copyright 1992-2003 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
const char lpw_routing_layer_pr_c [] = "MIL_3_Tfile_Hdr_ 100A 30A modeler 7 49AD4307 49AD4307 1 lpw Administrator 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 8f3 1                                                                                                                                                                                                                                                                                                                                                                                                         ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>



/* Header Block */

///////////////////////////////////////////////////////////////
// DSR HEADER BLOCK
//
// declaration of globale variables, type, constants ... used in
// the dsr process model
///////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////
//////////////////////// INCLUDE ////////////////////////////// 
///////////////////////////////////////////////////////////////

#include <math.h>
#include <opnet.h>
#include <string.h>
#include "dsr_support.h"
#include "fifo.h"
#include "complex_intrpt.h"

///////////////////////////////////////////////////////////////
///////////////// CONSTANTS DEFINITION ////////////////////////
///////////////////////////////////////////////////////////////

// stream number definition
#define FROM_UPPER_LAYER_STRM 1
#define FROM_MAC_LAYER_STRM 0
#define TO_MAC_LAYER_STRM 0
#define TO_UPPER_LAYER_STRM 1

// packet types definition
#define DATA_PACKET_TYPE 5
#define REQUEST_PACKET_TYPE 7
#define REPLY_PACKET_TYPE 11
#define ERROR_PACKET_TYPE 13

// interrruption codes definition
#define ACK_CODE 1
#define ERROR_CODE 2
#define SELF_ERROR_CODE 3
#define NO_REPLY_CODE 4
#define SEND_REPLY_CODE 5
// constant used to store 2 informations (source and destination) in an integer
#define OPTION 10000    

// other constants definition
#define BROADCAST -1

// constant used to removed the old received data packet ids from the memory
#define DATA_LIFE_TIME 5

// maximum number of hops in a route
#define  MAX_SIZE_ROUTE 8

///////////////////////////////////////////////////////////////
///////////// TRANSITION MACROS DEFINITION ////////////////////
///////////////////////////////////////////////////////////////

#define UPPER_LAYER_ARRIVAL (op_intrpt_type()==OPC_INTRPT_STRM && op_intrpt_strm()==FROM_UPPER_LAYER_STRM)
#define MAC_LAYER_ARRIVAL (op_intrpt_type()==OPC_INTRPT_STRM && op_intrpt_strm()==FROM_MAC_LAYER_STRM)
#define NO_REPLY (op_intrpt_type()==OPC_INTRPT_SELF && complex_intrpt_code(my_complex_intrpt_ptr)==NO_REPLY_CODE)
#define SEND_REPLY (op_intrpt_type()==OPC_INTRPT_SELF && complex_intrpt_code(my_complex_intrpt_ptr)==SEND_REPLY_CODE)
#define ERROR ((op_intrpt_type()==OPC_INTRPT_REMOTE && op_intrpt_code()==ERROR_CODE) || (op_intrpt_type()==OPC_INTRPT_SELF && complex_intrpt_code(my_complex_intrpt_ptr)==SELF_ERROR_CODE))
#define ACK (op_intrpt_type()==OPC_INTRPT_REMOTE && op_intrpt_code()==ACK_CODE)
#define END_SIM	(op_intrpt_type()==OPC_INTRPT_ENDSIM)

///////////////////////////////////////////////////////////////
///////////////////// TYPES DEFINITION ////////////////////////
///////////////////////////////////////////////////////////////

// definition of the sRoute structure
typedef struct
	{
	int route[MAX_SIZE_ROUTE];	// the route = array of integer
	int size_route;				// the size of the route
	} sRoute;

// definition of the sRequestSent structure
typedef struct
	{
	int sequence_number;	// the sequence number of the request
	double scheduling_time; // the time when a new request should be sent
	double waiting_time;	// the total time we have to wait before scheduling_time
	Evhandle evt;			// the event associated with the scheduling_time timer
	} sRequestSent;

// definition of the sReply structure used to store a scheduled reply 
typedef struct
	{
	int sequence_number; 	// the sequence number of the reply
	Packet* pk;				// the reply_packet
	Evhandle evt;			// the intrpt event which will "say" when to send the reply
	} sReply;

// definition of the sNoAck structure used to store information about the last data packet sent 
// and to schedule an event that will say "no ack has been received for this packet => error "
typedef struct
	{
	Evhandle evt;			// event indicating that no ack has been received => error (either the MAC layer does not reply, or no explicit dsr ack has been received)
	double schedule;		// time at wchich this event is scheduled
	sRoute route;				// route used by the data packet
	Packet* upper_layer_data; 	// upper layer data transmitted in the data packet
	int packet_id;			// packet_id of the data packet
	} sNoAck;


///////////////////////////////////////////////////////////////
////////////// GLOBAL VARIABLES DECLARATION ///////////////////
///////////////////////////////////////////////////////////////

int NUMBER_OF_NODES; 			// the number of nodes using dsr in the network
double TRANSMISSION_RANGE;		// the transmission range
double WAIT_REPLY_MIN_TIME;		// minimum time to wait for a reply after sending a propa request
double WAIT_REPLY_MAX_TIME;		// maximum time to wait for a reply after sending a propa request
double WAIT_NON_PROPA_REPLY_TIME;	// time to wait for a reply after sending a non propa request
double HOP_DELAY;				// constant time per hop that a node has to wait before sending its reply
double REQUEST_LIFE_TIME;		// maximum life time of a request packet
int NON_PROPA_REQUEST_MECHANISM;  	// flag for the non propagating request mechanism
int MAC_LAYER_ACK=1;			// flag for the mac layer ack (activated since in this first model version we used 802.11 as mac layer)
double WAIT_ACK_TIME;			// maximum waiting to wait for an ack or an error from the mac layer after sending a data packet1
int packet_id=0;				// the id that will be used by the next data packet to be transmitted
char message[200];				// use as temporary variable to display some messages


// requests statistics
Stathandle stat_total_non_propa_requests;
int total_non_propa_requests;
Stathandle stat_total_propa_requests;
int total_propa_requests;
Stathandle stat_total_renewed_propa_requests;
int total_renewed_propa_requests;
Stathandle stat_total_requests;
int total_requests;
Stathandle stat_total_requests_from_error;
int total_requests_from_error;

// replies statistics
Stathandle stat_total_replies;
int total_replies;
Stathandle stat_total_replies_from_target;
int total_replies_from_target;
Stathandle stat_total_replies_from_relay;
int total_replies_from_relay;
Stathandle stat_total_canceled_replies;
int total_canceled_replies;
Stathandle stat_total_gratuitous_replies;
int total_gratuitous_replies;

// data statistics
Stathandle stat_total_data_served;
int total_data_served;
Stathandle stat_total_data_in_buffer;
int total_data_in_buffer;
Stathandle stat_total_data_successfully_transmitted;
int total_data_successfully_transmitted;
Stathandle stat_total_data_efficiency;

// errors statistics
int total_error_detected;
Stathandle stat_total_error_detected;

// other statistics
int total_ack;
Stathandle stat_total_ack;
Stathandle stat_mean_hops_by_route;
int amazing_errors;
Stathandle stat_amazing_errors;
int total_data_transmit;
Stathandle stat_total_data_transmit;
int total_control_transmit;
Stathandle stat_total_control_transmit;
Stathandle stat_total_overhead;


///////////////////////////////////////////////////////////////
///////////////// DSR FUNCTIONS' HEADER ///////////////////////
///////////////////////////////////////////////////////////////

// init functions
void dsr_pre_init();
void dsr_user_parameter_init();
void dsr_tables_init();
void dsr_stats_init();
void dsr_route_init(sRoute* cache,int n);

// route discovery functions
void dsr_transmit_request(int dest_dsr_address);
void dsr_transmit_request_from_error(int destination_dsr_address);
void dsr_handle_request(Packet * pk_ptr);
int dsr_request_already_seen(int source_dsr_address,int destination_dsr_address,int sequence_number);
void dsr_forward_request(Packet* pk_ptr);
void dsr_transmit_reply_from_target(Packet* pk_ptr);
void dsr_transmit_reply_from_relay(Packet *pk_ptr);
void  dsr_handle_reply(Packet* pk_ptr);
int dsr_reply_already_seen(int source_dsr_address,int destination_dsr_address,int sequence_number);
void dsr_forward_reply(Packet* pk_ptr);
void dsr_insert_route_in_cache(Packet* pk_ptr);
void dsr_promiscuous_reply(Packet* pk_ptr);

// data transmission functions
void dsr_upper_layer_data_arrival (Packet* data_pk_ptr, int dest_dsr_address);
void dsr_transmit_data(Packet* pk_ptr, int dest_dsr_address);
void dsr_handle_data(Packet* pk_ptr);
void dsr_schedule_no_ack_event(Packet* pk_ptr);
int dsr_data_already_seen(int pk_id);
void dsr_forward_data(Packet* pk_ptr);

// route maintenance functions
void dsr_transmit_error(sRoute route, int error_node_dsr_address);
void  dsr_handle_error(Packet* pk_ptr);
int dsr_ckeck_gratuitous_reply(Packet* pk_ptr);
void dsr_transmit_gratuitous_reply(int current_node_index, Packet* pk_ptr);
void dsr_clean_cache(int first_node_dsr_address,int second_node_dsr_address);

// other functions
int dsr_in_transmission_range(Packet* pk_ptr);
void dsr_insert_buffer(Packet* pk_ptr, int dest_dsr_address);
Boolean dsr_buffer_empty(int dest_dsr_address);
Packet* dsr_extract_buffer(int dest_dsr_address);
void dsr_no_loop_in_route(sRoute* route);
void dsr_send_to_mac(Packet *pk, int next_destination);
void dsr_message(const char* message);
void dsr_end_simulation();


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
	int	                    		my_dsr_address;
	char *	                 		my_name;
	sRoute *	               		route_cache;
	sRequestSent *	         		request_sent;
	int	                    		request_sequence_number;
	int**	                  		request_seen;
	sComplexIntrpt *	       		my_complex_intrpt_ptr;
	sFifo	                  		reply_fifo;
	int**	                  		reply_seen;
	sFifo	                  		received_packet_id_fifo;
	sFifo	                  		no_ack_fifo;
	} lpw_routing_layer_state;

#define pr_state_ptr            		((lpw_routing_layer_state*) (OP_SIM_CONTEXT_PTR->mod_state_ptr))
#define my_objid                		pr_state_ptr->my_objid
#define my_node_objid           		pr_state_ptr->my_node_objid
#define my_network_objid        		pr_state_ptr->my_network_objid
#define my_mac_address          		pr_state_ptr->my_mac_address
#define my_dsr_address          		pr_state_ptr->my_dsr_address
#define my_name                 		pr_state_ptr->my_name
#define route_cache             		pr_state_ptr->route_cache
#define request_sent            		pr_state_ptr->request_sent
#define request_sequence_number 		pr_state_ptr->request_sequence_number
#define request_seen            		pr_state_ptr->request_seen
#define my_complex_intrpt_ptr   		pr_state_ptr->my_complex_intrpt_ptr
#define reply_fifo              		pr_state_ptr->reply_fifo
#define reply_seen              		pr_state_ptr->reply_seen
#define received_packet_id_fifo 		pr_state_ptr->received_packet_id_fifo
#define no_ack_fifo             		pr_state_ptr->no_ack_fifo

/* These macro definitions will define a local variable called	*/
/* "op_sv_ptr" in each function containing a FIN statement.	*/
/* This variable points to the state variable data structure,	*/
/* and can be used from a C debugger to display their values.	*/
#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE
#if defined (OPD_PARALLEL)
#  define FIN_PREAMBLE_DEC	lpw_routing_layer_state *op_sv_ptr; OpT_Sim_Context * tcontext_ptr;
#  define FIN_PREAMBLE_CODE	\
		if (VosS_Mt_Perform_Lock) \
			VOS_THREAD_SPECIFIC_DATA_GET (VosI_Globals.simi_mt_context_data_key, tcontext_ptr, SimT_Context *); \
		else \
			tcontext_ptr = VosI_Globals.simi_sequential_context_ptr; \
		op_sv_ptr = ((lpw_routing_layer_state *)(tcontext_ptr->mod_state_ptr));
#else
#  define FIN_PREAMBLE_DEC	lpw_routing_layer_state *op_sv_ptr;
#  define FIN_PREAMBLE_CODE	op_sv_ptr = pr_state_ptr;
#endif


/* Function Block */


#if !defined (VOSD_NO_FIN)
enum { _op_block_origin = __LINE__ };
#endif
///////////////////////////////////////////////////////////////
// DSR FUNCTION BLOCK
//
// Declaration of all the functions used in the dsr process model
///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////// INIT FUNCTIONS //////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

/*************************************************************/
/*                     DSR_PRE_INIT                          */
/*************************************************************/
/* This function pre-initializes the dsr state machine
/* That is initializes all Objid and the mac and dsr addresses
/* used by the node
/*************************************************************/
void dsr_pre_init()
{
char address_field[100];

// init the objid of the current dsr process
my_objid=op_id_self();
// init the objid of the node containing this process
my_node_objid=op_topo_parent(my_objid);
// init the objid ot the network containing this node
my_network_objid=op_topo_parent(my_node_objid);
// init the name of the node containing this process
my_name=(char*)op_prg_mem_alloc(25*sizeof(char));
op_ima_obj_attr_get(my_node_objid,"name",my_name);

// init the mac address used by the current node
op_ima_obj_attr_get(my_objid,"Mac Address Field",address_field);
// if address_field is not equal  to "name" then it must contain the name of field where the MAC address is defined
if  (strcmp(address_field,"name")!=0)
	{
	op_ima_obj_attr_get(my_node_objid,address_field,&my_mac_address);
	}
// otherwise the name of the node will be converted in integer and used as the node mac address
else
	{
	op_ima_obj_attr_get(my_node_objid,"name",address_field);
	my_mac_address=atoi(address_field);
	}
// init the dsr address used by the current node
op_ima_obj_attr_get(my_objid,"Dsr Address",&my_dsr_address);
// if the "use the mac address" option is choosen then the mac and dsr address are equal
if (my_dsr_address==DSR_SUPPORT_USE_THE_MAC_ADDRESS)
	{
	// declare both dsr and mac address to the dsr support
	my_dsr_address=dsr_support_declare_node_addresses(my_node_objid,my_mac_address,my_mac_address);
	}
// if the "automatic assignation" option is choosen then the dsr_support external library provides an address
else if (my_dsr_address==DSR_SUPPORT_AUTOMATIC_ASSIGNATION)
	{
	my_dsr_address=dsr_support_declare_node_addresses(my_node_objid,DSR_SUPPORT_AUTOMATIC_ASSIGNATION,my_mac_address);
	}
}

/*************************************************************/
/*             DSR_USER_PARAMETERS_INIT                      */
/*************************************************************/
/* This function initialiazes every parameters defined by the
/* user
/*************************************************************/
void dsr_user_parameter_init()
{
// read and calcul the time constants defined by the users for the request purpose
op_ima_obj_attr_get(my_objid,"Dsr Wait Reply Min Time",&WAIT_REPLY_MIN_TIME);
op_ima_obj_attr_get(my_objid,"Dsr Wait Reply Max Time",&WAIT_REPLY_MAX_TIME);
WAIT_NON_PROPA_REPLY_TIME=WAIT_REPLY_MIN_TIME/MAX_SIZE_ROUTE;
// calcul the hop_delay time  to ensure that a node will have enought time to reply to a non propa request
HOP_DELAY=WAIT_NON_PROPA_REPLY_TIME/(MAX_SIZE_ROUTE*1.5);
op_ima_sim_attr_get(OPC_IMA_DOUBLE,"Dsr Transmission Range",&TRANSMISSION_RANGE);
op_ima_obj_attr_get(my_objid,"Dsr Wait Ack Time",&WAIT_ACK_TIME);

printf("---------\n%d\n-----------",TRANSMISSION_RANGE);
// read the maximum life time of a request packet
op_ima_obj_attr_get(my_objid,"Dsr Request Life Time",&REQUEST_LIFE_TIME);

// read the flags to see which mechanism are activated
op_ima_obj_attr_get(my_objid,"Dsr Non Propagating Request",&NON_PROPA_REQUEST_MECHANISM);
}

/*************************************************************/
/*                 DSR_TABLES_INIT                           */
/*************************************************************/
/* This function initialiazes every tables using by the routing 
/* protocol
/*************************************************************/
void dsr_tables_init()
{
int i,j;

// count the number of moving nodes in the network (all must use dsr)
NUMBER_OF_NODES=dsr_support_number_of_nodes();

// memory allocation of the route cache => one route for each destination 
route_cache=(sRoute*)op_prg_mem_alloc(NUMBER_OF_NODES*sizeof(sRoute));
// inititialisation of this route cache
for (i=0;i<NUMBER_OF_NODES;i++)
	{
	dsr_route_init(route_cache,i);
	}

// memory allocation of the request sent table => one request can be sent to each destination 
request_sent=(sRequestSent*)op_prg_mem_alloc(NUMBER_OF_NODES*sizeof(sRequestSent));
// inititialisation of this table
for (i=0;i<NUMBER_OF_NODES;i++)
	{
	request_sent[i].sequence_number=0;
	request_sent[i].scheduling_time=0;
	request_sent[i].waiting_time=0;
	}
// init the request sequence number 
request_sequence_number=0;

// memory allocation of the reply seen table => store the sequence number of the last seen request identified by its source and destination
request_seen=(int**)op_prg_mem_alloc(NUMBER_OF_NODES*sizeof(int*));
// inititialisation of this table
for (i=0;i<NUMBER_OF_NODES;i++)
	{
	request_seen[i]=(int*)op_prg_mem_alloc(NUMBER_OF_NODES*sizeof(int));; 
	for (j=0;j<NUMBER_OF_NODES;j++)
		{
		request_seen[i][j]=-1;	// no request seen
		}
	}
// memory allocation of the reply received table => store the sequence number of the last received reply identified by its source
reply_seen=(int**)op_prg_mem_alloc(NUMBER_OF_NODES*sizeof(int*));
for (i=0;i<NUMBER_OF_NODES;i++)
	{
	reply_seen[i]=(int*)op_prg_mem_alloc(NUMBER_OF_NODES*sizeof(int));; 
	for (j=0;j<NUMBER_OF_NODES;j++)
		{
		reply_seen[i][j]=-1;	// no reply seen
		}
	}

// memory allocation for the structure managing the complex intrpt
my_complex_intrpt_ptr=complex_intrpt_new();

// init the reply_fifo
fifo_init(&reply_fifo);
// init the received_packet_id_fifo
fifo_init(&received_packet_id_fifo);
// init the no_ack_fifo
fifo_init(&no_ack_fifo);
}

/*************************************************************/
/*                 DSR_STATS_INIT                            */
/*************************************************************/
/* This function initialiazes and declare every statistics that
/* will be calculated and collected by the dsr process model
/*************************************************************/
void dsr_stats_init()
{
// init the "requests" statistics
stat_total_non_propa_requests=op_stat_reg("total non propa requests",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
op_stat_write(stat_total_non_propa_requests,total_non_propa_requests=0);
stat_total_propa_requests=op_stat_reg("total propa requests",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
op_stat_write(stat_total_propa_requests,total_propa_requests=0);
stat_total_renewed_propa_requests=op_stat_reg("total renewed propa requests",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
op_stat_write(stat_total_renewed_propa_requests,total_renewed_propa_requests=0);
stat_total_requests_from_error=op_stat_reg("total requests from error",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
op_stat_write(stat_total_requests_from_error,total_requests_from_error=0);
stat_total_requests=op_stat_reg("total requests",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
op_stat_write(stat_total_requests,total_requests=0);

// init the "replies" statistics
stat_total_replies=op_stat_reg("total replies",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
op_stat_write(stat_total_replies,total_replies);
stat_total_replies_from_target=op_stat_reg("total replies from target",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
op_stat_write(stat_total_replies_from_target,total_replies_from_target=0);
stat_total_replies_from_relay=op_stat_reg("total replies from relay",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
op_stat_write(stat_total_replies_from_relay,total_replies_from_relay=0);
stat_total_canceled_replies=op_stat_reg("total canceled replies",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
op_stat_write(stat_total_canceled_replies,total_canceled_replies=0);
stat_total_gratuitous_replies=op_stat_reg("total gratuitous replies",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
op_stat_write(stat_total_gratuitous_replies,total_gratuitous_replies=0);

// init the "data packets" statistics
stat_total_data_served=op_stat_reg("total data served",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
op_stat_write(stat_total_data_served,total_data_served=0);
stat_total_data_in_buffer=op_stat_reg("total data in buffer",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
op_stat_write(stat_total_data_in_buffer,total_data_in_buffer=0);
stat_total_data_successfully_transmitted=op_stat_reg("total data successfully transmitted",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
op_stat_write(stat_total_data_successfully_transmitted,total_data_successfully_transmitted=0);
stat_total_data_efficiency=op_stat_reg("total data efficiency",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
op_stat_write(stat_total_data_efficiency,1);

// init the "error packets" statistic
stat_total_error_detected=op_stat_reg("total error detected",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
op_stat_write(stat_total_error_detected,total_error_detected=0);

// init the other stats
stat_total_ack=op_stat_reg("total ack",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
op_stat_write(stat_total_ack,total_ack=0);
stat_mean_hops_by_route=op_stat_reg("mean hops by route",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
op_stat_write(stat_mean_hops_by_route,0);
stat_amazing_errors=op_stat_reg("amazing errors",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
op_stat_write(stat_amazing_errors,amazing_errors=0);
stat_total_data_transmit=op_stat_reg("total data transmit",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
op_stat_write(stat_total_data_transmit,total_data_transmit=0);
stat_total_control_transmit=op_stat_reg("total control transmit",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
op_stat_write(stat_total_control_transmit,total_control_transmit=0);
stat_total_overhead=op_stat_reg("total overhead",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
op_stat_write(stat_total_overhead,100);
}

/*************************************************************/
/*                   DSR_ROUTE_INIT                          */
/*************************************************************/
/* This function initialiazes the nth route of the sRoute table
/* given in  parameter
/* 		sRoute cache: the cache in which the route must be initialized
/* 		int n: the index of the route (nth) tp initialize
/*************************************************************/
void dsr_route_init(sRoute* cache, int n)
{
int i;
// init every dsr_address contained in the route to -1
for(i=0;i<MAX_SIZE_ROUTE;i++)
	{
	cache[n].route[i]=-1;
	}
// init the size_route to 0
cache[n].size_route=0;
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
//////////////// ROUTE DISCOVERY FUNCTIONS ////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

/*************************************************************/
/*               DSR_TRANSMIT_REQUEST                        */
/*************************************************************/
/* This function build and send a request packet destined to the
/* specified destination
/* 		int destination_dsr_address: the dsr address of the wanted 
/* destination
/*************************************************************/
void dsr_transmit_request(int destination_dsr_address)
{
Packet* pk_ptr;
double timer;
double random;
Distribution* reply_waiting_dist;
int *destination_dsr_address_ptr;

// Create the request packet and set its Type field
pk_ptr=op_pk_create_fmt("Dsr_Request");
op_pk_nfd_set(pk_ptr,"Type",REQUEST_PACKET_TYPE);

// set the SRC field of the packet (I am the source)
op_pk_nfd_set(pk_ptr,"SRC",my_dsr_address);
// set the DEST field of the packet with wanted destination dsr address
op_pk_nfd_set(pk_ptr,"DEST",destination_dsr_address);
// set the Node_0 fiels of the packet (I am the first node of the route)
op_pk_nfd_set(pk_ptr,"Node_0",my_dsr_address);
// set the Size_Route field (I am the first node, thus 1)
op_pk_nfd_set(pk_ptr,"Size_Route",1);
// set creation_time field for TTL (Time To Live) purpose
op_pk_nfd_set(pk_ptr,"Creation_Time",op_sim_time());

// check if it is the sequence first request to the destination or if a previous request has failed 
timer=request_sent[destination_dsr_address].waiting_time;

// if it is the first request of the sequence and the non propagating request mechanism is activated
if ((timer==0)&&(NON_PROPA_REQUEST_MECHANISM))
	{
	// a non propagating request (seg_left=0) will be sent
	op_pk_nfd_set(pk_ptr,"Seg_Left",0);
	// a specific timer is assigned to the non propagating request
	timer=WAIT_NON_PROPA_REPLY_TIME;
		
	sprintf(message,"I am sending a NON PROPA REQUEST to %d with the seq number %d\n",destination_dsr_address,request_sequence_number+1);
	dsr_message(message);
	
	// "requests" statistics calculation
	op_stat_write(stat_total_non_propa_requests,++total_non_propa_requests);
	op_stat_write(stat_total_requests,++total_requests);
	}

// else a propagating request must be sent
else
	{
	// a propagating request (seg_left = max_size_route - first hop<=>2nodes) will be sent
	op_pk_nfd_set(pk_ptr,"Seg_Left",MAX_SIZE_ROUTE-2);
	
	// if it is the first propagating request of the sequence to be sent
	if ((timer==WAIT_NON_PROPA_REPLY_TIME)||(timer==0))
		{
		// the timer is set to its minimal value
		timer=WAIT_REPLY_MIN_TIME;
		// "request" statictics calculation
		if (timer!=0)
			{
			op_stat_write(stat_total_propa_requests,++total_propa_requests);
			op_stat_write(stat_total_requests,++total_requests);
			}
		}
	// if the propagating request has failed
	else
		{
		// a longer timer is calculated for the new one
		if (timer!=WAIT_REPLY_MAX_TIME)
			{
			reply_waiting_dist=op_dist_load("uniform",0.0,1.0);
			random=op_dist_outcome(reply_waiting_dist);
			timer=timer+random*timer;
			if (timer>WAIT_REPLY_MAX_TIME) timer=WAIT_REPLY_MAX_TIME;
			}
		
		// "request" statistic calculation
		op_stat_write(stat_total_renewed_propa_requests,++total_renewed_propa_requests);
		op_stat_write(stat_total_propa_requests,++total_propa_requests);
		op_stat_write(stat_total_requests,++total_requests);
		}
	sprintf(message,"I am sending a PROPA REQUEST to %d with the seq number %d, and I am going to wait %f s\n",destination_dsr_address,request_sequence_number+1,timer);
	dsr_message(message);
	}

// set the sequence number of the packet and increment the sequence number for the next packet
op_pk_nfd_set(pk_ptr,"Seq_number",request_sequence_number++);

// Store the features of the new request in the request_sent table and shedule an event associated with the timer
request_sent[destination_dsr_address].sequence_number=request_sequence_number-1;
request_sent[destination_dsr_address].scheduling_time = op_sim_time()+timer;
request_sent[destination_dsr_address].waiting_time = timer;
destination_dsr_address_ptr=(int*)op_prg_mem_alloc(sizeof(int));
*destination_dsr_address_ptr=destination_dsr_address;
request_sent[destination_dsr_address].evt = complex_intrpt_schedule_self(my_complex_intrpt_ptr,op_sim_time()+timer,NO_REPLY_CODE,destination_dsr_address_ptr);
//op_intrpt_schedule_remote(op_sim_time()+timer,NREPLY_CODE+dest_dsr_address,my_objid);

// for TR (Transmission Range) purpose
op_pk_nfd_set(pk_ptr,"TR_source",my_node_objid);

// since I am the request initiator I store the fact that I have already seen the request
dsr_request_already_seen(my_dsr_address,destination_dsr_address,request_sent[destination_dsr_address].sequence_number);
// send the request to the MAC layer which will BROADCAST it
dsr_send_to_mac(pk_ptr,BROADCAST);

// breakpoint for debugging purpose
if (1)
	{
	op_prg_odb_bkpt("transmit_request");
	}
}

/*************************************************************/
/*            DSR_TRANSMIT_REQUEST_FROM_ERROR                */
/*************************************************************/
/* This function build and send a request packet destined to the
/* specified destination after an error detection. Thus the non 
/* propagating request preceding a "standard" request will not 
/* be sent
/* 		int destination_dsr_address: the dsr address of the wanted 
/* destination
/*************************************************************/
void dsr_transmit_request_from_error(int destination_dsr_address)
{
// init the scheduling time to WAIT_NON_PROPA_REPLY_TIME in order to send directly a Propa request
request_sent[destination_dsr_address].scheduling_time=WAIT_NON_PROPA_REPLY_TIME;
dsr_transmit_request(destination_dsr_address);
// requests statistic calculation
op_stat_write(stat_total_requests_from_error,++total_requests_from_error);

// debug breakpoint purpose
if (1)
	{
	op_prg_odb_bkpt("transmit_request_from_error");
	}
}

/*************************************************************/
/*                 DSR_HANDLE_REQUEST                        */
/*************************************************************/
/* This function handle any request packet received by the node
/* 		Packet* pk_ptr: a pointer to the request packet to process
/*************************************************************/
void dsr_handle_request(Packet * pk_ptr)
{
int source_dsr_address;
int destination_dsr_address;
int sequence_number;
double creation_time;
int size_route;
int seg_left;

// read the different fields of the packet
op_pk_nfd_get (pk_ptr,"SRC",&source_dsr_address);
op_pk_nfd_get (pk_ptr,"DEST",&destination_dsr_address); 
op_pk_nfd_get (pk_ptr,"Seq_number",&sequence_number);
op_pk_nfd_get(pk_ptr,"Creation_Time",&creation_time); 
op_pk_nfd_get (pk_ptr,"Size_Route",&size_route); 
op_pk_nfd_get (pk_ptr,"Seg_Left",&seg_left);

sprintf(message,"I handle a request from %d, destined %d with sequence_number %d\n",source_dsr_address,destination_dsr_address,sequence_number);
dsr_message(message);

// if this request (identified by its source, destination and sequence_number) is received for the first time
if (!dsr_request_already_seen(source_dsr_address,destination_dsr_address,sequence_number))
	{
	// if the current node is the destination of the request
	if (my_dsr_address==destination_dsr_address)
		{
		// a reply packet must be built and sent
		dsr_transmit_reply_from_target(pk_ptr);
		}
	// otherwise if the node is not the destination
	else 
		{
    	// if a route to the destination is already known and stored in the Route_Cache
		if  (route_cache[destination_dsr_address].size_route>0)
			{
			// a reply (from relay) packet must be built and sent
			dsr_transmit_reply_from_relay(pk_ptr);
			}
		// otherwise if the request is not too hold (life time and number of hops)
		else if ((creation_time+REQUEST_LIFE_TIME>op_sim_time()) && (seg_left>0))
			{
			// the packet must be forwarded
			dsr_forward_request(pk_ptr);
			}
		// otherwise
		else
            {
			dsr_message("The request is too hold thus I am destroying the packet\n");
			// the packet must be destroyed
			op_pk_destroy(pk_ptr);
            }
		}
	}
// otherwise if the packet has been already seen it must not be processed in order to avoid loop or multiple similar replies
else
	{
	// thus it is destroyed
	op_pk_destroy(pk_ptr);
	}

// breakpoint for debugging purpose
if (1)
	{
	op_prg_odb_bkpt("handle_request");
	}
}

/*************************************************************/
/*               DSR_REQUEST_ALREADY_SEEN                    */
/*************************************************************/
/* This function checks if the request identified by its source,
/* its destination and its sequence_number has been already seen
/* and consequently handled
/* 		int source_dsr_address: the dsr address of the request 
/* source
/* 		int destination_dsr_address: the dsr address of the 
/* request destination
/* 		int sequence_number: the sequence number of the request
/*		RETURN: 1 if the request packet has been already seen 
/*				0 otherwise
/*************************************************************/
int dsr_request_already_seen(int source_dsr_address,int destination_dsr_address,int sequence_number)
{
// the request has been already seen since its sequence number is lower or equal than the one stored in memory
if (request_seen[source_dsr_address][destination_dsr_address]>=sequence_number)
	{
	dsr_message("I have already seen this request\n");
	return(1);
    }
else
	{
	// store the fact that the request has been already seen for the future by storing its sequence number
	request_seen[source_dsr_address][destination_dsr_address]=sequence_number;
	dsr_message("It is the first time that I am seing this request\n");
	return (0);
	}
}

/*************************************************************/
/*                  DSR_FORWARD_REQUEST                      */
/*************************************************************/
/* This function forward the request received by the node
/* 		Packet* pk_ptr: a pointer to the request packet to forward
/*************************************************************/
void dsr_forward_request(Packet* pk_ptr)
{
int size_route;
int seg_left;
char field[10];

// read the Size_Route and Seg_Left fields from the request packet
op_pk_nfd_get (pk_ptr,"Size_Route",&size_route); 
op_pk_nfd_get (pk_ptr,"Seg_Left",&seg_left);

// the current node adds its own dsr address in the request path
sprintf(field,"Node_%d",size_route);
op_pk_nfd_set (pk_ptr,field,my_dsr_address);

// decrement by one the Seg_left field of the packet
op_pk_nfd_set(pk_ptr,"Seg_Left",--seg_left);
// increment by one the Size_Route field of the packet
op_pk_nfd_set(pk_ptr,"Size_Route",++size_route);

// write the node_objid in the packet for the transmission range purpose
op_pk_nfd_set(pk_ptr,"TR_source",my_node_objid);

// send the request to the MAC layer which will BROADCAST it
dsr_send_to_mac(pk_ptr,BROADCAST);

dsr_message("I forward the request\n");

// breakpoint for debugging purpose
if (1)
	{
	op_prg_odb_bkpt("forward request");
	}
}

/*************************************************************/
/*            DSR_TRANSMIT_REPLY_FROM_TARGET                 */
/*************************************************************/
/* This function build and transmit a reply packet from target
/* (the node is the destination of the received request)
/* 		Packet* pk_ptr: a pointer to the request packet to reply
/*************************************************************/
void dsr_transmit_reply_from_target(Packet* pk_ptr)
{
Packet* reply_pk_ptr;
int request_source_dsr_address;
int node_dsr_address;
int relay_dsr_address;
int relay_mac_address;
char field[10];
int seg_left,size_route;
int sequence_number;
int i;

// create the reply packet and set its Type field
reply_pk_ptr = op_pk_create_fmt("Dsr_Reply");
op_pk_nfd_set(pk_ptr,"Type",REPLY_PACKET_TYPE);

// since I am the request target I am also the reply source
op_pk_nfd_set(reply_pk_ptr,"SRC",my_dsr_address);
// set the reply DEST field => the request source is the reply destination
op_pk_nfd_get(pk_ptr,"SRC",&request_source_dsr_address);
op_pk_nfd_set(reply_pk_ptr,"DEST",request_source_dsr_address);
// set the field in the packet indicating that it is a reply from target
op_pk_nfd_set(reply_pk_ptr,"Reply_From_Target",1);

// set the reply Seq_number field => same sequence number than the request
op_pk_nfd_get(pk_ptr,"Seq_number",&sequence_number);
op_pk_nfd_set(reply_pk_ptr,"Seq_number",sequence_number);

// Copy the fields "Node_i" from the request to the reply in the same order
op_pk_nfd_get(pk_ptr,"Size_Route",&size_route);
printf("Reply from Target route: ");
for (i=0;i<size_route;i++)
	{
	sprintf(field,"Node_%d",i);
	op_pk_nfd_get(pk_ptr,field,&node_dsr_address);
	op_pk_nfd_set(reply_pk_ptr,field,node_dsr_address);
	printf("%d...",node_dsr_address);	
	}
printf("%d...end\n",my_dsr_address);

// Assign last Node_i containing the request destination, that is me
sprintf(field,"Node_%d",i);
op_pk_nfd_set(reply_pk_ptr,field,my_dsr_address);
size_route++;


// Assign the first relay on the reply packet's route (-1 for the index of the last node of the route and another -1 for the previous one)
sprintf(field,"Node_%d",size_route-2);
op_pk_nfd_get(reply_pk_ptr,field,&relay_dsr_address);
op_pk_nfd_set(reply_pk_ptr,"RELAY",relay_dsr_address);

// update the seg_left and size route fields (-1 = number of hops in the route, anther -1 = segment left)
seg_left=size_route-2;
op_pk_nfd_set(reply_pk_ptr,"Seg_Left",seg_left);
op_pk_nfd_set(reply_pk_ptr,"Size_Route",size_route);

// set the TR_source fiels for the Transmission range purpose
op_pk_nfd_set(reply_pk_ptr,"TR_source",my_node_objid);

// since I am the reply transmitter I will not have to process it in the futur
dsr_reply_already_seen(my_dsr_address,request_source_dsr_address,sequence_number);

// convert the dsr address of the first relay in its mac address
relay_mac_address = dsr_support_get_mac_from_dsr_address(relay_dsr_address);
// and send the reply packet instantanetly since I am the target of the request
dsr_send_to_mac(reply_pk_ptr,relay_mac_address);

sprintf(message,"I am sending a reply from target to %d with sequence_number %d\n",request_source_dsr_address,sequence_number);
dsr_message(message);

// "replies" statistic calculation
op_stat_write(stat_total_replies_from_target,++total_replies_from_target);
op_stat_write(stat_total_replies,++total_replies);

// and destroy the request packet
op_pk_destroy(pk_ptr);

// breakpoints for debugging purpose
if (1)
	{
	op_prg_odb_bkpt("transmit_reply");
	op_prg_odb_bkpt("transmit_reply_from_target");
	}
}

/*************************************************************/
/*            DSR_TRANSMIT_REPLY_FROM_RELAY                  */
/*************************************************************/
/* This function build and transmit a reply packet from relay
/* (the node is not the destination of the received request, but
/* can reply by using its route cache)
/* 		Packet* pk_ptr: a pointer to the request packet to reply
/*************************************************************/
void dsr_transmit_reply_from_relay(Packet *pk_ptr)
{
Packet* reply_pk_ptr;
int request_source_dsr_address; 
int request_destination_dsr_address;
int relay_dsr_address;
int node_dsr_address;
char field[10];
int seg_left;
int size_route;
int sequence_number;
sRoute reply_route;
Distribution* delay_reply_dist;
double delay;
sReply* reply;
int* intPtr;
int i;

// read the Size_Route and Seg_Left fields from the request packet
op_pk_nfd_get(pk_ptr,"Size_Route",&size_route);
op_pk_nfd_get(pk_ptr,"DEST",&request_destination_dsr_address);

// Check if the pasted route's length will be still < MAX_SIZE_ROUTE length=H
if ((size_route+route_cache[request_destination_dsr_address].size_route)<MAX_SIZE_ROUTE)
	{
	// create the reply packet and set its Type field
	reply_pk_ptr = op_pk_create_fmt("Dsr_Reply");
	op_pk_nfd_set(reply_pk_ptr,"Type",REPLY_PACKET_TYPE);

	// assign the Source and Dest field to the reply packet
	op_pk_nfd_set(reply_pk_ptr,"SRC",request_destination_dsr_address);
	op_pk_nfd_get(pk_ptr,"SRC",&request_source_dsr_address);
	op_pk_nfd_set(reply_pk_ptr,"DEST",request_source_dsr_address);

	// set the field in the packet indicating that it is a reply from target
	op_pk_nfd_set(reply_pk_ptr,"Reply_From_Target",0);

	// set the reply Seq_number field => same sequence number than the request
	op_pk_nfd_get(pk_ptr,"Seq_number",&sequence_number);
	op_pk_nfd_set(reply_pk_ptr,"Seq_number",sequence_number);

	// copy the first part of the route coming from the request in the reply_route
	for (i=0;i<size_route;i++)
		{
		sprintf(field,"Node_%d",i);
		op_pk_nfd_get(pk_ptr,field,&node_dsr_address);
		reply_route.route[i]=node_dsr_address;
		}

	// copy the second part of the route coming from the route_cache in the reply_route
	for (i=size_route;i<size_route+route_cache[request_destination_dsr_address].size_route;i++)
		{
		reply_route.route[i]=route_cache[request_destination_dsr_address].route[i-size_route];
		}
	
	// calcul the size ot the reply_route
	reply_route.size_route=size_route+route_cache[request_destination_dsr_address].size_route;
	
	// clear every loop from the reply_route
	dsr_no_loop_in_route(&reply_route);
	
	// ckeck if the node is still in the path and find its position 
	i=0;
	do
		{
		i++;
		}
	while ((reply_route.route[i]!=my_dsr_address)&&(i<reply_route.size_route));
			
	// if the node is still there: it was not in a loop and the reply packet can be sent
	if (i<reply_route.size_route)
		{
		// assign the first relay on the reply packet's route
		relay_dsr_address=reply_route.route[i-1];
		sprintf(field,"Node_%d",i-1);
		op_pk_nfd_set(reply_pk_ptr,"RELAY",relay_dsr_address);
				
		// set the Seg_Left field of the reply
		seg_left=i-1;
		op_pk_nfd_set(reply_pk_ptr,"Seg_Left",seg_left);
		
		printf("Reply from Relay route: ");
		// copy the reply_route in the Node_i fields of the packet
		for (i=0;i<reply_route.size_route;i++)
			{
			printf("%d...",reply_route.route[i]);
			sprintf(field,"Node_%d",i);
			op_pk_nfd_set(reply_pk_ptr,field,reply_route.route[i]);
			}
		printf("end\n");
		
		// set the Size_Route field of the reply
		op_pk_nfd_set(reply_pk_ptr,"Size_Route",reply_route.size_route);
		
		// set the TR_source fiels for the Transmission range purpose
		op_pk_nfd_set(reply_pk_ptr,"TR_source",my_node_objid);
		
		// since I am the reply transmitter I will not have to process it in the futur
		dsr_reply_already_seen(my_dsr_address,request_source_dsr_address,sequence_number);
			
		// init the delay_reply distribution
		delay_reply_dist=op_dist_load("uniform",0.0,1.0);
		// calcul the delay that the node must wait before sending its reply
		delay=op_dist_outcome(delay_reply_dist);
		delay=HOP_DELAY*((double)(reply_route.size_route-1)+delay);
		
		// prepare the code (destination and source coded in one integer) associated (in the fifo) with the reply information 
		intPtr=(int*)op_prg_mem_alloc(sizeof(int));
		*intPtr=request_destination_dsr_address*OPTION+request_source_dsr_address;
		
		// check if a reply for the same route is still scheduled by looking in the fifo for its information
		reply=fifo_multiplex_extract(&reply_fifo,*intPtr);
		// if some information about a scheduled reply are found
		if (reply!=NULL)
			{
			// cancel the intrpt associated with this previous scheduled reply 
			if (complex_intrpt_cancel(my_complex_intrpt_ptr,reply->evt))
				{
				// "replies" statistic calculation if success
				op_stat_write(stat_total_canceled_replies,++total_canceled_replies);
				}
			else
				{
				// error since the intrpt must be valid and active
				op_sim_end("Amazing error: reply should be valid and active","","","");
				}
			// destroy the information about this reply 
			op_pk_destroy(reply->pk);
			op_prg_mem_free(reply);
			}

		// memory allocation for structure containing information about the new reply
		reply=(sReply*)op_prg_mem_alloc(sizeof(sReply));
		// store in it the reply packet
		reply->pk=reply_pk_ptr;
		// and its sequence number
		reply->sequence_number=sequence_number;
		// schedule an intrpt that will "say" when to send the reply and store the associated event in the reply information structure 
		reply->evt=complex_intrpt_schedule_self(my_complex_intrpt_ptr,op_sim_time()+delay,SEND_REPLY_CODE,intPtr);
		// save the scheduled reply in a multiplexed Fifo
		fifo_multiplex_insert(&reply_fifo,reply,*intPtr);
		
		sprintf(message,"I plan to send a reply to %d for the route reaching %d in %f seconds (size_route=%d)\n",request_source_dsr_address,request_destination_dsr_address,delay,reply_route.size_route);
		dsr_message(message);
		
		// "replies" statistic calculation
		op_stat_write(stat_total_replies_from_relay,++total_replies_from_relay);
		op_stat_write(stat_total_replies,++total_replies);
		}
	// if the current node was in a loop, its dsr address is not the reply_route anymore so the packet connot be sent back to the request source
	else
		{
		// so the reply packet is destroyed
		op_pk_destroy(reply_pk_ptr);
		}
	}

// destroy the request packet
op_pk_destroy(pk_ptr);

// breakpoints for debugging purpose
if (1)
	{
	op_prg_odb_bkpt("transmit_reply");
	op_prg_odb_bkpt("transmit_reply_from_relay");
	}

}

/*************************************************************/
/*                    DSR_HANDLE_REPLY                       */
/*************************************************************/
/* This function handle any reply packet received by the node
/* 		Packet* pk_ptr: a pointer to the request packet to process
/*************************************************************/
void  dsr_handle_reply(Packet* pk_ptr)
{
int source_dsr_address;
int destination_dsr_address;
int sequence_number;
int relay_dsr_address;
int relay_mac_address;
double creation_time;
int size_route;
int seg_left;
int node_dsr_address;
int current_node_index;
Packet* data_pk_ptr;
int from_target;
char field[10];
int i;

// extract usefull information from the reply packet
op_pk_nfd_get (pk_ptr,"SRC",&source_dsr_address);
op_pk_nfd_get (pk_ptr,"DEST",&destination_dsr_address); 
op_pk_nfd_get (pk_ptr,"RELAY",&relay_dsr_address);
op_pk_nfd_get (pk_ptr,"Seq_number",&sequence_number);
op_pk_nfd_get (pk_ptr,"Reply_From_Target",&from_target);
op_pk_nfd_get(pk_ptr,"Size_Route",&size_route);
op_pk_nfd_get(pk_ptr,"Seg_Left",&seg_left);

// if the reply id received for the first time or of it is a gratuitous reply
if ((!dsr_reply_already_seen(source_dsr_address,destination_dsr_address,sequence_number))||(sequence_number==-1))
	{
	// if the reply packet is destinated to the current node
	if (my_dsr_address==destination_dsr_address)
		{
		// insert the route contained in the reply in the node route_cache
		dsr_insert_route_in_cache(pk_ptr);
		    			
		// if the reply is not a gratuitous reply
		if (sequence_number!=-1)
			{
			// if no other reply (with an inferior sequence number) for the same requests sequence was received before
			if (request_sent[source_dsr_address].waiting_time!=0)
				{
				// cancel the intrpt associated with this request expiration time
				if (complex_intrpt_cancel(my_complex_intrpt_ptr,request_sent[source_dsr_address].evt))
					{
					//printf("well done for the path !\n");
					}
				else
					{
					// error since the intrpt must be valid and active
					op_sim_end("Amazing error: request intrpt should be valid and active","","","");
					//op_stat_write(stat_amazing_errors,++amazing_errors);
					}
				
				// re-init the request_sent table since the reply has been received
				request_sent[source_dsr_address].scheduling_time = 0.0;
				request_sent[source_dsr_address].waiting_time = 0.0;
				}
	
			// try to extract the first packet to send on the new route (=> packet destined to the reply source)
			data_pk_ptr=dsr_extract_buffer(source_dsr_address);
			// if a packet was found in the buffer
			if (data_pk_ptr!=OPC_NIL)
				{
				dsr_transmit_data(data_pk_ptr,source_dsr_address);
				}
			}
		// destroy the reply packet
		op_pk_destroy(pk_ptr);
		}
	// if the node is the relay of the reply packet
	else if (my_dsr_address==relay_dsr_address)
		{
		// insert a truncated part of the route contained in the reply in its route_cache
		dsr_insert_route_in_cache(pk_ptr);
		/* ##### serve buffer */
		/* forward reply */
		dsr_forward_reply(pk_ptr);
		}	
	// if we are not concerned by the reply packet
	else if (sequence_number!=-1)
		{
		// if the promiscuous reply mode is activated
		dsr_promiscuous_reply(pk_ptr);
		// destroy the reply packet
		i=0;
		current_node_index=-1;
		// check if the current node is in the reply path
		do
			{
			sprintf(field,"Node_%d",i);
			op_pk_nfd_get(pk_ptr,field,&node_dsr_address);
			if (node_dsr_address==my_dsr_address)
				{
				current_node_index=i;
				}
			else 
				{ 
				i++;
				}
			}
		while ((current_node_index==-1)&&(i<seg_left));
		// if the node is in the path then there is a shorster path than the one used by the reply packet
		if (current_node_index!=-1)
			{
			// "calculate" and set the reply packet route with this shorter path
			for (i=1;i<size_route-seg_left;i++)
				{
				sprintf(field,"Node_%d",i+seg_left);
				op_pk_nfd_get(pk_ptr,field,&node_dsr_address);
				sprintf(field,"Node_%d",i+current_node_index);
				op_pk_nfd_set(pk_ptr,field,node_dsr_address);
				}
			// and update the Size_Route and Seg_left fields of the reply packet
			op_pk_nfd_set(pk_ptr,"Size_Route",size_route-(seg_left-current_node_index));
			op_pk_nfd_set(pk_ptr,"Seg_Left",current_node_index);
			dsr_message("I have just found a shorter path for the reply\n");
			dsr_forward_reply(pk_ptr);
			}
		else
			{
			dsr_message("I am not concerned by the reply thus I am destroying it\n");
			op_pk_destroy(pk_ptr);
			}
		}
	}
// if the reply has already been received by the node
else
	{
	// it is destroyed
	op_pk_destroy(pk_ptr);
	}

// breakpoints for debugging purpose
if (1)
	{
	op_prg_odb_bkpt("handle_reply");
   	}
if (sequence_number==-1)
	{
	op_prg_odb_bkpt("handle_gratuitous_reply");
	}
}

/*************************************************************/
/*                 DSR_REPLY_ALREADY_SEEN                    */
/*************************************************************/
/* This function checks if the reply identified by its source,
/* its destination and its sequence_number has been already seen
/* and consequently handled
/* 		int source_dsr_address: the dsr address of the reply 
/* source
/* 		int destination_dsr_address: the dsr address of the 
/* reply destination
/* 		int sequence_number: the sequence number of the reply
/*		RETURN: 1 if the reply packet has been already seen 
/*				0 otherwise or is it is a gratuitous reply
/*************************************************************/
int dsr_reply_already_seen(int source_dsr_address,int destination_dsr_address,int sequence_number)
{
// the reply has been already seen since its sequence number is lower or equal than the one stored in memory
if (reply_seen[source_dsr_address][destination_dsr_address]>=sequence_number)
	{
	if (sequence_number!=-1) 
		{
		dsr_message("I have already seen this reply\n");
		}
	else
		{
		dsr_message("It is a gratuitous reply\n");
		}
	return(1);
    }
else
	{
	// store the fact that the reply has been already seen for the future by storing its sequence number
	reply_seen[source_dsr_address][destination_dsr_address]=sequence_number;
	dsr_message("It is the first time that I am seeing this reply\n");
	return (0);
	}
}

/*************************************************************/
/*                    DSR_FORWARD_REPLY                      */
/*************************************************************/
/* This function forward the reply packet received by the node
/* 		Packet* pk_ptr: a pointer to the reply packet to forward
/*************************************************************/
void dsr_forward_reply(Packet* pk_ptr)
{
int seg_left;
char field[10];
int relay_dsr_address;
int relay_mac_address;

// read, then decrement seg_left by one since the transmission on the previous hop was successfull, and update its value in the reply packet
op_pk_nfd_get (pk_ptr,"Seg_Left",&seg_left);
seg_left--;
op_pk_nfd_set(pk_ptr,"Seg_Left",seg_left);

// extract the dsr address of the next relay from the packet route
sprintf(field,"Node_%d",seg_left);
op_pk_nfd_get (pk_ptr,field,&relay_dsr_address);
// and write it in the RELAY field
op_pk_nfd_set(pk_ptr,"RELAY",relay_dsr_address);

// set the TR_source field for the Transmission Range purpose
op_pk_nfd_set(pk_ptr,"TR_source",my_node_objid);

// convert the dsr address of the next relay in its mac address
relay_mac_address=dsr_support_get_mac_from_dsr_address(relay_dsr_address);
// and send the packet
dsr_send_to_mac(pk_ptr,relay_mac_address);

sprintf(message,"I am forwarding a reply packet to %d\n",relay_dsr_address);
dsr_message(message);

// breakpoint for debugging purpose
if (1)
	{
	op_prg_odb_bkpt("forward_reply");
	}
}

/*************************************************************/
/*                  DSR_INSERT_ROUTE_IN_CACHE                */
/*************************************************************/
/* This function insert every routes (leading to the request 
/* destination and to the intermediate nodes) coming from a reply 
/* packet in the node cache of the node
/* 		Packet* pk_ptr: a pointer to the reply packet from which 
/* the route is extracted
/*************************************************************/
void dsr_insert_route_in_cache(Packet* pk_ptr)
{
int destination_dsr_address;
int node_dsr_address;
int current_node_index;
int seg_left;
char field[10];
int size_route;
int node_listed;
int i,j;

// extract some usefull information from the reply packet
op_pk_nfd_get(pk_ptr,"Size_Route",&size_route);
op_pk_nfd_get(pk_ptr,"Seg_Left",&seg_left);

i=0;
current_node_index=-1;
// find the current node index in the path
do
	{
	sprintf(field,"Node_%d",i);
	op_pk_nfd_get(pk_ptr,field,&node_dsr_address);
	if (node_dsr_address==my_dsr_address)
		{
		current_node_index=i;
		}
	else 
		{ 
		i++;
		}
	}
while ((current_node_index==-1)&&(i<size_route));

// if the current was found in the path
if (current_node_index!=-1)
	{
	// copy every routes whitin the current node and the reply source node in the route cache
	for (i=seg_left+1;i<size_route;i++)
		{
		// extract the address of each intermediate node who will be the path destination
		sprintf(field,"Node_%d",i);
		op_pk_nfd_get(pk_ptr,field,&destination_dsr_address);
		// copy the current node dsr_address as the first node of the path
		sprintf(field,"Node_%d",current_node_index);
		op_pk_nfd_get(pk_ptr,field,&node_dsr_address);
		if (node_dsr_address!=my_dsr_address)
			{
			op_sim_end("Amazing error: the first address of the route should be mine","","","");
			}
		else
			{
			route_cache[destination_dsr_address].route[0]=node_dsr_address;
			}
		// and copy the route from the current node to destination internediate node in the route_cache
		for (j=1;j<=(i-seg_left);j++)
			{
			sprintf(field,"Node_%d",j+seg_left);
			op_pk_nfd_get(pk_ptr,field,&node_dsr_address);
			route_cache[destination_dsr_address].route[j]=node_dsr_address;
			}
		// write in the route cache the size of this route (number of hops + 1)
		route_cache[destination_dsr_address].size_route=i-seg_left+1;
		}
	// display the new route cache on the sreen
	dsr_message("I have just updated my route cache so...\n");
	dsr_message("************* Here is my cache ****************\n");
	for (i=0;i<NUMBER_OF_NODES;i++)
		{
		size_route=route_cache[i].size_route;
		printf("Route to %d with size %d: ",i,size_route);
		for (j=0;j<size_route;j++)
			{
			printf("%d...",route_cache[i].route[j]);
			}
		printf("end\n");
		}
	dsr_message("***********************************************\n");
	}
// otherwise the current node was not found in the path
else
	{
	// the simulation is stopped since it should be in the path
	op_sim_end("Amazing error: the current node is not in the reply path","","","");
	}

// breakpoint for debugging purpose
if (1)
	{
	op_prg_odb_bkpt("insert_route_in_cache");
	}

}

/*************************************************************/
/*                 DSR_PROMISCUOUS_REPLY                     */
/*************************************************************/
/* This function handle the replies that are not destinated to 
/* the node (neither destination nor relay). It checks that no
/* similar reply (replying to the same request) is scheduled, 
/* and eventually cancel it since it is not usefull anymore
/* 		Packet* pk_ptr: a pointer to the reply packet to process
/*************************************************************/
void dsr_promiscuous_reply(Packet* pk_ptr)
{
int reply_source_dsr_address;
int reply_destination_dsr_address;
int relay_dsr_address;
int sequence_number;
int seg_left;
char field[10];
int i;
sReply* reply;
int code;

// extract some usefull information from the packet
op_pk_nfd_get (pk_ptr,"SRC",&reply_source_dsr_address);
op_pk_nfd_get (pk_ptr,"DEST",&reply_destination_dsr_address);
op_pk_nfd_get(pk_ptr,"Seg_Left",&seg_left);
op_pk_nfd_get (pk_ptr,"Seq_number",&sequence_number);
sprintf(field,"Node_%d",seg_left+1);
op_pk_nfd_get(pk_ptr,field,&relay_dsr_address);

// prepare the code (destination and source coded in one integer) associated (in the fifo) with the reply information 
code=reply_source_dsr_address*OPTION+reply_destination_dsr_address;
// extract an eventual scheduled reply from the multiplexed Fifo
reply=fifo_multiplex_extract(&reply_fifo,code);
// if a scheduled reply was found
if (reply!=NULL)
	{
	// if it is for an older request than the new one
	if ((reply->sequence_number)<=sequence_number)
		{
		
		// cancel the intrpt associated with this previous scheduled reply 
		if (complex_intrpt_cancel(my_complex_intrpt_ptr,reply->evt))
			{
			sprintf(message,"Cancelation of the scheduled reply from %d to %d with sequence number %d\n",reply_source_dsr_address,reply_destination_dsr_address,sequence_number);
			dsr_message(message);
			// "replies" statistic calculation if success
			op_stat_write(stat_total_canceled_replies,++total_canceled_replies);
			}
		else
			{
			// error since the intrpt must be valid and active
			op_sim_end("Amazing error: reply should be valid and active","","","");
			}
		op_pk_destroy(reply->pk);
		op_prg_mem_free(reply);
		}
	// if the reply was for a more recent request then it is not canceled
	else
		{
		sprintf(message,"My reply from %d to %d is more recent, thus I will send it\n",reply_source_dsr_address,reply_destination_dsr_address);
		fifo_multiplex_insert(&reply_fifo,reply,code);
		}
	}
else
	{
	sprintf(message,"I have no scheduled from %d to %d reply that are waiting\n",reply_source_dsr_address,reply_destination_dsr_address);
	dsr_message(message);
	}

// breakpoint for debugging purpose
if (1)
	{
	op_prg_odb_bkpt("promiscuous_reply");
	}
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////// DATA TRANSMISSION FUNCTIONS /////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

/*************************************************************/
/*               DSR_UPPER_LAYER_ARRIVAL                     */
/*************************************************************/
/* This function handle the data coming from the upper layer.
/* 		Packet* data_pk_ptr: a pointer to the upper layer data 
/* packet
/* 		int destination_dsr_address: the dsr address of the upper
/* layer data destination
/*************************************************************/
void dsr_upper_layer_data_arrival (Packet* data_pk_ptr, int destination_dsr_address)
{
int size_route;
Ici * ici_dest_address;
Packet* pk_ptr;

// Create the data packet wich is going to transport the upper layer packet, and set its Type field
pk_ptr=op_pk_create_fmt("Dsr_Data");
op_pk_nfd_set(pk_ptr,"Type",DATA_PACKET_TYPE);

// insert the upper layer data in the dsr data packet
op_pk_nfd_set (pk_ptr, "data", data_pk_ptr);

// extract the size of the route contained in the route cache (if size=0 there is no route)
size_route=route_cache[destination_dsr_address].size_route;

// if there is a route to the destination and no previous data packet in the buffer to serve 
if ((size_route!=0) && (dsr_buffer_empty(destination_dsr_address)))
	{
	// the packet is transmitted now
	dsr_transmit_data(pk_ptr,destination_dsr_address);
	}
// if there is already a packet in the buffer to serve in priority
else if ((size_route!=0) && (!dsr_buffer_empty(destination_dsr_address)))
	{
	// store the new packet at the end of the destination transmission buffer
	dsr_insert_buffer(pk_ptr,destination_dsr_address);
	}
// if there is no route to the destination in the route cache
else
	{
	// if no request has been sent yet to this destination
	if (request_sent[destination_dsr_address].waiting_time==0)
		{
		// transmit a request to this destination
		dsr_transmit_request(destination_dsr_address);
		// store the new packet at the end of the destination transmission buffer waiting for the reply
		dsr_insert_buffer(pk_ptr,destination_dsr_address);
		}
	else
		{
		// store the new packet at the end of the destination transmission buffer waiting for the reply
		dsr_insert_buffer(pk_ptr,destination_dsr_address);
		}
	}

// breakpoint for debugging purpose
if (1)
	{
	op_prg_odb_bkpt("upper_layer_data_arrival");
	}

}

/*************************************************************/
/*                 DSR_TRANSMIT_DATA                         */
/*************************************************************/
/* This function fills in and sends the data packet destined to 
/* the specified destination
/* 		Packet* pk_ptr: a pointer to the data packet to fill in
/* 		int destination_dsr_address: the dsr address of the data 
/* packet destination
/*************************************************************/
void dsr_transmit_data(Packet* pk_ptr, int destination_dsr_address)
{
int size_route;
int relay_mac_address;
char field[10];
int i;

// set the DEST field of the packet with this mac address
op_pk_nfd_set(pk_ptr,"DEST",destination_dsr_address);

// set the source field of this packet (I am the source)
op_pk_nfd_set(pk_ptr,"SRC",my_dsr_address);

// extract the size of the route contained in the route cache
size_route=route_cache[destination_dsr_address].size_route;
// set the seg_left and size_route fields
op_pk_nfd_set(pk_ptr,"Size_Route",size_route);
op_pk_nfd_set(pk_ptr,"Seg_Left",size_route-2);

// write the route of the packet in the Node_k fields
for (i=0;i<size_route;i++)
	{
	sprintf(field,"Node_%d",i);
	op_pk_nfd_set(pk_ptr,field,route_cache[destination_dsr_address].route[i]);
	}

// set the RELAY field of the packet (the first relay is the first node of the route)
op_pk_nfd_set(pk_ptr,"RELAY",route_cache[destination_dsr_address].route[1]);

// convert this relay dsr address in a mac address via the dsr support package
relay_mac_address=dsr_support_get_mac_from_dsr_address(route_cache[destination_dsr_address].route[1]);

// data packet transmitted thus already processed by the node
dsr_data_already_seen(packet_id);
// set the packet id of the packet
op_pk_nfd_set (pk_ptr,"packet_ID",packet_id++);

// set the TR_source field for the TR purpose
op_pk_nfd_set(pk_ptr,"TR_source",my_node_objid);

// schedule an event if no ack received within a waiting window
dsr_schedule_no_ack_event(pk_ptr);
// "data" statistic calculation
op_stat_write(stat_total_data_served,++total_data_served);
dsr_send_to_mac(pk_ptr,relay_mac_address);

sprintf(message,"I am sending the data packet for %d to %d\n",destination_dsr_address,route_cache[destination_dsr_address].route[1]);
dsr_message(message);
// breakpoint for debugging purpose
if (1)
	{
	op_prg_odb_bkpt("transmit_data");
	}
}

/*************************************************************/
/*                   DSR_HANDLE_DATA                         */
/*************************************************************/
/* This function handle any data packet received by the node
/* 		Packet* pk_ptr: a pointer to the data packet to process
/*************************************************************/
void dsr_handle_data(Packet* pk_ptr)
{
int source_dsr_address;
int destination_dsr_address;
int relay_dsr_address;
int sequence_number;
int current_node_index;
int size_route;
Objid trsource;
int seg_left;
int pk_id;
Packet* data_pk_ptr;

// extract some usefull information from the data packet
op_pk_nfd_get (pk_ptr,"SRC",&source_dsr_address);
op_pk_nfd_get (pk_ptr,"DEST",&destination_dsr_address); 
op_pk_nfd_get (pk_ptr,"RELAY",&relay_dsr_address);
op_pk_nfd_get (pk_ptr,"TR_source",&trsource);
op_pk_nfd_get (pk_ptr,"Seq_number",&sequence_number);
op_pk_nfd_get(pk_ptr,"Size_Route",&size_route);

// check via the packet id if the data packet has already been received
op_pk_nfd_get (pk_ptr,"packet_ID",&pk_id);

// if the data packet is received for the first time
if (!dsr_data_already_seen(pk_id))
	{
	// if the current node is the destination and the final relay of the packet
	if ((my_dsr_address==destination_dsr_address)&&(my_dsr_address==relay_dsr_address))
		{
		// if the dsr layer has to send explicit ack
		if (!MAC_LAYER_ACK)
			{
			// simulation is stopped since it is not implemented in this version
			op_sim_end("Explicit Dsr Ack are not supported in this dsr model version","","","");
			}
		dsr_message("The data packet is for me, thus I transmit it to my upper layer\n");
		// extract the upper layer data in the dsr data packet
		op_pk_nfd_get (pk_ptr, "data", &data_pk_ptr);
		// send the data to the upper layer
		op_pk_send(data_pk_ptr,TO_UPPER_LAYER_STRM); 
		// destroy the dsr data packet
		op_pk_destroy(pk_ptr);
		
		// "data" statistic calculation
		op_stat_write(stat_total_data_successfully_transmitted,++total_data_successfully_transmitted);
		op_stat_write(stat_total_data_efficiency,(double)total_data_successfully_transmitted/(double)total_data_served);
		}
	// if the current node is the destination but not the last relay
	else if ((my_dsr_address==destination_dsr_address)&&(my_dsr_address!=relay_dsr_address))
		{
		// a gratuitous reply need to be sent
		dsr_transmit_gratuitous_reply(size_route-1,pk_ptr);
		// ckeck_gratuitous_reply(pk_ptr);
		dsr_message("The data packet is almost for me, thus I transmit it to my upper layer\n");
		// extract the upper layer data in the dsr data packet
		op_pk_nfd_get (pk_ptr, "data", &data_pk_ptr);
		// send the data to the upper layer
		op_pk_send(data_pk_ptr,TO_UPPER_LAYER_STRM); 
		// destroy the dsr data packet
		op_pk_destroy(pk_ptr);
		
		// "data" statistic calculation
		op_stat_write(stat_total_data_successfully_transmitted,++total_data_successfully_transmitted);
		op_stat_write(stat_total_data_efficiency,(double)total_data_successfully_transmitted/(double)total_data_served);
		op_stat_write(stat_mean_hops_by_route,(double)total_ack/(double)total_data_served);
		}
	
	// if the current node is the relay of the packet 
	else if ((my_dsr_address==relay_dsr_address)&&(my_dsr_address!=destination_dsr_address))  
		{
		// forward the packet
		dsr_forward_data(pk_ptr);		
		}
	
	// if the current is neither the destination nor the relay of the packet
	else if ((my_dsr_address!=destination_dsr_address)&&(my_dsr_address!=relay_dsr_address)) 
		{
		dsr_message("I am not concerned by the data packet\n");
		op_pk_nfd_get (pk_ptr,"Seg_Left",&seg_left);
		// if the packet needs to be still forwarded
		if (seg_left>0)	
			{
			// if a gratuitous reply need to be sent
			if (current_node_index=dsr_ckeck_gratuitous_reply(pk_ptr))
			 	{
				dsr_transmit_gratuitous_reply(current_node_index,pk_ptr);
				// the packet route has been updated by the check_gratuitous_reply function, thus the packet just need to be forwarded
				dsr_forward_data(pk_ptr);
	
				dsr_message("I am sending a gratuitous reply and relaying the data packet\n");
				}
			//if no gratuitous reply need to be sent
			else
				{	
				// destroy the data packet
				op_pk_destroy(pk_ptr);
				}
			}
		// if the packet do not need to be forwarded
		else
			{
			// it is destroyed
			op_pk_destroy(pk_ptr);
			}
		}
	}
// if the packet has already been received and thus processed by the current node
else 
	{
	// then it is destroyed
	op_pk_destroy(pk_ptr);
	}

// breakpoint for debugging purpose
if (1)
	{
	op_prg_odb_bkpt("handle_data");
	}

}

/*************************************************************/
/*               DSR_SCHEDULE_NO_ACK_EVENT                   */
/*************************************************************/
/* This function schedule an "no ack reception" event associated
/* with a data packet that will be transmitted. It also associated 
/* some information in a structure, in order to be able to send an 
/* error packet if the link on which the data packet is transmitted
/* is brocken
/* 		Packet* pk_ptr: the data packet that will be transmitted
/*************************************************************/
void dsr_schedule_no_ack_event(Packet* pk_ptr)
{
sNoAck* no_ack;
char field[10];
int node_dsr_address; 
int relay_dsr_address;
int i;

// alloc the memory for the sNoAck structure that will contain the information about the no ack event
no_ack=(sNoAck*)op_prg_mem_alloc(sizeof(sNoAck));

// if there was no problem during this memory allocation
if (no_ack!=OPC_NIL)
	{
	// schedule the no_ack complex intrpt event and store it in the sNoAck structure
	no_ack->evt=complex_intrpt_schedule_self(my_complex_intrpt_ptr,op_sim_time()+WAIT_ACK_TIME,SELF_ERROR_CODE,OPC_NIL);
	
	// and store many other information (packet route, packet id...) in this structure
	no_ack->schedule=op_sim_time()+WAIT_ACK_TIME;
	op_pk_nfd_get(pk_ptr,"packet_ID",&(no_ack->packet_id));
	op_pk_nfd_get(pk_ptr,"Size_Route",&(no_ack->route.size_route));
	for (i=0;i<no_ack->route.size_route;i++)
		{
		sprintf(field,"Node_%d",i);
		op_pk_nfd_get(pk_ptr,field,&node_dsr_address);
		no_ack->route.route[i]=node_dsr_address;
		}
	
	op_pk_nfd_get(pk_ptr,"RELAY",&relay_dsr_address);
	// if the next relay dsr address is invalid 
	if ((relay_dsr_address<0)||(NUMBER_OF_NODES<=relay_dsr_address))
		{
		// the simulation is stopped since the packet can not be transmitted and this error is "amazing"
		sprintf(message,"Amazing error: invalid address (%d) for the data packet relay",relay_dsr_address);
		op_sim_end(message,"","","");
		}
	// otherwise the next relay dsr address is valid
	else
		{
		// and the sNoAck structure is store in a fifo
		fifo_multiplex_insert(&no_ack_fifo,no_ack,relay_dsr_address);
		}
	}
else
	{
	op_sim_end("no enought memory for no_ack_event structure allocation !","","","");
	}
}


/*************************************************************/
/*                DSR_DATA_ALREADY_SEEN                      */
/*************************************************************/
/* This function checks if the data packet identified by its 
/* packet_id has been already seen and consequently handled
/* 		int pk_id: the packet_id of tha data packet
/*		RETURN: 1 if the data packet has been already seen 
/*				0 otherwise
/*************************************************************/
int dsr_data_already_seen(int pk_id)
{
double* time;
int tmp_int; 
int already_received;

// try to find the time when this packet was previously received
time=fifo_multiplex_extract(&received_packet_id_fifo,pk_id);

// if this time is unfoundable, it is the first time that the packet is received
if (time==OPC_NIL)
	{
	// save its reception time indexed by its pk_id in the received_packet_id_fifo to indicate in the future that this packet has been already received
	time=(double *)malloc(sizeof(double));
	*time=op_sim_time();
	fifo_multiplex_insert(&received_packet_id_fifo,time,pk_id);
	// set the already_received variable to TRUE
	already_received=0;
	dsr_message("It is the first time that I am seeing this data packet\n");
	}
// otherwise the packet has been already received
else
	{
	// save its new receival time in the received_packet_id_fifo
	*time=op_sim_time();
	fifo_multiplex_insert(&received_packet_id_fifo,time,pk_id);
	// set the already_received variable to TRUE
	already_received=1;
	dsr_message("I have already seen this data packet\n");
	}

// delete every too old packet ids from the received_packet_id_fifo
time=OPC_NIL;
do
	{
	// free the memory used by the last extracted packet id
	if (time!=OPC_NIL) free(time);
	// read the oldest packet id reception time in the fifo
	time=fifo_multiplex_read_first(&received_packet_id_fifo,&tmp_int);
	// if this time is older than the life time of the data packet
	if ((time!=OPC_NIL)&&((*time+DATA_LIFE_TIME)<op_sim_time()))
		{
		// extract the time associated with the packet id from the fifo
		time=fifo_multiplex_extract_first(&received_packet_id_fifo,&tmp_int);
		}
	}
while ((time!=OPC_NIL)&&((*time+DATA_LIFE_TIME)<op_sim_time()));

return (already_received);
}


/*************************************************************/
/*                   DSR_FORWARD_DATA                         */
/*************************************************************/
/* This function forward the data packet received by the node
/* 		Packet* pk_ptr: a pointer to the data packet to forward
/*************************************************************/
void dsr_forward_data(Packet* pk_ptr)
{
int size_route;
int seg_left;
int next_relay_dsr_address;
int next_relay_mac_address;
char field[10];

// read the seg_left and the size_route field in the packet
op_pk_nfd_get (pk_ptr,"Seg_Left",&seg_left);
op_pk_nfd_get (pk_ptr,"Size_Route",&size_route);

// extract the next node dsr address from the packet (node size_route-seg_left)
sprintf(field,"Node_%d",size_route-seg_left);
op_pk_nfd_get(pk_ptr,field,&next_relay_dsr_address);
// and set it as the next packet relay
op_pk_nfd_set(pk_ptr,"RELAY",next_relay_dsr_address);

// update the seg_left field of the packet
op_pk_nfd_set(pk_ptr,"Seg_Left",--seg_left);

// set the TR_source field for the Transmission Range purpose
op_pk_nfd_set(pk_ptr,"TR_source",my_node_objid);

// schedule an event if no ack received within the waiting window
dsr_schedule_no_ack_event(pk_ptr);

// convert the dsr address of the next relay in its mac address
next_relay_mac_address=dsr_support_get_mac_from_dsr_address(next_relay_dsr_address);

// and send the packet
dsr_send_to_mac(pk_ptr,next_relay_mac_address);

sprintf(message,"I am forwarding the data packet to %d\n",next_relay_dsr_address);
dsr_message(message);

// breakpoint for debugging purpose
if (1)
	{
	op_prg_odb_bkpt("forward_data");
	}
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
////////////// ROUTE MAINTENANCE FUNCTIONS ////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

/*************************************************************/
/*                 DSR_TRANSMIT_ERROR                        */
/*************************************************************/
/* This function build and send an error packet destined to the
/* specified destination
/* 		sRoute route: the route to the error packet destination
/* 		int error_node_dsr_address: the address of the node with 
/* whom the communication link is broken (link from myself to 
/* him is broken)
/*************************************************************/
void dsr_transmit_error(sRoute route, int error_node_dsr_address)
{
Packet* pk_ptr;
int unreachable_node_dsr_address;
char field[10];
int current_node_index;
int relay_mac_address;
int i;

// create the error packet and init its Type field
pk_ptr=op_pk_create_fmt("Dsr_Error");
op_pk_nfd_set(pk_ptr,"Type",ERROR_PACKET_TYPE);

// set the error packet DEST field: it is the first node (source) of the route followed by the data packet
op_pk_nfd_set(pk_ptr,"DEST",route.route[0]);
// set the error packet SRC field: I am the source of this packet
op_pk_nfd_set(pk_ptr,"SRC",my_dsr_address);

// set the error packet Unreachable_Node field: it is the last node (destination) of the route followed bu the data packet
unreachable_node_dsr_address=route.route[route.size_route-1];
op_pk_nfd_set(pk_ptr,"Unreached_Node",unreachable_node_dsr_address);

// set the error packet PbNode field: it is the error_node_dsr_address (link between me and that node is broken)
op_pk_nfd_set(pk_ptr,"PbNode",error_node_dsr_address);

// find the index of the current node in the route (I am the node preceding the error_node_dsr_address)
i=0;
current_node_index=-1;
do
	{
	if (route.route[i+1]==error_node_dsr_address)
		{
		current_node_index=i;
		}
	else 
		{
		i++;
		}
	}
while ((current_node_index==-1)&&(i<MAX_SIZE_ROUTE));


// check that the current node is in the route followed by the data packet
if (current_node_index!=-1)
	{
	if (route.route[current_node_index]!=my_dsr_address)
		{
		op_sim_end("I should be the current node !!\n","","","");
		}
	
	// set the error packet Size_Route field
	op_pk_nfd_set(pk_ptr,"Size_Route",current_node_index+1);
	
	printf("Error route: ");
	// set the route (Node_i fields) of the error packet -> current_node becomes Node_0, ... Node_0 becomes Node_current_node_index
	for (i=0;i<=current_node_index;i++)
		{
		printf("%d...",route.route[current_node_index-i]);
		sprintf(field,"Node_%d",i);
		op_pk_nfd_set(pk_ptr,field,route.route[current_node_index-i]);
		}
	printf("end\n");

	// if the current node is not the source of the data packet
	if (current_node_index!=0)
		{
		sprintf(message,"seg_left %d and relay %d\n",current_node_index-1,route.route[current_node_index-1]);
		// set the Seg_Left field of the error packet
		op_pk_nfd_set(pk_ptr,"Seg_Left",current_node_index-1);
		// set the RELAY field of the packet
		op_pk_nfd_set(pk_ptr,"RELAY",route.route[current_node_index-1]);
		// and convert this dsr address in the mac address
		relay_mac_address=dsr_support_get_mac_from_dsr_address(route.route[current_node_index-1]);
		}
	// else the current node is the source of the data packet
	else
		{
		// thus there are no more Seg_Left for the error Packet
		op_pk_nfd_set(pk_ptr,"Seg_Left",0);
		// and the packet will be broadcast to every neighbours of the current node
		op_pk_nfd_set(pk_ptr,"RELAY",BROADCAST);
		relay_mac_address=BROADCAST;
		}
	
	// set the TR_source field of the error packet for transmission range purpose
	op_pk_nfd_set(pk_ptr,"TR_source",my_node_objid);

	// and send the error packet
	dsr_send_to_mac(pk_ptr,relay_mac_address);

	// update the cache of the current node considering the broken link
	dsr_clean_cache(my_dsr_address,error_node_dsr_address);
	dsr_clean_cache(error_node_dsr_address,my_dsr_address);
	
	// if the current node is the data packet source and if no request are processed for the unreachable destination
	if ((current_node_index==0)&&(request_sent[unreachable_node_dsr_address].waiting_time==0))
		{
		// a new request packet must be sent
		dsr_transmit_request_from_error(unreachable_node_dsr_address);
		}
	}
else
	{
	op_sim_end("the current node shoudl be in the path","","","");
	}

// breakpoint for debugging purpose
if(1)
	{
	op_prg_odb_bkpt("transmit_error");
	}
}

/*************************************************************/
/*                    DSR_HANDLE_ERROR                       */
/*************************************************************/
/* This function handle any error packet received by the node
/* 		Packet* pk_ptr: a pointer to the error packet to process
/*************************************************************/
void  dsr_handle_error(Packet* pk_ptr)
{
int source_dsr_address;
int destination_dsr_address;
int relay_dsr_address;
int relay_mac_address;
int seg_left;
char field[10];
int error_node_dsr_address;
int unreachable_node_dsr_address;

// extract some usefull information from the error packet
op_pk_nfd_get (pk_ptr,"SRC",&source_dsr_address);
op_pk_nfd_get (pk_ptr,"DEST",&destination_dsr_address); 
op_pk_nfd_get (pk_ptr,"RELAY",&relay_dsr_address);

op_pk_nfd_get(pk_ptr,"PbNode",&error_node_dsr_address);
op_pk_nfd_get(pk_ptr,"Unreached_Node",&unreachable_node_dsr_address);

// if the current node is the destination of the error packet
if (my_dsr_address==destination_dsr_address)
	{
	// update the cache of the current node considering the broken link
	dsr_clean_cache(source_dsr_address,error_node_dsr_address);
	dsr_clean_cache(error_node_dsr_address,source_dsr_address);

	// if no request are processed for the unreachable destination
	if (request_sent[unreachable_node_dsr_address].waiting_time==0)
		{
		//
		// a new request packet must be sent
		dsr_transmit_request_from_error(unreachable_node_dsr_address);
		}
	op_pk_destroy(pk_ptr);
	}

// else if the current node is the relay of the error packet
else if (my_dsr_address==relay_dsr_address)
	{
	// update the cache of the current node considering the broken link
	dsr_clean_cache(source_dsr_address,error_node_dsr_address);
	dsr_clean_cache(error_node_dsr_address,source_dsr_address);
	
	// extract the Seg_left field from the error packet
	op_pk_nfd_get (pk_ptr,"Seg_Left",&seg_left);
	
	// if the packet should be forwarded (Seg_left>0)
	if (seg_left>0)
		{
		// extract the node dsr address of the next relay in the error packet route
		sprintf(field,"Node_%d",--seg_left);
		op_pk_nfd_get(pk_ptr,field,&relay_dsr_address);
		// and set with this value the RELAY field of the error packet
		op_pk_nfd_set(pk_ptr,"RELAY",relay_dsr_address);
		
		// update the Seg_left field of the error packet
		op_pk_nfd_set(pk_ptr,"Seg_Left",seg_left);
		
		sprintf(message,"seg_left %d, and relay %d\n",seg_left,relay_dsr_address);
		dsr_message(message);
		
		// set the TR_source field for the transmission range purpose
		op_pk_nfd_set(pk_ptr,"TR_source",my_node_objid);
		
		// convert the relay dsr address in a mac address
		relay_mac_address=dsr_support_get_mac_from_dsr_address(relay_dsr_address);
		// and send (that is forward) the error packet
		dsr_send_to_mac(pk_ptr,relay_mac_address);
		}
	else
		{
		op_pk_destroy(pk_ptr);
		}
	}

// else the current node is not concerned by the error packet
else
	{
	// but it must update its cache considering the broken link
	dsr_clean_cache(source_dsr_address,error_node_dsr_address);
	dsr_clean_cache(error_node_dsr_address,source_dsr_address);
	op_pk_destroy(pk_ptr);
	}

// breakpoint for debugging purpose
if (1)
	{
	op_prg_odb_bkpt("handle_error");
	}
}

/*************************************************************/
/*               DSR_CHECK_GRATUITOUS_REPLY                  */
/*************************************************************/
/* This function checks when a data packet is not destinated to 
/* the node (neither destination nor next relay) if a shortest 
/* path physically exists and thus if a gratuitous reply is 
/* required
/* 		Packet* pk_ptr: a pointer to the received data packet
/*		RETURN: the index of the current node in the data packet
/* path if a gratuitous reply is required
/*				0 otherwise
/*************************************************************/
int dsr_ckeck_gratuitous_reply(Packet* pk_ptr)
{
int seg_left; 
int size_route;
int node_index;
int node_dsr_address;
int current_node_index;
char field[10];

// extract some usefull information from the data packet
op_pk_nfd_get (pk_ptr,"Seg_Left",&seg_left);
op_pk_nfd_get (pk_ptr,"Size_Route",&size_route);

current_node_index=0;
// index in the path of the node following the planned relay node
node_index=size_route-seg_left;

// look if the current node is in the path after the planned relay node
do
	{
	sprintf(field,"Node_%d",node_index);
	op_pk_nfd_get(pk_ptr,field,&node_dsr_address);
	// if the current node is found
	if (node_dsr_address==my_dsr_address) 
		{
		// it receives the packet before the planned relay => its index in the path will be returned
		current_node_index=node_index;
		}
	else
		{
		node_index++;
		}
	}
while ((node_index<size_route)&&(current_node_index==0));
return (current_node_index);
}

/*************************************************************/
/*             DSR_TRANSMIT_GRATUITOUS_REPLY                 */
/*************************************************************/
/* This function build and send a gratuitous reply packet
/* 		int current_node_index: the index of the current node in
/* the data packet path
/*		Packet* pk_ptr: a pointer to the data packet that requires
/* a gratuitous reply
/*************************************************************/
void dsr_transmit_gratuitous_reply(int current_node_index, Packet* pk_ptr)
{
Packet* reply_pk_ptr;
int source_dsr_address;
int destination_dsr_address;
int previous_node_dsr_address;
int previous_node_mac_address;
int size_route;
int last_node_index;
int seg_left;
int sequence_number;
int node_dsr_address;
char field[10];
int i,j;
Distribution* delay_reply_dist;
double delay;
sReply* reply;
int* intPtr;
Objid trsource;

// find the address of the current node in the path
sprintf(field,"Node_%d",current_node_index);
op_pk_nfd_get(pk_ptr,field,&node_dsr_address);

// if this address extracted from the path is valid
if (node_dsr_address==my_dsr_address)
	{
	// build the "gratuitous" reply packet and init its Type field
	reply_pk_ptr=op_pk_create_fmt("Dsr_Reply");
	op_pk_nfd_set(reply_pk_ptr,"Type",REPLY_PACKET_TYPE);
	
	// extract some usefull information from the data packet
	op_pk_nfd_get (pk_ptr,"SRC",&source_dsr_address);
	op_pk_nfd_get (pk_ptr,"DEST",&destination_dsr_address); 
	op_pk_nfd_get (pk_ptr,"Size_Route",&size_route);
	op_pk_nfd_get (pk_ptr,"Seg_Left",&seg_left);
	
	// calcul the last node index
	last_node_index=size_route-1;
	
	// set the SRC field of the reply packet : it is the destination of the data packet
	op_pk_nfd_set(reply_pk_ptr,"SRC",destination_dsr_address);
	// set the DEST field of the reply packet : it is the source of the data packet
	op_pk_nfd_set(reply_pk_ptr,"DEST",source_dsr_address);
	// set the Reply_From_Target field: neither 1 nor 0 since it is a Gratuitous reply (thus 2)
	op_pk_nfd_set(reply_pk_ptr,"Reply_From_Target",2);
	
	// extract the dsr_address of the last transmitor node (size_route-seg_left=node_following_the_planned_relay - 1 = planned relay - 1 = what we want)
	sprintf(field,"Node_%d",size_route-seg_left-2);
	op_pk_nfd_get(pk_ptr,field,&previous_node_dsr_address);
	
	// and set this node as the first relay of the gratuitous reply
	op_pk_nfd_set(reply_pk_ptr,"RELAY",previous_node_dsr_address);
	
	// update fields Node_k of the reply from the data packet path
	i=0;
	// write in the reply the path from the data packet source to the previous_node
	do 
		{
		sprintf(field,"Node_%d",i);
		op_pk_nfd_get(pk_ptr,field,&node_dsr_address);
		op_pk_nfd_set(reply_pk_ptr,field,node_dsr_address);
		i++;
		}
	while (node_dsr_address!=previous_node_dsr_address);
	seg_left=i-1;
	// following by the path from current node to the destination
	for (j=current_node_index;j<size_route;j++)
		{
		sprintf(field,"Node_%d",j);
		op_pk_nfd_get(pk_ptr,field,&node_dsr_address);
		sprintf(field,"Node_%d",i);
		op_pk_nfd_set(reply_pk_ptr,field,node_dsr_address);
		i++;
		}
	size_route=i;
	
	// set the seg_left and size_route fields of the gratuitous reply packet
	op_pk_nfd_set(reply_pk_ptr,"Seg_Left",seg_left);
	op_pk_nfd_set(reply_pk_ptr,"Size_Route",size_route);
	
	printf("Gratuitous reply route (size %d, seg_left %d): ",size_route,seg_left);
	for (i=0;i<size_route;i++)
		{	
		sprintf(field,"Node_%d",i);
		op_pk_nfd_get(reply_pk_ptr,field,&node_dsr_address);
		printf("%d...",node_dsr_address);
		}
	printf("end\n");
	
	// set the TR_source for the transmission range purpose 
	op_pk_nfd_set(reply_pk_ptr,"TR_source",my_node_objid);
	
	// if the current node is not the destination of the data packet then the data packet route need to be forwarded and the gratuitous reply will be sent after a delay to avoid replies storm
	if (current_node_index!=last_node_index)
		{
		// actualize the data packet path with the shortest one
		seg_left=size_route-(seg_left+2);
		op_pk_nfd_set(pk_ptr,"Seg_Left",seg_left);
		op_pk_nfd_set(pk_ptr,"Size_Route",size_route);
		for(i=0;i<size_route;i++)
			{
			sprintf(field,"Node_%d",i);
			op_pk_nfd_get(reply_pk_ptr,field,&node_dsr_address);
			op_pk_nfd_set(pk_ptr,field,node_dsr_address);
			}
		 
		 // init the delay_reply distribution
		 delay_reply_dist=op_dist_load("uniform",0.0,1.0);
		 // calcul the delay that the node must wait before sending its reply
		 delay=op_dist_outcome(delay_reply_dist);
		 delay=HOP_DELAY*((double)(size_route-1)+delay);
		
		 // prepare the code (destination and source coded in one integer) associated (in the fifo) with the reply information 
		 intPtr=(int*)op_prg_mem_alloc(sizeof(int));
		 *intPtr=destination_dsr_address*OPTION+source_dsr_address;
		
		 // check if a reply for the same route is still scheduled by looking in the fifo for its information
		 reply=fifo_multiplex_extract(&reply_fifo,*intPtr);
		 // if some information about a scheduled reply are found
		 if (reply!=OPC_NIL)
			 {
			 
			 // cancel the intrpt associated with this previous scheduled reply 
			 if (complex_intrpt_cancel(my_complex_intrpt_ptr,reply->evt))
				 {
				 // "replies" statistic calculation if success
				 op_stat_write(stat_total_canceled_replies,++total_canceled_replies);
				 }
			 else
				 {
				 // error since the intrpt must be valid and active
				 op_sim_end("Amazing error: reply should be valid and active","","","");
				 }
			 
			 // set the sequence number equal to the one already scheduled since this reply with replace the old one
			 sequence_number=reply->sequence_number;
			 op_pk_nfd_set (reply_pk_ptr,"Seq_number",sequence_number); 
			 // destroy the information about this reply 
			 op_pk_destroy(reply->pk);
			 op_prg_mem_free(reply);
			 }
		 // if no reply was scheduled
		 else
			 {
			 // set the sequence number to -1 to identifie it as a gratuitous reply
			 sequence_number=-1;
			 op_pk_nfd_set (reply_pk_ptr,"Seq_number",sequence_number); 
			 }
		 
		 // memory allocation for structure containing information about the new reply
		 reply=(sReply*)op_prg_mem_alloc(sizeof(sReply));
		 // store in it the reply packet
		 reply->pk=reply_pk_ptr;
		 // and its sequence number
		 reply->sequence_number=sequence_number;
		 // schedule an intrpt that will "say" when to send the reply and store the associated event in the reply information structure 
		 reply->evt=complex_intrpt_schedule_self(my_complex_intrpt_ptr,op_sim_time()+delay,SEND_REPLY_CODE,intPtr);
		 // save the scheduled reply in a multiplexed Fifo
		 fifo_multiplex_insert(&reply_fifo,reply,*intPtr);
		
		 sprintf(message,"I plan to send a gratuitous reply to %d for the route reaching %d in %f seconds (size_route=%d)\n",source_dsr_address,destination_dsr_address,delay,size_route);
		 dsr_message(message);
		
		 // "replies" statistic calculation
		 op_stat_write(stat_total_gratuitous_replies,++total_gratuitous_replies);
		 op_stat_write(stat_total_replies,++total_replies);
		 }
	
	// otherwise the current node is the destination of the data packet then the gratuitous reply will be sent immediatly
	else 
		{
		// set the sequence number to -1 since it is a direct (destination thus no reply could be scheduled to be sent) gratuitous reply
		op_pk_nfd_set (reply_pk_ptr,"Seq_number",-1); 
		// convert the previous node dsr address in its mac address
		previous_node_mac_address=dsr_support_get_mac_from_dsr_address(previous_node_dsr_address);
		// and send it the gratuitous reply packet
		dsr_send_to_mac(reply_pk_ptr,previous_node_dsr_address);
		}
	}
else
	{
	op_sim_end("Bad function current_node_index parameter","","","");
	}

// breakpoint for debugging purpose
if (1)
	{
	op_prg_odb_bkpt("transmit_gratuitous_reply");
	}
}


/*************************************************************/
/*                      DSR_CLEAN_CACHE                      */
/*************************************************************/
/* This function erase every paths containing the link identified
/* by the dsr address of two nodes from the current node route 
/* cache
/* 		int first_node_dsr_address: the dsr address of the first
/* node identifying the broken link
/* 		int second_node_dsr_address: the dsr address of the second
/* node identifying the broken link
/*************************************************************/
void dsr_clean_cache(int first_node_dsr_address,int second_node_dsr_address)
{
int i,j;

for (i=0;i<NUMBER_OF_NODES;i++)
	{
	// for each route contained in the route cache
	for (j=0;(j+1)<route_cache[i].size_route;j++)
		{
		// if the route contain the broken link
		if ((route_cache[i].route[j]==first_node_dsr_address)&&(route_cache[i].route[j+1]==second_node_dsr_address))
			{
			// the route is remove from the cache by calling the dsr_route_init function
			dsr_route_init(route_cache,i);
			break;
			}
		}
	}
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
////////////////////// OTHER FUNCTIONS ////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

/*************************************************************/
/*              DSR_IN_TRANSMISSION_RANGE                    */
/*************************************************************/
/* This function check if the packet is whitin the transmission
/* range of the sending node
/*		Packet* pk_ptr: a pointer to the received packet
/*		RETURN: 1 if the packet is whitin transmission range 
				0 otherwise
/*************************************************************/
int dsr_in_transmission_range(Packet* pk_ptr)
{
Objid tr_source;
double x,y,z;
double latitude,longitude,altitude;
double x_source,y_source,z_source;
double distance;
Compcode comp_code;

// get the objid of the source node from the packet
op_pk_nfd_get(pk_ptr,"TR_source",&tr_source);

// get the position of both transmitter and receiver nodes and compute the distance between them
comp_code=op_ima_node_pos_get (my_node_objid,&latitude,&longitude, &altitude, &x, &y, &z);
comp_code=op_ima_node_pos_get (tr_source,&latitude,&longitude, &altitude, &x_source, &y_source, &z_source);
distance= sqrt((x-x_source)*(x-x_source) + (y-y_source)*(y-y_source) + (z-z_source)*(z-z_source));

if (distance<=0.0)
	{
	// if the distance is<=0 there is a problem thus the packet is rejected
	dsr_message("The received packet is NOT in TR\n");
	return(0);
	}
else if (distance<=TRANSMISSION_RANGE)
	{
	// if the distance is whitin transmission range the packet is accepted
    dsr_message("The received packet is IN TR\n");
	return(1);
	}
else
	{
	// otherwise the packet is rejected
	dsr_message("The received packet is NOT in TR\n");
	return(0);
	}
}

/*************************************************************/
/*                  DSR_INSERT_BUFFER                        */
/*************************************************************/
/* This function inserts a data packet waiting to be transmitted
/* in the node buffer (queue) corresponding to its destination
/* 		Packet* pk_ptr: a pointer to the data packet to store 
/* in the buffer
/* 		int destination_dsr_address: the dsr address of the data 
/* packet destination
/*************************************************************/
void dsr_insert_buffer(Packet* pk_ptr, int destination_dsr_address)
{
// enqueue packet in the destination subqueue
if (op_subq_pk_insert(destination_dsr_address,pk_ptr,OPC_QPOS_TAIL)!=OPC_QINS_OK)
	{
	// if there is an error the packet is destroyed
	dsr_message("Error, Can not insert packet in subqueue \n");
	op_pk_destroy(pk_ptr);
	}
else
	{
	// otherwise buffer statistic is updated
	sprintf(message,"I am inserting in the buffer a data packet for %d to %d\n",destination_dsr_address);
	dsr_message(message);
	op_stat_write(stat_total_data_in_buffer,++total_data_in_buffer);
	}
}

/*************************************************************/
/*                  DSR_BUFFER_EMPTY                         */
/*************************************************************/
/* This function checks if the node buffer is empty for the given 
/* destination
/* 		int destination_dsr_address: the dsr address of the 
/* eventual data packet destination
/*		RETURN: 1 if the buffer is empty
/*				0 otherwise
/*************************************************************/
int dsr_buffer_empty(int destination_dsr_address)
{
return (op_subq_empty(destination_dsr_address));
}

/*************************************************************/
/*                  DSR_EXTRACT_BUFFER                       */
/*************************************************************/
/* This function extract a data packet from the buffer for the 
/* given destination
/* 		int destination_dsr_address: the dsr address of the data 
/* packet destination
/* `	RETURN: a pointer to the data packet extract from the 
/* buffer
/*************************************************************/
Packet* dsr_extract_buffer(int destination_dsr_address)
{
Packet* pk_ptr;
// if there is at list one packet in the destination suqueue 
if (op_subq_empty(destination_dsr_address)==OPC_FALSE)
	{
	// extract the first packet from this queue
	pk_ptr=op_subq_pk_remove(destination_dsr_address,OPC_QPOS_HEAD);
	// update the buffer statistic
	op_stat_write(stat_total_data_in_buffer,--total_data_in_buffer);
	}
// if there is no packet in the destination subqueue
else
	{
	// will return OPC_NIL
	pk_ptr=OPC_NIL;
	}
// return the packet pointer
return(pk_ptr);
}

/*************************************************************/
/*                    DSR_NO_LOOP_IN_ROUTE                   */
/*************************************************************/
/* This function checks and removes every loops in the given 
/* route
/* 		sRoute* route: a pointer to the route that will be free 
/* from every eventual loops
/*************************************************************/
void dsr_no_loop_in_route(sRoute* route)
{
int i,j;
int dsr_address1,dsr_address2;
int size_route;
int loop_detected;
do
	{
	size_route=route->size_route;
	loop_detected=0;
	i=0;
	// try to detect a loop in the path
	do
		{
		j=size_route-1;
		dsr_address1=route->route[i];
		do
			{
			dsr_address2=route->route[j];
			if ((dsr_address1==dsr_address2)&&(i!=j))
				{
				// set the loop_detected flag to indicate that there is a loop in the path 
				loop_detected=1;
				j++; 
				}
			else 
				{
				j--;
				}
			}
		while ((!loop_detected)&&(i<j));
		i++;	
		}
	while ((!loop_detected)&&(i<size_route-1));
	
	// if a loop was detected in the path
	if (loop_detected)
		{
		// remove this loop
		for (j;j<size_route;j++)
			{
			route->route[i]=route->route[j];
			i++;
			}
		route->size_route=size_route-(j-i);
		}
	}
// do this process until there is no more loop in the path
while (loop_detected);

// display the new loop free path on the screen
printf("New route without loop: ");
for (i=0;i<route->size_route;i++)
	{
	printf("%d...",route->route[i]);
	}
printf("end\n");
}

/*************************************************************/
/*                  DSR_SEND_TO_MAC                          */
/*************************************************************/
/* This function communicate the address and the packet to send 
/* to the mac layer
/* 		Packet* pk_ptr: a pointer to the packet to send to the 
/* MAC LAYER
/*		int destination_mac_address: the MAC address of the 
/* packet destination 
/*************************************************************/
void dsr_send_to_mac(Packet* pk_ptr, int destination_mac_address)
{
int packet_type;
Ici* iciptr;
Packet* data_pk_ptr;
int reply_source;
int reply_dest;

// calcul the overhead statictic
op_pk_nfd_get(pk_ptr,"Type",&packet_type);
if (packet_type==DATA_PACKET_TYPE)
	{
	op_pk_nfd_get(pk_ptr,"data",&data_pk_ptr);
	total_data_transmit+=op_pk_total_size_get(data_pk_ptr);
	op_stat_write(stat_total_data_transmit,total_data_transmit);
	total_control_transmit+=op_pk_total_size_get(pk_ptr);
	op_stat_write(stat_total_control_transmit,total_control_transmit);
	op_stat_write(stat_total_overhead,((double)total_control_transmit/(double)(total_data_transmit+total_control_transmit))*100);
	op_pk_nfd_set(pk_ptr,"data",data_pk_ptr);
	}
else
	{
	total_control_transmit+=op_pk_total_size_get(pk_ptr);
	}

// creatre and install an ici to communicate the mac address of the packet destination to the MAC layer
iciptr = op_ici_create("Dsr_Wlan_Dest_Ici");
op_ici_attr_set(iciptr,"Packet_Destination",destination_mac_address);
op_ici_install(iciptr);
// send the packet to the MAC layer
op_pk_send(pk_ptr,TO_MAC_LAYER_STRM);

// breakpoint for debugging purpose
if (1)
	{
	op_prg_odb_bkpt("send_to_mac");
	}
}  

/*************************************************************/
/*                     DSR_MESSAGE                           */
/*************************************************************/
/* This function display the name of the talking node (thus the 
/* name of the current node) following by its message
/* 		const char* message: the node message to display
/*************************************************************/
void dsr_message(const char* message)
{
printf("%s: ",my_name);
printf(message);
}

/*************************************************************/
/*                  DSR_END_SIMULATION                       */
/*************************************************************/
/* This function is called at the end of the simulation. Its 
/* purpose is to free the memory used by the dsr process, and to 
/* store all the collected statistic in a Text file
/*************************************************************/
void dsr_end_simulation()
{
FILE* out;
double step_dist;
double pos_timer;
char outfile_name[256];

// finalize the "requests" statistics
op_stat_write(stat_total_non_propa_requests,total_non_propa_requests);
op_stat_write(stat_total_propa_requests,total_propa_requests);
op_stat_write(stat_total_renewed_propa_requests,total_renewed_propa_requests);
op_stat_write(stat_total_requests_from_error,total_requests_from_error);
op_stat_write(stat_total_requests,total_requests);

// finalize the "replies" statistics
op_stat_write(stat_total_replies_from_target,total_replies_from_target);
op_stat_write(stat_total_replies_from_relay,total_replies_from_relay);
op_stat_write(stat_total_canceled_replies,total_canceled_replies);
op_stat_write(stat_total_gratuitous_replies,total_gratuitous_replies);

// finalize the "data packets" statistics
op_stat_write(stat_total_data_served,total_data_served);
op_stat_write(stat_total_data_in_buffer,total_data_in_buffer);
op_stat_write(stat_total_data_successfully_transmitted,total_data_successfully_transmitted);
op_stat_write(stat_total_data_efficiency,(double)total_data_successfully_transmitted/(double)total_data_served);

// finalize the "error packets" statistic
op_stat_write(stat_total_error_detected,total_error_detected);

// finalize other statistics
op_stat_write(stat_total_ack,total_ack);
op_stat_write(stat_mean_hops_by_route,(double)total_ack/(double)total_data_served);
op_stat_write(stat_amazing_errors,amazing_errors);
op_stat_write(stat_total_data_transmit,total_data_transmit);
op_stat_write(stat_total_control_transmit,total_control_transmit);
op_stat_write(stat_total_overhead,((double)total_control_transmit/(double)(total_data_transmit+total_control_transmit))*100);

// closure of the dsr support package
dsr_support_end();

// save some collected statistic in a Text file
op_ima_sim_attr_get(OPC_IMA_STRING,"Dsr Result File Name",&outfile_name);
op_ima_sim_attr_get(OPC_IMA_DOUBLE, "mobil_POS_TIMER", &pos_timer);
op_ima_sim_attr_get(OPC_IMA_DOUBLE, "mobil_STEP_DIST", &step_dist);
out=fopen(outfile_name,"w");
fprintf(out,"/////////////////////////////////////////////\n");
fprintf(out,"// RESULT FILE %s\n",outfile_name);
fprintf(out,"// Number of nodes: %d\n",NUMBER_OF_NODES);
fprintf(out,"// Mobility: %f m/s\n",step_dist/pos_timer);
fprintf(out,"// Simulation duration: %f s\n",op_sim_time());
fprintf(out,"/////////////////////////////////////////////\n");
fprintf(out,"\n");
fprintf(out,"EFFICIENCY:         %f\n", (double)(total_data_successfully_transmitted)/(double)(total_data_served));
fprintf(out,"data served:        %d\n",total_data_served);
fprintf(out,"data success:       %d\n",total_data_successfully_transmitted);
fprintf(out,"data in buffer:      %d\n",total_data_in_buffer);
fprintf(out,"mean hops by route: %f\n",(double)total_ack/(double)total_data_served);
fprintf(out,"overhead (bits):    %f\n",((double)total_control_transmit/(double)(total_data_transmit+total_control_transmit))*100);
fprintf(out,"\n");
fprintf(out,"total request:         %d\n",total_requests);
fprintf(out,"non propa request:     %d\n",total_non_propa_requests);
fprintf(out,"propa request:         %d\n",total_propa_requests);
fprintf(out,"renewed propa request: %d\n",total_renewed_propa_requests);
fprintf(out,"request from error:    %d\n",total_requests_from_error);
fprintf(out,"error detected:        %d\n",total_error_detected);
fprintf(out,"\n");
fprintf(out,"total reply:     %d\n",total_replies);
fprintf(out,"reply from target: %d\n",total_replies_from_target);
fprintf(out,"reply from relay:  %d\n",total_replies_from_relay);
fprintf(out,"gratuitous reply:  %d\n",total_gratuitous_replies);
fprintf(out,"canceled reply:    %d\n",total_canceled_replies);
fprintf(out,"\n");
fprintf(out,"amazing error: %d\n",amazing_errors);
fprintf(out,"hop delay    : %f\n",HOP_DELAY);
fprintf(out,"\n");
fprintf(out,"/////////////////////////////////////////////\n");
fclose(out);
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
	void lpw_routing_layer (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Obtype lpw_routing_layer_init (int * init_block_ptr);
	VosT_Address lpw_routing_layer_alloc (VOS_THREAD_INDEX_ARG_COMMA VosT_Obtype, int);
	void lpw_routing_layer_diag (OP_SIM_CONTEXT_ARG_OPT);
	void lpw_routing_layer_terminate (OP_SIM_CONTEXT_ARG_OPT);
	void lpw_routing_layer_svar (void *, const char *, void **);


	VosT_Fun_Status Vos_Define_Object (VosT_Obtype * obst_ptr, const char * name, unsigned int size, unsigned int init_obs, unsigned int inc_obs);
	VosT_Address Vos_Alloc_Object_MT (VOS_THREAD_INDEX_ARG_COMMA VosT_Obtype ob_hndl);
	VosT_Fun_Status Vos_Poolmem_Dealloc_MT (VOS_THREAD_INDEX_ARG_COMMA VosT_Address ob_ptr);
#if defined (__cplusplus)
} /* end of 'extern "C"' */
#endif




/* Process model interrupt handling procedure */


void
lpw_routing_layer (OP_SIM_CONTEXT_ARG_OPT)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	FIN_MT (lpw_routing_layer ());
	if (1)
		{
		///////////////////////////////////////////////////////////////
		// DSR TEMPORARY VARIABLES BLOCK
		//
		// Declaration of the temporary variables
		///////////////////////////////////////////////////////////////
		
		Packet * pk_ptr;
		Ici* ici_dest_address;
		Ici* ici_ack;
		Ici* ici_error;
		int destination_dsr_address;
		int relay_dsr_address;
		int relay_mac_address;
		int packet_type;
		int *intPtr;
		int tmp;
		sReply* reply;
		int error_node_dsr_address;
		sNoAck* no_ack;
		sNoAck* no_ack_first;
		int i;
		


		FSM_ENTER ("lpw_routing_layer")

		FSM_BLOCK_SWITCH
			{
			/*---------------------------------------------------------*/
			/** state (Init) enter executives **/
			FSM_STATE_ENTER_FORCED (0, "Init", state0_enter_exec, "lpw_routing_layer [Init enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_routing_layer [Init enter execs], state0_enter_exec)
				{
				///////////////////////////////////////////////////////////////
				// DSR INIT STATE
				//
				// Initialize the dsr process model (table, variable, stats...)
				///////////////////////////////////////////////////////////////
				
				// initialize the user parameters
				dsr_user_parameter_init();
				
				// validation of the mac and dsr addresses declared during the pre-init stage
				dsr_support_validate_addresses();
				
				// initialize the tables and variable used by the process
				dsr_tables_init();
				
				// initialize the statistic that will be collected by the process
				dsr_stats_init();
				
				// breakpoint for debugging purpose
				if (1)
					{
					op_prg_odb_bkpt("init");
					}
				
					
					
				
				
				
				
				}

				FSM_PROFILE_SECTION_OUT (lpw_routing_layer [Init enter execs], state0_enter_exec)

			/** state (Init) exit executives **/
			FSM_STATE_EXIT_FORCED (0, "Init", "lpw_routing_layer [Init exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_routing_layer [Init exit execs], state0_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_routing_layer [Init exit execs], state0_exit_exec)


			/** state (Init) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "Init", "idle")
				/*---------------------------------------------------------*/



			/** state (idle) enter executives **/
			FSM_STATE_ENTER_UNFORCED (1, "idle", state1_enter_exec, "lpw_routing_layer [idle enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_routing_layer [idle enter execs], state1_enter_exec)
				{
				///////////////////////////////////////////////////////////////
				// DSR IDLE STATE
				//
				// Just wait for an interruption
				///////////////////////////////////////////////////////////////
				
				
				}

				FSM_PROFILE_SECTION_OUT (lpw_routing_layer [idle enter execs], state1_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (3,"lpw_routing_layer")


			/** state (idle) exit executives **/
			FSM_STATE_EXIT_UNFORCED (1, "idle", "lpw_routing_layer [idle exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_routing_layer [idle exit execs], state1_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_routing_layer [idle exit execs], state1_exit_exec)


			/** state (idle) transition processing **/
			FSM_PROFILE_SECTION_IN (lpw_routing_layer [idle trans conditions], state1_trans_conds)
			FSM_INIT_COND (UPPER_LAYER_ARRIVAL)
			FSM_TEST_COND (MAC_LAYER_ARRIVAL)
			FSM_TEST_COND (ACK)
			FSM_TEST_COND (ERROR)
			FSM_TEST_COND (NO_REPLY)
			FSM_TEST_COND (SEND_REPLY)
			FSM_TEST_COND (END_SIM)
			FSM_DFLT_COND
			FSM_TEST_LOGIC ("idle")
			FSM_PROFILE_SECTION_OUT (lpw_routing_layer [idle trans conditions], state1_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 4, state4_enter_exec, ;, "UPPER_LAYER_ARRIVAL", "", "idle", "Upper Layer Arrival")
				FSM_CASE_TRANSIT (1, 6, state6_enter_exec, ;, "MAC_LAYER_ARRIVAL", "", "idle", "MAC Layer Arrival")
				FSM_CASE_TRANSIT (2, 5, state5_enter_exec, ;, "ACK", "", "idle", "Ack")
				FSM_CASE_TRANSIT (3, 3, state3_enter_exec, ;, "ERROR", "", "idle", "Error")
				FSM_CASE_TRANSIT (4, 7, state7_enter_exec, ;, "NO_REPLY", "", "idle", "No Reply")
				FSM_CASE_TRANSIT (5, 8, state8_enter_exec, ;, "SEND_REPLY", "", "idle", "Send Reply")
				FSM_CASE_TRANSIT (6, 1, state1_enter_exec, dsr_end_simulation();, "END_SIM", "dsr_end_simulation()", "idle", "idle")
				FSM_CASE_TRANSIT (7, 1, state1_enter_exec, ;, "default", "", "idle", "idle")
				}
				/*---------------------------------------------------------*/



			/** state (pre-init) enter executives **/
			FSM_STATE_ENTER_UNFORCED_NOLABEL (2, "pre-init", "lpw_routing_layer [pre-init enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_routing_layer [pre-init enter execs], state2_enter_exec)
				{
				///////////////////////////////////////////////////////////////
				// DSR PRE-INIT STATE
				//
				// Pre-Initialize the dsr process model by managing and checking 
				// every dsr addresses
				///////////////////////////////////////////////////////////////
				
				// start the dsr support package (dsr management package)
				dsr_support_start();
				// preinitialize the process
				dsr_pre_init();
				sprintf(message,"Successful initialization with mac_address=%d and dsr_address=%d\n",my_mac_address,my_dsr_address);
				dsr_message(message);
				
				// ensure that each node has finised its pre-initialization before going in the init sate
				op_intrpt_schedule_self(op_sim_time(),0);
				
				// breakpoint for debugging purpose
				if (1)
					{
					op_prg_odb_bkpt("pre-init");
					}
				}

				FSM_PROFILE_SECTION_OUT (lpw_routing_layer [pre-init enter execs], state2_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (5,"lpw_routing_layer")


			/** state (pre-init) exit executives **/
			FSM_STATE_EXIT_UNFORCED (2, "pre-init", "lpw_routing_layer [pre-init exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_routing_layer [pre-init exit execs], state2_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_routing_layer [pre-init exit execs], state2_exit_exec)


			/** state (pre-init) transition processing **/
			FSM_TRANSIT_FORCE (0, state0_enter_exec, ;, "default", "", "pre-init", "Init")
				/*---------------------------------------------------------*/



			/** state (Error) enter executives **/
			FSM_STATE_ENTER_FORCED (3, "Error", state3_enter_exec, "lpw_routing_layer [Error enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_routing_layer [Error enter execs], state3_enter_exec)
				{
				///////////////////////////////////////////////////////////////
				// UPPER_LAYER_ARRIVAL
				//
				// Processed when an error (broken link) is detected
				///////////////////////////////////////////////////////////////
				
				dsr_message("I am in error\n");
				// if the error interupt was genarated by the process itself
				if ((op_intrpt_type()==OPC_INTRPT_SELF)&&(complex_intrpt_code(my_complex_intrpt_ptr)==SELF_ERROR_CODE))
					{
					// if the mac layer should send error and ack message it means that the mac layer doesn't reply
					if (MAC_LAYER_ACK)
						{
						// close the complex intrpt
						complex_intrpt_close(my_complex_intrpt_ptr);
						// and end the simulation
						sprintf(message,"No reply from the MAC layer after WAIT_ACK_TIME = %fseconds\n",WAIT_ACK_TIME);
						op_sim_end(message,"","","");
						}
					}
				// otherwise the error interupt was sent by the MAC layer
				else if ((op_intrpt_type()==OPC_INTRPT_REMOTE)&&(op_intrpt_code()==ERROR_CODE))
					{
					// check if ack from the MAC layer are waited
					if (!MAC_LAYER_ACK)
						{
						op_sim_end("Ack received from the MAC layer, despite none of them are waited!","","","");
						}
					// error statistic calculation
					op_stat_write(stat_total_error_detected,++total_error_detected);
					
					// extract the ici associated with the error interupt
					ici_error=op_intrpt_ici();
					
					// extract some usefull information from this ici
					op_ici_attr_get(ici_error,"Relay_Destination",&error_node_dsr_address);
					op_ici_attr_get(ici_error,"Final_Destination",&destination_dsr_address);
					op_ici_destroy(ici_error);
						
					// read the first sNoAck in the fifo, that is the oldest one, that is the one that should be associated with the intrpt
					no_ack_first=(sNoAck*)fifo_multiplex_read_first(&no_ack_fifo,&tmp);
						
					// check that this sNoAck structure is effectively associated with the interupt 
					if (no_ack_first!=(no_ack=(sNoAck*)fifo_multiplex_extract(&no_ack_fifo,error_node_dsr_address)))
						{
						// if not then error
						printf("error relay %d, final dest %d, first in pile %d\n",error_node_dsr_address, destination_dsr_address, tmp);
						printf("error mac for %d and my error %d\n",error_node_dsr_address,tmp);
						op_sim_end("Amazing error: the error received is not for the oldest packet sent\n","","","");
						}
					
					// if this sNoAck structure was found
					if (no_ack!=OPC_NIL)
						{
						
						// cancel the complex_intrpt associated with this no_ack event meaning that no reply (ack or error) are received from the MAC layer
						if (complex_intrpt_cancel(my_complex_intrpt_ptr,no_ack->evt))
							{
							sprintf(message,"I have just received an error for node %d from the MAC layer\n",error_node_dsr_address);
							dsr_message(message);
							}
						// if the no_ack event is either invalid or inactive 
						else
							{
							// there is an amazing error and the simulation is stopped
							op_sim_end("Amazing error: the no_ack event for MAC layer reply checking must be valid and active","","","");
							//op_stat_write(stat_amazing_errors,++amazing_errors);
							}
						}
					// if the sNoAck structure was not found
					else 
						{
						// there is an amazing error and the simulation is stopped
						op_sim_end("Amazing error: no_ack object associated with the current MAC layer error intrpt\n","","","");
						}
					
					// and finally transmit an error packet to the data packet source
					dsr_transmit_error(no_ack->route, error_node_dsr_address);
					}
				
				// breakpoint for debugging purpose
				if(1)
					{
					op_prg_odb_bkpt("error");
					}
				
				
				}

				FSM_PROFILE_SECTION_OUT (lpw_routing_layer [Error enter execs], state3_enter_exec)

			/** state (Error) exit executives **/
			FSM_STATE_EXIT_FORCED (3, "Error", "lpw_routing_layer [Error exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_routing_layer [Error exit execs], state3_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_routing_layer [Error exit execs], state3_exit_exec)


			/** state (Error) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "Error", "idle")
				/*---------------------------------------------------------*/



			/** state (Upper Layer Arrival) enter executives **/
			FSM_STATE_ENTER_FORCED (4, "Upper Layer Arrival", state4_enter_exec, "lpw_routing_layer [Upper Layer Arrival enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_routing_layer [Upper Layer Arrival enter execs], state4_enter_exec)
				{
				///////////////////////////////////////////////////////////////
				// UPPER_LAYER_ARRIVAL
				//
				// Processed when some data from the upper layer are received 
				///////////////////////////////////////////////////////////////
				
				dsr_message("I am in upper_layer_arrival\n");
				
				// extract the packet from the stream coming from the upper layer
				pk_ptr = op_pk_get(FROM_UPPER_LAYER_STRM);
				
				// extract the dsr address of the packet destination from the ici associated strm intrpt
				ici_dest_address = op_intrpt_ici();
				op_ici_attr_get(ici_dest_address,"Packet_Destination",&destination_dsr_address);
				
				// call the transmit data function
				dsr_upper_layer_data_arrival(pk_ptr, destination_dsr_address);
				
				// breakpoint for debugging purpose
				if (1)
					{
					op_prg_odb_bkpt("upper_layer_arrival");
					}
				}

				FSM_PROFILE_SECTION_OUT (lpw_routing_layer [Upper Layer Arrival enter execs], state4_enter_exec)

			/** state (Upper Layer Arrival) exit executives **/
			FSM_STATE_EXIT_FORCED (4, "Upper Layer Arrival", "lpw_routing_layer [Upper Layer Arrival exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_routing_layer [Upper Layer Arrival exit execs], state4_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_routing_layer [Upper Layer Arrival exit execs], state4_exit_exec)


			/** state (Upper Layer Arrival) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "Upper Layer Arrival", "idle")
				/*---------------------------------------------------------*/



			/** state (Ack) enter executives **/
			FSM_STATE_ENTER_FORCED (5, "Ack", state5_enter_exec, "lpw_routing_layer [Ack enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_routing_layer [Ack enter execs], state5_enter_exec)
				{
				///////////////////////////////////////////////////////////////
				// UPPER_LAYER_ARRIVAL STATE
				//
				// Processed when an acknoledgement is received
				///////////////////////////////////////////////////////////////
				
				dsr_message("I am in ack\n");
				
				// extract the ici associated with the ack interupt
				ici_ack=op_intrpt_ici();
				
				// extract some usefull information from this ici
				op_ici_attr_get(ici_ack,"Relay_Destination",&relay_dsr_address);
				op_ici_attr_get(ici_ack,"Final_Destination",&destination_dsr_address);
				op_ici_destroy(ici_ack);
				
				// read the first sNoAck in the fifo, that is the oldest one, that is the one that should be associated with the intrpt
				no_ack_first=(sNoAck*)fifo_multiplex_read_first(&no_ack_fifo,&tmp);
				
				// check that this sNoAck structure is effectively associated with the interupt 
				if (no_ack_first!=(no_ack=(sNoAck*)fifo_multiplex_extract(&no_ack_fifo,relay_dsr_address)))
					{
					// if not then error
					sprintf(message,"relay %d, final dest %d, first in fifo %d\n",relay_dsr_address,destination_dsr_address,tmp);
					dsr_message(message);
					op_stat_write(stat_amazing_errors,++amazing_errors);
					op_sim_end("Amazing error: the ack received is not for the oldest packet sent\n","","","");
					}
					
				// if some information were found
				if (no_ack!=OPC_NIL)
					{
					// cancel the self intrpt associated with the data packet to generate an error in case of no_ack reception
					if (op_ev_pending(no_ack->evt) == OPC_FALSE)
						{
						// there is an error since this self intrpt should be valide and active
						op_sim_end("Amazing Error: the event in the sNoAck structure should be active\n","","","");
						//op_stat_write(stat_amazing_errors,++amazing_errors);
						}
					else if (op_ev_cancel(no_ack->evt)==OPC_COMPCODE_SUCCESS)
						{
						sprintf(message,"reception Ack, so transmission data to %d successfull\n",relay_dsr_address);
						dsr_message(message);
						}
					// free the memory used by the sNoAck structure
					op_prg_mem_free(no_ack);
					}
				else
					{
					op_sim_end("No data information associated with the received Ack","","","");
					}
				
				op_stat_write(stat_total_ack,++total_ack);
				
				
				// if there is a known route to the same destination may be there is some packet to serve in the buffer
				// (case I was the source node of the data packet just after the reception of a reply for its route)
				if (route_cache[destination_dsr_address].size_route>0)
					{	
					// try to extract the first packet to send on the same route
					pk_ptr=dsr_extract_buffer(destination_dsr_address);
					// if a packet was found in the buffer
					if (pk_ptr!=OPC_NIL)
						{
						dsr_transmit_data(pk_ptr,destination_dsr_address);
						}
					}
				
				// breakpoint for debugging purpose
				if (1)
					{
					op_prg_odb_bkpt("ack");
					}
				
				
				}

				FSM_PROFILE_SECTION_OUT (lpw_routing_layer [Ack enter execs], state5_enter_exec)

			/** state (Ack) exit executives **/
			FSM_STATE_EXIT_FORCED (5, "Ack", "lpw_routing_layer [Ack exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_routing_layer [Ack exit execs], state5_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_routing_layer [Ack exit execs], state5_exit_exec)


			/** state (Ack) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "Ack", "idle")
				/*---------------------------------------------------------*/



			/** state (MAC Layer Arrival) enter executives **/
			FSM_STATE_ENTER_FORCED (6, "MAC Layer Arrival", state6_enter_exec, "lpw_routing_layer [MAC Layer Arrival enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_routing_layer [MAC Layer Arrival enter execs], state6_enter_exec)
				{
				///////////////////////////////////////////////////////////////
				// MAC_LAYER_ARRIVAL
				//
				// Processed when a packet from the MAC layer is received
				///////////////////////////////////////////////////////////////
				
				// extract the packet from the stream coming from the mac layer
				pk_ptr=op_pk_get(FROM_MAC_LAYER_STRM);
				
				// if the received packet is within transmission range
				if (dsr_in_transmission_range(pk_ptr))
					{
					// it is processing depending of its type
					op_pk_nfd_get(pk_ptr,"Type",&packet_type);
					switch(packet_type)
						{
						case DATA_PACKET_TYPE:
							dsr_message("I have just received a data packet\n"); 
							// call the dsr_handle_data function for data packet
							dsr_handle_data(pk_ptr);
							break;
						case REQUEST_PACKET_TYPE:
							dsr_message("I have just received a request packet\n");
							// call the dsr_handle_request function for request packet
							dsr_handle_request(pk_ptr);
							break;
						case REPLY_PACKET_TYPE:
							dsr_message("I have just received a reply packet\n");
							// call the dsr_handle_reply function for reply packet
							dsr_handle_reply(pk_ptr);
							break;		
						case ERROR_PACKET_TYPE:
							dsr_message("I have just received an error packet\n");
							// call the dsr_handle_error function for error packet
							dsr_handle_error(pk_ptr);
					 		break;
						default:
							// otherwise the packet is invalid and destroyed
							dsr_message("I have just received an invalid packet\n");
							op_pk_destroy(pk_ptr);			
						}
					// breakpoint for debugging purpose
					if (1)
						{
						op_prg_odb_bkpt("mac_layer_arrival");
						}
					}
				// if the packet is outside the transmission range
				else 
					{
					// it is not processed and destroyed
					op_pk_destroy(pk_ptr);
					}
				}

				FSM_PROFILE_SECTION_OUT (lpw_routing_layer [MAC Layer Arrival enter execs], state6_enter_exec)

			/** state (MAC Layer Arrival) exit executives **/
			FSM_STATE_EXIT_FORCED (6, "MAC Layer Arrival", "lpw_routing_layer [MAC Layer Arrival exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_routing_layer [MAC Layer Arrival exit execs], state6_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_routing_layer [MAC Layer Arrival exit execs], state6_exit_exec)


			/** state (MAC Layer Arrival) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "MAC Layer Arrival", "idle")
				/*---------------------------------------------------------*/



			/** state (No Reply) enter executives **/
			FSM_STATE_ENTER_FORCED (7, "No Reply", state7_enter_exec, "lpw_routing_layer [No Reply enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_routing_layer [No Reply enter execs], state7_enter_exec)
				{
				///////////////////////////////////////////////////////////////
				// UPPER_LAYER_ARRIVAL
				//
				// Processed when no reply answering to a request is received
				///////////////////////////////////////////////////////////////
				
				// read the destination_dsr_address from the data associated with the intrpt
				intPtr=complex_intrpt_data(my_complex_intrpt_ptr);
				
				sprintf(message,"I did not received any reply for route to node %d\n",*intPtr);
				dsr_message(message);
				
				// if the node is effectivelly waiting for a reply
				if (request_sent[*intPtr].waiting_time!=0) 
					{
					// build and transmit another request to the wanted destination
					dsr_transmit_request(*intPtr);
					}
				// otherwise there is an error since this interruption should not be scheduled
				else
					{
					// and the simulation is stopped
					op_sim_end("La condition sur no_reply is vraiment utile\n","","","");
					}
				
				// free the memory used by the complex_intrpt data
				op_prg_mem_free(intPtr);
				// and close the current complex_intrpt
				complex_intrpt_close(my_complex_intrpt_ptr);
				
				// breakpoint for debugging purpose
				if (1)
					{
					op_prg_odb_bkpt("no_reply");
					}
				}

				FSM_PROFILE_SECTION_OUT (lpw_routing_layer [No Reply enter execs], state7_enter_exec)

			/** state (No Reply) exit executives **/
			FSM_STATE_EXIT_FORCED (7, "No Reply", "lpw_routing_layer [No Reply exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_routing_layer [No Reply exit execs], state7_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_routing_layer [No Reply exit execs], state7_exit_exec)


			/** state (No Reply) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "No Reply", "idle")
				/*---------------------------------------------------------*/



			/** state (Send Reply) enter executives **/
			FSM_STATE_ENTER_FORCED (8, "Send Reply", state8_enter_exec, "lpw_routing_layer [Send Reply enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_routing_layer [Send Reply enter execs], state8_enter_exec)
				{
				///////////////////////////////////////////////////////////////
				// SEND_REPLY
				//
				// Proccessed when a scheduled reply (from relay) must be sent
				///////////////////////////////////////////////////////////////
				
				
				// read the code (destination and source coded in one integer) associated (in the fifo) with the reply information 
				intPtr=complex_intrpt_data(my_complex_intrpt_ptr);
				
				// extract with this code the information about the reply to send from the fifo
				reply=(sReply*)fifo_multiplex_extract(&reply_fifo,*intPtr);
				
				// free the memory used by the code (complex_intrpt data)
				op_prg_mem_free(intPtr);
				
				// if some information about the reply to send were found
				if (reply!=NULL)
					{
					// extract from the reply packet the dsr_address of the next node
					op_pk_nfd_get(reply->pk,"RELAY",&relay_dsr_address);
					// convert the dsr address of the relay in its mac address
					relay_mac_address=dsr_support_get_mac_from_dsr_address(relay_dsr_address);
					// and send the reply packet
					dsr_send_to_mac(reply->pk,relay_mac_address);
					// free the memory used by the reply information structure
					op_prg_mem_free(reply);
					dsr_message("I am sending a scheduled reply\n");
					}
				// if no information about the reply to send were found in the fifo
				else
					{
					// the simulation is stoped
					op_sim_end("Amazing error: no reply information were found in the fifo","","","");
					}
				
				// close the current complex_intrpt
				complex_intrpt_close(my_complex_intrpt_ptr);
				
				// breakpoint for debugging purpose
				if (1)
					{
					op_prg_odb_bkpt("send_reply");
					}
				
				}

				FSM_PROFILE_SECTION_OUT (lpw_routing_layer [Send Reply enter execs], state8_enter_exec)

			/** state (Send Reply) exit executives **/
			FSM_STATE_EXIT_FORCED (8, "Send Reply", "lpw_routing_layer [Send Reply exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_routing_layer [Send Reply exit execs], state8_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_routing_layer [Send Reply exit execs], state8_exit_exec)


			/** state (Send Reply) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "Send Reply", "idle")
				/*---------------------------------------------------------*/



			}


		FSM_EXIT (2,"lpw_routing_layer")
		}
	}




void
lpw_routing_layer_diag (OP_SIM_CONTEXT_ARG_OPT)
	{
	/* No Diagnostic Block */
	}




void
lpw_routing_layer_terminate (OP_SIM_CONTEXT_ARG_OPT)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = __LINE__;
#endif

	FIN_MT (lpw_routing_layer_terminate ())

	Vos_Poolmem_Dealloc_MT (OP_SIM_CONTEXT_THREAD_INDEX_COMMA pr_state_ptr);

	FOUT
	}


/* Undefine shortcuts to state variables to avoid */
/* syntax error in direct access to fields of */
/* local variable prs_ptr in lpw_routing_layer_svar function. */
#undef my_objid
#undef my_node_objid
#undef my_network_objid
#undef my_mac_address
#undef my_dsr_address
#undef my_name
#undef route_cache
#undef request_sent
#undef request_sequence_number
#undef request_seen
#undef my_complex_intrpt_ptr
#undef reply_fifo
#undef reply_seen
#undef received_packet_id_fifo
#undef no_ack_fifo

#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE

#define FIN_PREAMBLE_DEC
#define FIN_PREAMBLE_CODE

VosT_Obtype
lpw_routing_layer_init (int * init_block_ptr)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	VosT_Obtype obtype = OPC_NIL;
	FIN_MT (lpw_routing_layer_init (init_block_ptr))

	Vos_Define_Object (&obtype, "proc state vars (lpw_routing_layer)",
		sizeof (lpw_routing_layer_state), 0, 20);
	*init_block_ptr = 4;

	FRET (obtype)
	}

VosT_Address
lpw_routing_layer_alloc (VOS_THREAD_INDEX_ARG_COMMA VosT_Obtype obtype, int init_block)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	lpw_routing_layer_state * ptr;
	FIN_MT (lpw_routing_layer_alloc (obtype))

	ptr = (lpw_routing_layer_state *)Vos_Alloc_Object_MT (VOS_THREAD_INDEX_COMMA obtype);
	if (ptr != OPC_NIL)
		ptr->_op_current_block = init_block;
	FRET ((VosT_Address)ptr)
	}



void
lpw_routing_layer_svar (void * gen_ptr, const char * var_name, void ** var_p_ptr)
	{
	lpw_routing_layer_state		*prs_ptr;

	FIN_MT (lpw_routing_layer_svar (gen_ptr, var_name, var_p_ptr))

	if (var_name == OPC_NIL)
		{
		*var_p_ptr = (void *)OPC_NIL;
		FOUT
		}
	prs_ptr = (lpw_routing_layer_state *)gen_ptr;

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
	if (strcmp ("my_dsr_address" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_dsr_address);
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
	if (strcmp ("my_complex_intrpt_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_complex_intrpt_ptr);
		FOUT
		}
	if (strcmp ("reply_fifo" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->reply_fifo);
		FOUT
		}
	if (strcmp ("reply_seen" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->reply_seen);
		FOUT
		}
	if (strcmp ("received_packet_id_fifo" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->received_packet_id_fifo);
		FOUT
		}
	if (strcmp ("no_ack_fifo" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->no_ack_fifo);
		FOUT
		}
	*var_p_ptr = (void *)OPC_NIL;

	FOUT
	}

