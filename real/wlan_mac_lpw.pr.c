/* Process model C form file: wlan_mac_lpw.pr.c */
/* Portions of this file copyright 1992-2003 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
const char wlan_mac_lpw_pr_c [] = "MIL_3_Tfile_Hdr_ 100A 30A modeler 7 49B004F6 49B004F6 1 lpw Administrator 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 8f3 1                                                                                                                                                                                                                                                                                                                                                                                                         ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>



/* Header Block */

#include <math.h>
#include "oms_pr.h"
#include "oms_tan.h"
#include "oms_bgutil.h"

/* Definitions to all protocol specific parameters*/
#include "wlan_support.h"

/* Station address assignment definitions.				  */ 
#include "oms_auto_addr_support.h"
#include "oms_dist_support.h"

//$$$$$$$$$$$$$$$$$$ DSR $$$$$$$$$$$$$$$$$$$$$$$$
// define packet Types
#define DATA_PACKET_TYPE 5
#define REQUEST_PACKET_TYPE 7
#define REPLY_PACKET_TYPE 11
#define ERROR_PACKET_TYPE 13
int WLAN_AIR_PROPAGATION_TIME = 0;
// remote intrpt or self intrpt codes definition
#define ACK_CODE 1
#define ERROR_CODE 2
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//#define WLANC_MSDU_HEADER_SIZE 272

/* incoming statistics and stream wires */
#define 	TRANSMITTER_INSTAT				2
#define		RECEIVER_BUSY_INSTAT	    	1
#define		RECEIVER_COLL_INSTAT	    	0
#define		LOW_LAYER_INPUT_STREAM_CH4		3
#define 	LOW_LAYER_OUT_STREAM_CH1		0
#define 	LOW_LAYER_OUT_STREAM_CH2		1
#define 	LOW_LAYER_OUT_STREAM_CH3		2
#define 	LOW_LAYER_OUT_STREAM_CH4		3

/* Flags to load different variables based on attribute settings.	*/
#define		WLAN_AP						1
#define		WLAN_STA					0


//$$$$$$$$$$$$$$$$$$ DSR $$$$$$$$$$$$$$$$$$$$$$$$
Stathandle stat_mac_failed_data;
int mac_failed_data;
Stathandle stat_mac_failed_reply;
int mac_failed_reply;
Stathandle stat_mac_failed_error;
int	mac_failed_error=0;
Stathandle stat_mac_total_failed;
int mac_total_failed=0;
Stathandle stat_mac_retry_rts;
int mac_retry_rts;
Stathandle stat_mac_retry_data;
int mac_retry_data;
Stathandle stat_mac_retry_reply;
int mac_retry_reply;
Stathandle stat_mac_retry_error;
int mac_retry_error;
Stathandle stat_mac_total_retry;
int mac_total_retry;
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

/* Define interrupt codes for generating handling interrupts			*/
/* indicating changes in deference, frame timeout which infers         	*/
/* that the collision has occurred, random backoff and transmission 	*/
/* completion by the physical layer (self interrupts).					*/
typedef enum WlanT_Mac_Intrpt_Code
	{	
	WlanC_Deference_Off,  	/* Deference before frame transmission 				*/
	WlanC_Frame_Timeout,	/* No frame rcvd in set duration (infer collision)	*/
	WlanC_Backoff_Elapsed,  /* Backoff done before frame transmission			*/
	WlanC_CW_Elapsed		/* Bakcoff done after successful frame transmission	*/	
	} WlanT_Mac_Intrpt_Code;

/* Define codes for data and managment frames use in DCF		        */
/* The code defined is consistent with IEEE 802.11 format			    */
/* There are 6 bits used to define the code and in the following        */
/* enumeration the first 6 bits are used in the type field of the frame.*/
//typedef enum WlanT_Mac_Frame_Type
//	{
//	WlanC_Rts  = 0x6C, /* Rts code set into the Rts control frame */
//    WlanC_Cts  = 0x70, /* Cts code set into the Cts control frame */
//	WlanC_Ack  = 0x74, /* Ack code set into the Ack control frame */
 //	WlanC_Data = 0x80, /* Data code set into the Data frame       */
//	WlanC_None = 0x00  /* None type 							  */
//	} WlanT_Mac_Frame_Type;

/* Defining codes for the physical layer characteristics type	*/
typedef enum WlanT_Phy_Char_Code
	{
	WlanC_Frequency_Hopping,			
	WlanC_Direct_Sequence,				
	WlanC_Infra_Red					
	} WlanT_Phy_Char_Code;

/* Define a structure to maintain data fragments received by each 	  */
/* station for the purpose of reassembly (or defragmentation)		  */
typedef struct WlanT_Mac_Defragmentation_Buffer_Entry
	{		
	int			tx_station_address    ;/* Store the station address of transmitting station  		*/	 
	double		time_rcvd		      ;/* Store time the last fragment for this frame was received	*/ 
	Sbhandle	reassembly_buffer_ptr ;/* Store data fragments for a particular packet       		*/  		 
	} WlanT_Mac_Defragmentation_Buffer_Entry;

/* Define a structure to maintain a copy of each unique data frame      */
/* received by the station. This is done so that the station can    	*/
/* discard any additional copies of the frame received by it. 	   		*/
typedef struct WlanT_Mac_Duplicate_Buffer_Entry
	{	
	int         tx_station_address;  /* store the station address of transmitting station	*/
	int 		sequence_id		  ;  /* rcvd packet sequence id 						 	*/	
	int		    fragment_number	  ;  /* rcvd packet fragment number                      	*/	 
	} WlanT_Mac_Duplicate_Buffer_Entry;

/* This structure contains all the flags used in this process model to determine	*/
/* various conditions as mentioned in the comments for each flag					*/
typedef struct WlanT_Mac_Flags
	{
	Boolean 	data_frame_to_send; /* Flag to check when station needs to transmit.		*/ 
	Boolean     backoff_flag;  	    /* Backoff flag is set when either the collision is		*/
	                                /* inferred or the channel switched from busy to idle	*/
	Boolean		rts_sent;   		/* Flag to indicate that wether the Rts for this		*/
								    /* particular data frame is sent						*/
	Boolean		rcvd_bad_packet;	/* Flag to indicate that the received packet is bad		*/
    Boolean	    receiver_busy;		/* Set this flag if receiver busy stat is enabled		*/	
    Boolean	    transmitter_busy;	/* Set this flag if we are transmitting something.		*/	
	Boolean		wait_eifs_dur;		/* Set this flag if the station needs to wait for eifs	*/
									/* duration.											*/	
	Boolean		gateway_flag;		/* Set this flag if the station is a gateway.			*/
	Boolean		bridge_flag;		/* Set this flag if the station is a bridge				*/
	Boolean		immediate_xmt;		/* Set this flag if the new frame can be transmitted	*/
									/* without deferring.									*/
	Boolean		cw_required;		/* Indicates the arrival of an ACK making the 			*/
									/* transmission successful, which requires a CW period.	*/
	Boolean		nav_updated;		/* Indicates a new NAV value since the last time when	*/
									/* self interrupt is scheduled for the end of deference.*/
	} WlanT_Mac_Flags;

/* This structure contains the destination address to which the received */
/* data packet needs to be sent and the contents of the recieved packet  */
/* from the higher layer.												 */
typedef struct WlanT_Hld_List_Elem
	{
	double		time_rcvd;  			/* Time packet is received by the higher layer	*/
	int			destination_address; 	/* Station to which this packet needs to be sent*/
	Packet*     pkptr;				 	/* store packet contents  					  	*/
	} WlanT_Hld_List_Elem;

/**	Macros	Definition														**/
/** The data frame send flag is set whenever there is a data to be send by	**/
/** the higher layer or the response frame needs to be sent. However,in 	**/
/** either case the flag will not be set if the receiver is busy			**/
/** Frames cannot be transmitted until medium is idle. Once, the medium 	**/
/** is available then the station is eligible to transmit provided there	**/
/** is a need for backoff. Once the transmission is complete then the		**/
/** station will wait for the response provided the frame transmitted  		**/
/** requires a response (such as Rts and Data frames). If response			**/
/** is not needed then the station will defer to transmit next packet		**/

/* After receiving a stream interrupt, we need to switch states from	*/
/* idle to defer or transmit if there is a frame to transmit and the	*/
/* receiver is not busy													*/ 
#define READY_TO_TRANSMIT		((intrpt_type == OPC_INTRPT_STRM || (intrpt_type == OPC_INTRPT_SELF && intrpt_code == WlanC_CW_Elapsed)) && \
								(wlan_flags->data_frame_to_send == OPC_BOOLINT_ENABLED || fresp_to_send != WlanC_None) && \
								wlan_flags->receiver_busy == OPC_BOOLINT_DISABLED && \
								(current_time >= cw_end || fresp_to_send != WlanC_None))
	
/* When we have a frame to transmit, we move to transmit state if the	*/
/* medium was idle for at least a DIFS time, otherwise we go to defer	*/
/* state.																*/
#define MEDIUM_IS_IDLE			((current_time - nav_duration >= difs_time) && \
	             				 (wlan_flags->receiver_busy == OPC_BOOLINT_DISABLED) && \
								 (current_time - rcv_idle_time >= difs_time))

/* Change state to Defer from Frm_End, if the input buffers are not empty or a frame needs	*/
/* to be retransmitted or the station has to respond to some frame.							*/		
#define FRAME_TO_TRANSMIT		((wlan_flags->data_frame_to_send == OPC_BOOLINT_ENABLED && current_time >= cw_end) || \
	                             fresp_to_send != WlanC_None || \
	                             retry_count != 0)
	
/* After defering for either collision avoidance or interframe gap      */
/* the channel will be available for transmission 						*/
#define DEFERENCE_OFF			(intrpt_type == OPC_INTRPT_SELF && \
								 intrpt_code == WlanC_Deference_Off && \
								 wlan_flags->receiver_busy == OPC_BOOLINT_DISABLED)

/* Isssue a transmission complete stat once the packet has successfully */
/* been transmitted from the source station								*/						 
#define TRANSMISSION_COMPLETE	(intrpt_type == OPC_INTRPT_STAT && \
								 op_intrpt_stat () == TRANSMITTER_INSTAT)

/* Backoff is performed based on the value of the backoff flag.			*/
#define PERFORM_BACKOFF			(wlan_flags->backoff_flag == OPC_BOOLINT_ENABLED)

/* Need to start transmitting frame once the backoff (self intrpt) 		*/
/* completed															*/
#define BACKOFF_COMPLETED		(intrpt_type == OPC_INTRPT_SELF && \
								 intrpt_code == WlanC_Backoff_Elapsed && \
								 wlan_flags->receiver_busy == OPC_BOOLINT_DISABLED)

/* After transmission the station will wait for a frame response for   */
/* Data and Rts frames.												   */
#define WAIT_FOR_FRAME          (expected_frame_type != WlanC_None)

/* Need to retransmit frame if there is a frame timeout and the        */
/* required frame is not received									   */
#define FRAME_TIMEOUT           (intrpt_type == OPC_INTRPT_SELF && intrpt_code == WlanC_Frame_Timeout)

/* If the frame is received appropriate response will be transmitted   */
/* provided the medium is considered to be idle						   */
#define FRAME_RCVD			    (intrpt_type == OPC_INTRPT_STRM && wlan_flags->rcvd_bad_packet == OPC_BOOLINT_DISABLED && \
		 						 i_strm <= LOW_LAYER_INPUT_STREAM_CH4)

/* Skip backoff if no backoff is needed								   */
#define TRANSMIT_FRAME			(wlan_flags->backoff_flag == OPC_BOOLINT_DISABLED)

/* Expecting frame response	after data or Rts transmission			   */
#define EXPECTING_FRAME			(expected_frame_type != WlanC_None)

/* Macros that check the change in the busy status of the receiver.	   */
#define	RECEIVER_BUSY_HIGH		(intrpt_type == OPC_INTRPT_STAT && intrpt_code == RECEIVER_BUSY_INSTAT && \
								 op_stat_local_read (RECEIVER_BUSY_INSTAT) == 1.0)
#define	RECEIVER_BUSY_LOW		(intrpt_type == OPC_INTRPT_STAT && intrpt_code == RECEIVER_BUSY_INSTAT && \
								 op_stat_local_read (RECEIVER_BUSY_INSTAT) == 0.0)

/* Function declarations.	*/
static void			wlan_mac_sv_init ();
static void			wlan_higher_layer_data_arrival ();
static void			wlan_physical_layer_data_arrival ();
static void			wlan_hlpk_enqueue (Packet* hld_pkptr, int dest_addr);
Boolean 			wlan_tuple_find (int sta_addr, int seq_id, int frag_num);
static void			wlan_data_process (Packet* seg_pkptr, int sta_addr, int final_dest_addr, int frag_num, int more_frag, int pkt_id, int rcvd_sta_bssid);
static void			wlan_accepted_frame_stats_update (Packet* seg_pkptr);
static void			wlan_interrupts_process ();
static void 		wlan_prepare_frame_to_send (int frame_type);
static void			wlan_frame_transmit ();
static void			wlan_schedule_deference ();
static void			wlan_frame_discard ();
static void			wlan_mac_error (char* msg1, char* msg2, char* msg3);


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
	int	                    		retry_count;
	int	                    		intrpt_type;
	WlanT_Mac_Intrpt_Code	  		intrpt_code;
	int	                    		my_address;
	List*	                  		hld_list_ptr;
	double	                 		operational_speed;
	int	                    		frag_threshold;
	int	                    		packet_seq_number;
	int	                    		packet_frag_number;
	int	                    		destination_addr;
	Sbhandle	               		fragmentation_buffer_ptr;
	WlanT_Mac_Frame_Type	   		fresp_to_send;
	double	                 		nav_duration;
	int	                    		rts_threshold;
	int	                    		duplicate_entry;
	WlanT_Mac_Frame_Type	   		expected_frame_type;
	int	                    		remote_sta_addr;
	double	                 		backoff_slots;
	Stathandle	             		packet_load_handle;
	double	                 		intrpt_time;
	Packet *	               		wlan_transmit_frame_copy_ptr;
	Stathandle	             		backoff_slots_handle;
	int	                    		instrm_from_mac_if;
	int	                    		outstrm_to_mac_if;
	int	                    		num_fragments;
	int	                    		remainder_size;
	List*	                  		defragmentation_list_ptr;
	WlanT_Mac_Flags*	       		wlan_flags;
	OmsT_Aa_Address_Handle	 		oms_aa_handle;
	double	                 		current_time;
	double	                 		rcv_idle_time;
	double	                 		cw_end;
	WlanT_Mac_Duplicate_Buffer_Entry**			duplicate_list_ptr;
	Pmohandle	              		hld_pmh;
	int	                    		max_backoff;
	char	                   		current_state_name [32];
	Stathandle	             		hl_packets_rcvd;
	Stathandle	             		media_access_delay;
	Stathandle	             		ete_delay_handle;
	Stathandle	             		global_ete_delay_handle;
	Stathandle	             		global_throughput_handle;
	Stathandle	             		global_load_handle;
	Stathandle	             		global_dropped_data_handle;
	Stathandle	             		global_mac_delay_handle;
	Stathandle	             		ctrl_traffic_rcvd_handle_inbits;
	Stathandle	             		ctrl_traffic_sent_handle_inbits;
	Stathandle	             		ctrl_traffic_rcvd_handle;
	Stathandle	             		ctrl_traffic_sent_handle;
	Stathandle	             		data_traffic_rcvd_handle_inbits;
	Stathandle	             		data_traffic_sent_handle_inbits;
	Stathandle	             		data_traffic_rcvd_handle;
	Stathandle	             		data_traffic_sent_handle;
	double	                 		sifs_time;
	double	                 		slot_time;
	int	                    		cw_min;
	int	                    		cw_max;
	double	                 		difs_time;
	Stathandle	             		channel_reserv_handle;
	Stathandle	             		retrans_handle;
	Stathandle	             		throughput_handle;
	int	                    		long_retry_limit;
	int	                    		short_retry_limit;
	int	                    		retry_limit;
	WlanT_Mac_Frame_Type	   		last_frametx_type;
	Evhandle	               		deference_evh;
	Evhandle	               		backoff_elapsed_evh;
	Evhandle	               		frame_timeout_evh;
	Evhandle	               		cw_end_evh;
	double	                 		eifs_time;
	int	                    		i_strm;
	Boolean	                		wlan_trace_active;
	int	                    		pkt_in_service;
	Stathandle	             		bits_load_handle;
	int	                    		ap_flag;
	int	                    		bss_flag;
	int	                    		bss_id;
	int	                    		hld_max_size;
	double	                 		max_receive_lifetime;
	WlanT_Phy_Char_Code	    		phy_char_flag;
	OmsT_Aa_Address_Handle	 		oms_aa_wlan_handle;
	int	                    		total_hlpk_size;
	Stathandle	             		drop_packet_handle;
	Stathandle	             		drop_packet_handle_inbits;
	Log_Handle	             		drop_pkt_log_handle;
	Boolean	                		drop_pkt_entry_log_flag;
	int	                    		packet_size;
	double	                 		receive_time;
	Ici*	                   		llc_iciptr;
	int*	                   		bss_stn_list;
	int	                    		bss_stn_count;
	int	                    		data_packet_type;
	int	                    		data_packet_dest;
	int	                    		data_packet_final_dest;
	} wlan_mac_lpw_state;

#define pr_state_ptr            		((wlan_mac_lpw_state*) (OP_SIM_CONTEXT_PTR->mod_state_ptr))
#define retry_count             		pr_state_ptr->retry_count
#define intrpt_type             		pr_state_ptr->intrpt_type
#define intrpt_code             		pr_state_ptr->intrpt_code
#define my_address              		pr_state_ptr->my_address
#define hld_list_ptr            		pr_state_ptr->hld_list_ptr
#define operational_speed       		pr_state_ptr->operational_speed
#define frag_threshold          		pr_state_ptr->frag_threshold
#define packet_seq_number       		pr_state_ptr->packet_seq_number
#define packet_frag_number      		pr_state_ptr->packet_frag_number
#define destination_addr        		pr_state_ptr->destination_addr
#define fragmentation_buffer_ptr		pr_state_ptr->fragmentation_buffer_ptr
#define fresp_to_send           		pr_state_ptr->fresp_to_send
#define nav_duration            		pr_state_ptr->nav_duration
#define rts_threshold           		pr_state_ptr->rts_threshold
#define duplicate_entry         		pr_state_ptr->duplicate_entry
#define expected_frame_type     		pr_state_ptr->expected_frame_type
#define remote_sta_addr         		pr_state_ptr->remote_sta_addr
#define backoff_slots           		pr_state_ptr->backoff_slots
#define packet_load_handle      		pr_state_ptr->packet_load_handle
#define intrpt_time             		pr_state_ptr->intrpt_time
#define wlan_transmit_frame_copy_ptr		pr_state_ptr->wlan_transmit_frame_copy_ptr
#define backoff_slots_handle    		pr_state_ptr->backoff_slots_handle
#define instrm_from_mac_if      		pr_state_ptr->instrm_from_mac_if
#define outstrm_to_mac_if       		pr_state_ptr->outstrm_to_mac_if
#define num_fragments           		pr_state_ptr->num_fragments
#define remainder_size          		pr_state_ptr->remainder_size
#define defragmentation_list_ptr		pr_state_ptr->defragmentation_list_ptr
#define wlan_flags              		pr_state_ptr->wlan_flags
#define oms_aa_handle           		pr_state_ptr->oms_aa_handle
#define current_time            		pr_state_ptr->current_time
#define rcv_idle_time           		pr_state_ptr->rcv_idle_time
#define cw_end                  		pr_state_ptr->cw_end
#define duplicate_list_ptr      		pr_state_ptr->duplicate_list_ptr
#define hld_pmh                 		pr_state_ptr->hld_pmh
#define max_backoff             		pr_state_ptr->max_backoff
#define current_state_name      		pr_state_ptr->current_state_name
#define hl_packets_rcvd         		pr_state_ptr->hl_packets_rcvd
#define media_access_delay      		pr_state_ptr->media_access_delay
#define ete_delay_handle        		pr_state_ptr->ete_delay_handle
#define global_ete_delay_handle 		pr_state_ptr->global_ete_delay_handle
#define global_throughput_handle		pr_state_ptr->global_throughput_handle
#define global_load_handle      		pr_state_ptr->global_load_handle
#define global_dropped_data_handle		pr_state_ptr->global_dropped_data_handle
#define global_mac_delay_handle 		pr_state_ptr->global_mac_delay_handle
#define ctrl_traffic_rcvd_handle_inbits		pr_state_ptr->ctrl_traffic_rcvd_handle_inbits
#define ctrl_traffic_sent_handle_inbits		pr_state_ptr->ctrl_traffic_sent_handle_inbits
#define ctrl_traffic_rcvd_handle		pr_state_ptr->ctrl_traffic_rcvd_handle
#define ctrl_traffic_sent_handle		pr_state_ptr->ctrl_traffic_sent_handle
#define data_traffic_rcvd_handle_inbits		pr_state_ptr->data_traffic_rcvd_handle_inbits
#define data_traffic_sent_handle_inbits		pr_state_ptr->data_traffic_sent_handle_inbits
#define data_traffic_rcvd_handle		pr_state_ptr->data_traffic_rcvd_handle
#define data_traffic_sent_handle		pr_state_ptr->data_traffic_sent_handle
#define sifs_time               		pr_state_ptr->sifs_time
#define slot_time               		pr_state_ptr->slot_time
#define cw_min                  		pr_state_ptr->cw_min
#define cw_max                  		pr_state_ptr->cw_max
#define difs_time               		pr_state_ptr->difs_time
#define channel_reserv_handle   		pr_state_ptr->channel_reserv_handle
#define retrans_handle          		pr_state_ptr->retrans_handle
#define throughput_handle       		pr_state_ptr->throughput_handle
#define long_retry_limit        		pr_state_ptr->long_retry_limit
#define short_retry_limit       		pr_state_ptr->short_retry_limit
#define retry_limit             		pr_state_ptr->retry_limit
#define last_frametx_type       		pr_state_ptr->last_frametx_type
#define deference_evh           		pr_state_ptr->deference_evh
#define backoff_elapsed_evh     		pr_state_ptr->backoff_elapsed_evh
#define frame_timeout_evh       		pr_state_ptr->frame_timeout_evh
#define cw_end_evh              		pr_state_ptr->cw_end_evh
#define eifs_time               		pr_state_ptr->eifs_time
#define i_strm                  		pr_state_ptr->i_strm
#define wlan_trace_active       		pr_state_ptr->wlan_trace_active
#define pkt_in_service          		pr_state_ptr->pkt_in_service
#define bits_load_handle        		pr_state_ptr->bits_load_handle
#define ap_flag                 		pr_state_ptr->ap_flag
#define bss_flag                		pr_state_ptr->bss_flag
#define bss_id                  		pr_state_ptr->bss_id
#define hld_max_size            		pr_state_ptr->hld_max_size
#define max_receive_lifetime    		pr_state_ptr->max_receive_lifetime
#define phy_char_flag           		pr_state_ptr->phy_char_flag
#define oms_aa_wlan_handle      		pr_state_ptr->oms_aa_wlan_handle
#define total_hlpk_size         		pr_state_ptr->total_hlpk_size
#define drop_packet_handle      		pr_state_ptr->drop_packet_handle
#define drop_packet_handle_inbits		pr_state_ptr->drop_packet_handle_inbits
#define drop_pkt_log_handle     		pr_state_ptr->drop_pkt_log_handle
#define drop_pkt_entry_log_flag 		pr_state_ptr->drop_pkt_entry_log_flag
#define packet_size             		pr_state_ptr->packet_size
#define receive_time            		pr_state_ptr->receive_time
#define llc_iciptr              		pr_state_ptr->llc_iciptr
#define bss_stn_list            		pr_state_ptr->bss_stn_list
#define bss_stn_count           		pr_state_ptr->bss_stn_count
#define data_packet_type        		pr_state_ptr->data_packet_type
#define data_packet_dest        		pr_state_ptr->data_packet_dest
#define data_packet_final_dest  		pr_state_ptr->data_packet_final_dest

/* These macro definitions will define a local variable called	*/
/* "op_sv_ptr" in each function containing a FIN statement.	*/
/* This variable points to the state variable data structure,	*/
/* and can be used from a C debugger to display their values.	*/
#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE
#if defined (OPD_PARALLEL)
#  define FIN_PREAMBLE_DEC	wlan_mac_lpw_state *op_sv_ptr; OpT_Sim_Context * tcontext_ptr;
#  define FIN_PREAMBLE_CODE	\
		if (VosS_Mt_Perform_Lock) \
			VOS_THREAD_SPECIFIC_DATA_GET (VosI_Globals.simi_mt_context_data_key, tcontext_ptr, SimT_Context *); \
		else \
			tcontext_ptr = VosI_Globals.simi_sequential_context_ptr; \
		op_sv_ptr = ((wlan_mac_lpw_state *)(tcontext_ptr->mod_state_ptr));
#else
#  define FIN_PREAMBLE_DEC	wlan_mac_lpw_state *op_sv_ptr;
#  define FIN_PREAMBLE_CODE	op_sv_ptr = pr_state_ptr;
#endif


/* Function Block */


#if !defined (VOSD_NO_FIN)
enum { _op_block_origin = __LINE__ };
#endif
static void
wlan_mac_sv_init ()
	{
	Objid					mac_params_comp_attr_objid;
	Objid					params_attr_objid;
	Objid					phy_params_comp_attr_objid;
	Objid					my_objid;
	Objid					my_node_objid;
	Objid					my_subnet_objid;
	Objid					rx_objid;
	Objid					tx_objid;
	Objid					chann_params_comp_attr_objid;
	Objid					subchann_params_attr_objid;
	Objid					chann_objid;
	Objid					sub_chann_objid;
	int						num_chann;
	char					subnet_name [512];
	double					bandwidth;
	double					frequency;
	int						ap1_flag, i ;

	/**	1. Initialize state variables.					**/
	/** 2. Read model attribute values in variables.	**/
	/** 3. Create global lists							**/
	/** 4. Register statistics handlers					**/
	FIN (wlan_mac_sv_init ());

	/* object id of the surrounding processor.	*/
	my_objid = op_id_self ();

	/* Obtain the node's object identifier	*/
	my_node_objid = op_topo_parent (my_objid);

	/* Obtain subnet objid.					*/
	my_subnet_objid = op_topo_parent (my_node_objid);

	/* Obtain the values assigned to the various attributes	*/
	op_ima_obj_attr_get (my_objid, "Wireless LAN Parameters", &mac_params_comp_attr_objid);
    params_attr_objid = op_topo_child (mac_params_comp_attr_objid, OPC_OBJTYPE_GENERIC, 0);

	/* Determine the assigned MAC address.	*/
	op_ima_obj_attr_get (my_objid, "station_address", &my_address);

	/* Obtain an address handle for resolving WLAN MAC addresses.	*/
	oms_aa_handle = oms_aa_address_handle_get ("MAC Addresses", "station_address");

	/* Creating a pool of station addresses for each subnet based on subnet name.	*/
	op_ima_obj_attr_get (my_subnet_objid, "name", &subnet_name);
	oms_aa_wlan_handle = oms_aa_address_handle_get (subnet_name, "station_address");
	
	/* Get model attributes.	*/
	op_ima_obj_attr_get (params_attr_objid, "Data Rate", &operational_speed);
	op_ima_obj_attr_get (params_attr_objid, "Fragmentation Threshold", &frag_threshold);
	op_ima_obj_attr_get (params_attr_objid, "Rts Threshold", &rts_threshold);
	op_ima_obj_attr_get (params_attr_objid, "Short Retry Limit", &short_retry_limit);
	op_ima_obj_attr_get (params_attr_objid, "Long Retry Limit", &long_retry_limit);
	op_ima_obj_attr_get (params_attr_objid, "Access Point Functionality", &ap_flag);	
	op_ima_obj_attr_get (params_attr_objid, "Buffer Size", &hld_max_size);
	op_ima_obj_attr_get (params_attr_objid, "Max Receive Lifetime", &max_receive_lifetime);	

	/* Initialize the retry limit for the current frame to long retry limit.	*/
	retry_limit = long_retry_limit;
	
	/* Get the Channel Settings.										*/
	/* Extracting Channel 0,1,2,3 (i.e. 1,2,5.5 and 11Mbps) Settings	*/
	op_ima_obj_attr_get (params_attr_objid, "Channel Settings", &chann_params_comp_attr_objid);
	subchann_params_attr_objid = op_topo_child (chann_params_comp_attr_objid, OPC_OBJTYPE_GENERIC, 0);
	op_ima_obj_attr_get (subchann_params_attr_objid, "Bandwidth", &bandwidth);	
	op_ima_obj_attr_get (subchann_params_attr_objid, "Min Frequency", &frequency);	

	/* Load the appropriate physical layer characteristics.	*/	
	op_ima_obj_attr_get (params_attr_objid, "Physical Characteristics", &phy_char_flag);

	/* Based on physical charateristics settings set appropriate values to the variables.	*/
	switch (phy_char_flag)
		{
		case WlanC_Frequency_Hopping:
			{
			/* Slot duration in terms of sec.	*/
			slot_time = 5E-05;

			/* Short interframe gap in terms of sec.	*/
			sifs_time = 2.8E-05;

			/* Minimum contention window size for selecting backoff slots.	*/
			cw_min = 15;

			/* Maximum contention window size for selecting backoff slots.	*/
			cw_max = 1023;
			break;
			}

		case WlanC_Direct_Sequence:
			{
			/* Slot duration in terms of sec.	*/
			slot_time = 2E-05;

			/* Short interframe gap in terms of sec.	*/
			sifs_time = 1E-05;

			/* Minimum contention window size for selecting backoff slots.	*/
			cw_min = 31;

			/* Maximum contention window size for selecting backoff slots.	*/
			cw_max = 1023;
			break;
			}

		case WlanC_Infra_Red:
			{
			/* Slot duration in terms of sec.	*/
			slot_time = 8E-06;

			/* Short interframe gap in terms of sec.	*/
			sifs_time = 1E-05;

			/* Minimum contention window size for selecting backoff slots.	*/
			cw_min = 63;

			/* Maximum contention window size for selecting backoff slots.	*/
			cw_max = 1023;
			break;
			}

		default:
			{
			wlan_mac_error ("Unexpected Physical Layer Characteristic encountered.", OPC_NIL, OPC_NIL);
			break;
			}
		}

	/* By default stations are configured for IBSS unless an Access Point is found,	*/
	/* then the network will have an infrastructure BSS configuration.				*/
	bss_flag = OPC_BOOLINT_DISABLED;

	/* Computing DIFS interval which is interframe gap between successive	*/
	/* frame transmissions.													*/
	difs_time = sifs_time + 2 * slot_time;

	/* If the receiver detects that the received frame is erroneous then it	*/
	/* will set the network allocation vector to EIFS duration. 			*/
	eifs_time = difs_time + sifs_time + (8 * WLAN_ACK_DURATION) ;
	
	/* Creating list to store data arrived from higher layer.	*/	
	hld_list_ptr = op_prg_list_create ();

	/* Initialize segmentation and reassembly buffer.	*/
	defragmentation_list_ptr = op_prg_list_create ();
	fragmentation_buffer_ptr = op_sar_buf_create (OPC_SAR_BUF_TYPE_SEGMENT, OPC_SAR_BUF_OPT_PK_BNDRY);

	/* Registering local statistics.	*/
	packet_load_handle				= op_stat_reg ("Wireless Lan.Load (packets)", 					  OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	bits_load_handle				= op_stat_reg ("Wireless Lan.Load (bits/sec)", 					  OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	hl_packets_rcvd					= op_stat_reg ("Wireless Lan.Hld Queue Size (packets)", 		  OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	backoff_slots_handle			= op_stat_reg ("Wireless Lan.Backoff Slots (slots)", 			  OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	data_traffic_sent_handle 		= op_stat_reg ("Wireless Lan.Data Traffic Sent (packets/sec)", 	  OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);	
	data_traffic_rcvd_handle		= op_stat_reg ("Wireless Lan.Data Traffic Rcvd (packets/sec)", 	  OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL); 
	data_traffic_sent_handle_inbits	= op_stat_reg ("Wireless Lan.Data Traffic Sent (bits/sec)", 	  OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	data_traffic_rcvd_handle_inbits	= op_stat_reg ("Wireless Lan.Data Traffic Rcvd (bits/sec)", 	  OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	ctrl_traffic_sent_handle	 	= op_stat_reg ("Wireless Lan.Control Traffic Sent (packets/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	ctrl_traffic_rcvd_handle		= op_stat_reg ("Wireless Lan.Control Traffic Rcvd (packets/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL); 
	ctrl_traffic_sent_handle_inbits	= op_stat_reg ("Wireless Lan.Control Traffic Sent (bits/sec)",    OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	ctrl_traffic_rcvd_handle_inbits	= op_stat_reg ("Wireless Lan.Control Traffic Rcvd (bits/sec)", 	  OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL); 
	drop_packet_handle       		= op_stat_reg ("Wireless Lan.Dropped Data Packets (packets/sec)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL); 
	drop_packet_handle_inbits		= op_stat_reg ("Wireless Lan.Dropped Data Packets (bits/sec)", 	  OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL); 
	retrans_handle					= op_stat_reg ("Wireless Lan.Retransmission Attempts (packets)",  OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL); 
	media_access_delay				= op_stat_reg ("Wireless Lan.Media Access Delay (sec)", 		  OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	ete_delay_handle				= op_stat_reg ("Wireless Lan.Delay (sec)", 					 	  OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	channel_reserv_handle			= op_stat_reg ("Wireless Lan.Channel Reservation (sec)", 		  OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
	throughput_handle				= op_stat_reg ("Wireless Lan.Throughput (bits/sec)", 			  OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);

	/* Registering global statistics.	*/
	global_ete_delay_handle 		= op_stat_reg ("Wireless LAN.Delay (sec)", 	  		    OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
	global_load_handle 				= op_stat_reg ("Wireless LAN.Load (bits/sec)", 		    OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
	global_throughput_handle 		= op_stat_reg ("Wireless LAN.Throughput (bits/sec)",    OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
	global_dropped_data_handle		= op_stat_reg ("Wireless LAN.Data Dropped (bits/sec)",  OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
	global_mac_delay_handle			= op_stat_reg ("Wireless LAN.Media Access Delay (sec)", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
	
	//$$$$$$$$$$$$$$$$$$ DSR $$$$$$$$$$$$$$$$$$$$$$$$
	stat_mac_failed_data=op_stat_reg("Mac Failed Data",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
	op_stat_write(stat_mac_failed_data,mac_failed_data=0);
	stat_mac_failed_reply=op_stat_reg("Mac Failed Reply",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
	op_stat_write(stat_mac_failed_reply,mac_failed_reply=0);
	stat_mac_failed_error=op_stat_reg("Mac Failed Error",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
	op_stat_write(stat_mac_failed_error,mac_failed_error=0);
	stat_mac_total_failed=op_stat_reg("Mac Total Failed",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
	op_stat_write(stat_mac_total_failed,mac_total_failed=0);
	stat_mac_retry_rts=op_stat_reg("Mac Retry Rts",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
	op_stat_write(stat_mac_retry_rts,mac_retry_rts=0);
	stat_mac_retry_data=op_stat_reg("Mac Retry Data",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
	op_stat_write(stat_mac_retry_data,mac_retry_data=0);
	stat_mac_retry_reply=op_stat_reg("Mac Retry Reply",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
	op_stat_write(stat_mac_retry_reply,mac_retry_reply=0);
	stat_mac_retry_error=op_stat_reg("Mac Retry Error",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
	op_stat_write(stat_mac_retry_error,mac_retry_error=0);
	stat_mac_total_retry=op_stat_reg("Mac Total Retry",OPC_STAT_INDEX_NONE,OPC_STAT_GLOBAL);
	op_stat_write(stat_mac_total_retry,mac_total_retry=0);
	//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
	
	/* Registering log handles */
	drop_pkt_log_handle	= op_prg_log_handle_create (OpC_Log_Category_Configuration, "Wireless Lan", "Data packet Drop", 128);
    drop_pkt_entry_log_flag = OPC_FALSE;

	/* Allocating memory for the flags used in this process model. */
	wlan_flags = (WlanT_Mac_Flags *) op_prg_mem_alloc (sizeof (WlanT_Mac_Flags));

	/* Disabling all flags as a default.	*/
	wlan_flags->data_frame_to_send 	= OPC_BOOLINT_DISABLED;
	wlan_flags->backoff_flag       	= OPC_BOOLINT_DISABLED;
	wlan_flags->rts_sent		   	= OPC_BOOLINT_DISABLED;
	wlan_flags->rcvd_bad_packet		= OPC_BOOLINT_DISABLED;
	wlan_flags->receiver_busy		= OPC_BOOLINT_DISABLED;
	wlan_flags->transmitter_busy	= OPC_BOOLINT_DISABLED;
	wlan_flags->gateway_flag		= OPC_BOOLINT_DISABLED;
	wlan_flags->bridge_flag			= OPC_BOOLINT_DISABLED;
	wlan_flags->wait_eifs_dur		= OPC_BOOLINT_DISABLED;
	wlan_flags->immediate_xmt		= OPC_BOOLINT_DISABLED;
	wlan_flags->cw_required			= OPC_BOOLINT_DISABLED;
	wlan_flags->nav_updated			= OPC_BOOLINT_DISABLED;

	/* Intialize retry count. */
	retry_count = 0;

	/* Initialize the packet pointer that holds the last		*/
	/* transmitted packet to be used for retransmissions when	*/
	/* necessary.												*/
	wlan_transmit_frame_copy_ptr = OPC_NIL;
	
	/* Initialize nav duration	*/
	nav_duration = 0;
	
	/* Initialize receiver idle and conetion window timers.	*/
	rcv_idle_time = -2.0 * difs_time;
	cw_end =        0.0;

	/* Initializing the sum of sizes of the packets in the higher layer queue.	*/
	total_hlpk_size = 0;

	/* Initialize the state variables related with the current frame that is being handled.	*/
	packet_size  = 0;
	receive_time = 0.0;
	
	/* Data arrived from higher layer is queued in the buffer. Pool memory is used for		*/
	/* allocating data structure for the higher layer packet and the random destination		*/
	/* for the packet. This structure is then inserted in the higher layer arrival queue.	*/
	hld_pmh = op_prg_pmo_define ("WLAN hld list elements", sizeof (WlanT_Hld_List_Elem), 32);

	/* Obtaining transmitter objid for accessing channel data rate attribute.	*/
	tx_objid = op_topo_assoc (my_objid, OPC_TOPO_ASSOC_OUT, OPC_OBJTYPE_RATX, 0);

	/* If no receiver is attach then generate error message and abort the simulation.	*/
	if (tx_objid == OPC_OBJID_INVALID)
		{
		wlan_mac_error ("No transmitter attached to this MAC process", OPC_NIL, OPC_NIL);	
		}

	/* Obtaining number of channels available.	*/
	op_ima_obj_attr_get (tx_objid, "channel", &chann_objid);
	num_chann = op_topo_child_count (chann_objid, OPC_OBJTYPE_RATXCH);
	
	/* Generate error message and terminate simulation if no channel is available for transmission.	*/
	if (num_chann == 0)
		{
		wlan_mac_error (" No channel is available for transmission", OPC_NIL, OPC_NIL);
		}

	/* Setting the Frequency and Bandwidth for the transmitting channels.	*/
	for (i = 0; i < num_chann; i++)
		{ 
		/* Accessing channel to set the frequency and bandwidth.	*/
		sub_chann_objid = op_topo_child (chann_objid, OPC_OBJTYPE_RATXCH, i);

		/* Setting the operating freqeuncy and channel bandwidth for the transmitting channels.	*/	
		op_ima_obj_attr_set (sub_chann_objid, "bandwidth", bandwidth);
		op_ima_obj_attr_set (sub_chann_objid, "min frequency", frequency);
		}

	/* Obtaining receiver's objid for accessing channel data rate attribute.	*/
	rx_objid = op_topo_assoc (my_objid, OPC_TOPO_ASSOC_IN, OPC_OBJTYPE_RARX, 0);

	/* If no receiver is attach then generate error message and abort the simulation.	*/
	if (rx_objid == OPC_OBJID_INVALID)
		{
		wlan_mac_error ("No receiver attached to this MAC process", OPC_NIL, OPC_NIL);	
		}

	/* Obtaining number of channels available.	*/
	op_ima_obj_attr_get (rx_objid, "channel", &chann_objid);
	num_chann = op_topo_child_count (chann_objid, OPC_OBJTYPE_RARXCH);
	
	/* Generate error message and terminate simulation if no channel is available for reception.	*/
	if (num_chann == 0)
		{
		wlan_mac_error (" No channel is available for reception", OPC_NIL, OPC_NIL);
		}

	/* Setting the Frequency and Bandwidth for the transmitting channels.	*/
	for (i = 0; i < num_chann; i++)
		{ 	
		/* Accessing channel to set the frequency and bandwidth.	*/
		sub_chann_objid = op_topo_child (chann_objid, OPC_OBJTYPE_RARXCH, i);
	
		/* Setting the operating freqeuncy and channel bandwidth for the receiving channels.	*/
		op_ima_obj_attr_set (sub_chann_objid, "bandwidth", bandwidth);
		op_ima_obj_attr_set (sub_chann_objid, "min frequency", frequency);
		}
	
	llc_iciptr = op_ici_create ("wlan_mac_ind");

	if (llc_iciptr == OPC_NIL)
		{
		wlan_mac_error ("Unable to create ICI for communication with LLC.", OPC_NIL, OPC_NIL);
		}

	FOUT;
	}

static void
wlan_higher_layer_data_arrival ()
	{
	Packet*					hld_pkptr;
	int						pk_size,i;
	int						dest_addr;
	Ici*					ici_ptr;
	Boolean					stn_det_flag;

	/** Queue the packet as it arrives from higher layer.	**/
	/** Also, store the destination address of the packet	**/
	/** in the queue and the arrival time.					**/
	FIN (wlan_higher_layer_data_arrival ());

	/* Get packet from the incomming stream from higher layer and	*/
	/* obtain the packet size 										*/
	hld_pkptr = op_pk_get (op_intrpt_strm ());
	/* For bridge and gateway, only accept packet from the higher	*/
	/* layer if the access point functionality is enabled.			*/
	if (((wlan_flags->gateway_flag == OPC_BOOLINT_ENABLED) || 
		(wlan_flags->bridge_flag == OPC_BOOLINT_ENABLED)) &&
		(ap_flag == OPC_BOOLINT_DISABLED))
		{
		op_pk_destroy (hld_pkptr);
		FOUT;
		}

	pk_size   = op_pk_total_size_get (hld_pkptr);		

	/* maintaining total packet size of the packets in the higer layer queue.	*/
	total_hlpk_size = total_hlpk_size + pk_size;

	/* If fragmentation is enabled and packet size is greater than the threshold		*/
	/* then MSDU length will not be more than fragmentation threshold, hence			*/
	/* the packet will be fragmented into the size less than or equal to fragmentaion   */
	/* threshold.																		*/
	if ((pk_size > frag_threshold * 8) && (frag_threshold != -1))
		{
		 pk_size = frag_threshold * 8;
		}

	/* Destroy packet if it is more than max msdu length or its		*/
	/* size zero. Also, if the size of the higher layer queue  		*/
	/* will exceed its maximum after the insertion of this packet, 	*/
	/* then discard the arrived packet. 							*/
	/* The higher layer is reponsible for the retransmission of 	*/
	/* this packet.													*/ 
	if (pk_size > WLAN_MAXMSDU_LENGTH || pk_size == 0 ||
       total_hlpk_size > hld_max_size)
		{
		/* change the total hld queue size to original value	*/
		/* as this packet will not be added to the queue.		*/
		total_hlpk_size = total_hlpk_size - pk_size;
			
		if (drop_pkt_entry_log_flag == OPC_FALSE)
			{
			if (total_hlpk_size > hld_max_size)
				{
				/* Writing log message for dropped packets.	*/
				op_prg_log_entry_write (drop_pkt_log_handle, 
				"SYMPTOMS(S):\n"
			    " Wireless LAN MAC layer discarded some packets due to\n "
			    " insufficient buffer capacity. \n"
				"\n"
			    " This may lead to: \n"
  			    " - application data loss.\n"
			    " - higher layer packet retransmission.\n"
			    "\n"
			    " REMDEDIAL ACTION (S): \n"
			    " 1. Reduce Network laod. \n"
			    " 2. User higher speed wireless lan. \n"
			    " 3. Increase buffer capacity\n");
				}
			
			if (pk_size > WLAN_MAXMSDU_LENGTH)
				{
				/* Writing log message for dropped packets due to packet size.	*/
				op_prg_log_entry_write (drop_pkt_log_handle, 
				"SYMPTOMS(S):\n"
			    " Wireless LAN MAC layer discarded some packets due to\n "
			    " large packet size. \n"
				"\n"
			    " This may lead to: \n"
  			    " - application data loss.\n"
			    " - higher layer packet retransmission.\n"
			    "\n"
			    " REMDEDIAL ACTION (S): \n"
			    " 1. Enable fragmentation threshold. \n"
			    " 2. Set the higher layer packet size to \n"
				   " be smaller than max MSDU size \n");
				 }
			drop_pkt_entry_log_flag = OPC_TRUE;
			}

	/* Limit the sum of the sizes of all the packets in the queue to be 	*/
	/* the maximum buffer size.												*/
	if (total_hlpk_size >= hld_max_size)
		{
		total_hlpk_size = hld_max_size;
		}
 
		/* Report stat when data packet is dropped due to overflow buffer.	*/
		op_stat_write (drop_packet_handle, 1.0);
	    op_stat_write (drop_packet_handle, 0.0);

		/* Report stat when data packet is dropped due to overflow buffer.	*/
		op_stat_write (drop_packet_handle_inbits, (double) (pk_size));
	    op_stat_write (drop_packet_handle_inbits, 0.0);
		op_stat_write (global_dropped_data_handle, (double) (pk_size));
		op_stat_write (global_dropped_data_handle, 0.0);
		
		FOUT; 
		}
		
	/* Read ICI parameters at the stream interrupt.	*/
	ici_ptr = op_intrpt_ici ();

	/* Retrieve destination address from the ici set by the interface layer.	*/
	if (ici_ptr == OPC_NIL || op_ici_attr_get (ici_ptr, "dest_addr", &dest_addr) == OPC_COMPCODE_FAILURE)
		{
		/* Generate error message.	*/
		wlan_mac_error ("Destination address in not valid.", OPC_NIL, OPC_NIL);
		}

	/* If it is a broadcast packet or the station doesn't exist in the subnet	*/  
	/*if ((dest_addr < 0) || (oms_aa_address_find (oms_aa_wlan_handle, dest_addr) < 0))*/
	// ############ DEBUG: need broadcasting thus <-1 instead <0
	if (dest_addr < -1)
	   {
	   /* change the total hld queue size to original value	*/
	   /* as this packet will not be added to the queue.		*/
	   total_hlpk_size = total_hlpk_size - pk_size;
	   op_pk_destroy (hld_pkptr);
	   FOUT;		
	   }
		
	   
	/* Check for an AP bridge that whether the destined stations exist in the BSS or not	*/
	/* if not then no need to broadcast the packet.											*/
	if (wlan_flags->bridge_flag == OPC_BOOLINT_ENABLED && ap_flag == OPC_BOOLINT_ENABLED)
		{
		stn_det_flag = OPC_FALSE;
		for (i = 0; i < bss_stn_count; i++)
			{
			if (dest_addr == bss_stn_list [i])
				{
				stn_det_flag = OPC_TRUE;
				}
			}
		
		/* If the destination station doesn't exist in the BSS then */
		/* no need to broadcast the packet.							*/
		if (stn_det_flag == OPC_FALSE)
			{
			/* change the total hld queue size to original value	*/
			/* as this packet will not be added to the queue.		*/
			total_hlpk_size = total_hlpk_size - pk_size;
			op_pk_destroy (hld_pkptr);
			FOUT;	
			}
		}

	/* Stamp the packet with the current time. This information will remain		*/
	/* unchanged even if the packet is copied for retransmissions, and			*/
	/* eventually it will be used by the destination MAC to compute the end-to-	*/
	/* end delay.																*/
	op_pk_stamp (hld_pkptr);
	
	/* Insert the arrived packet in higher layer queue.	*/	
	wlan_hlpk_enqueue (hld_pkptr, dest_addr);
	FOUT;
	}

static void
wlan_hlpk_enqueue (Packet* hld_pkptr, int dest_addr)
	{
	int						list_index;
	char					msg_string [120];
	char					msg_string1 [120];
	WlanT_Hld_List_Elem*	hld_ptr;
	double					data_size;
	
	/* Enqueuing data packet for transmission.	*/
	FIN (wlan_hlpk_enqueue (Packet* hld_pkptr, int dest_addr));

	/* Allocating pool memory to the higher layer data structure type. */	
	hld_ptr = (WlanT_Hld_List_Elem *) op_prg_pmo_alloc (hld_pmh);

	/* Generate error message and abort simulation if no memory left for data received from higher layer.	*/
	if (hld_ptr == OPC_NIL)
		{
		wlan_mac_error ("No more memory left to assign for data received from higher layer", OPC_NIL, OPC_NIL);
		}

	/* Updating higher layer data structure fields.	*/
	hld_ptr->time_rcvd           = current_time;
	hld_ptr->destination_address = dest_addr;
	hld_ptr->pkptr               = hld_pkptr;

	/* Insert a packet to the list.*/
	op_prg_list_insert (hld_list_ptr, hld_ptr, OPC_LISTPOS_TAIL);	
	
	/* Enable the flag indicating that there is a data frame to transmit.	*/
	wlan_flags->data_frame_to_send = OPC_BOOLINT_ENABLED;

	/* Printing out information to ODB.	*/
	if (wlan_trace_active == OPC_TRUE)
		{
		sprintf (msg_string, "Just arrived outbound Data packet id %d ", op_pk_id (hld_ptr->pkptr));
		sprintf	(msg_string1, "The outbound Data queue size is %d", 	op_prg_list_size (hld_list_ptr));	
		op_prg_odb_print_major (msg_string, msg_string1, OPC_NIL);
		}

	/* Report stat when outbound data packet is received.	*/
	op_stat_write (packet_load_handle, 1.0);
    op_stat_write (packet_load_handle, 0.0);

	/* Report stat in bits when outbound data packet is received.	*/
	data_size = (double) op_pk_total_size_get (hld_pkptr);
	op_stat_write (bits_load_handle, data_size);
    op_stat_write (bits_load_handle, 0.0);
	
	/* Update the global statistics as well.						*/
	op_stat_write (global_load_handle, data_size);
    op_stat_write (global_load_handle, 0.0);
	
	/* Report outbound data packets queue size at the arrival of every packet.	*/
	op_stat_write (hl_packets_rcvd, (double) (op_prg_list_size (hld_list_ptr)));

	FOUT;
	}

static void 
wlan_frame_transmit ()
	{
	char						msg_string  [120];
	char						msg_string1 [120];
	WlanT_Hld_List_Elem*		hld_ptr;
	int 						frag_list_size;
	int							type;
	double						pkt_tx_time;

	/** Main procedure to call functions for preparing frames.   **/
	/** The procedure to prepare frame is called in this routine **/
	FIN (wlan_frame_transmit());

	/* If Ack and Cts needs to be sent then prepare the appropriate	*/
	/* frame type for transmission									*/
	if ((fresp_to_send == WlanC_Cts) || (fresp_to_send == WlanC_Ack))
		{
		wlan_prepare_frame_to_send (fresp_to_send);

		/* Break the routine if Cts or Ack is already prepared to tranmsit */
		FOUT;
		}
	
	/* If it is a retransmission then check which type of frame needs to be	*/
	/* retransmitted and then prepare and transmit that frame				*/
	else if (retry_count != 0)
		{
		/* If the last frame unsuccessfully transmitted was Rts then transmit it again.	*/
		if ((last_frametx_type == WlanC_Rts) && (wlan_flags->rts_sent == OPC_BOOLINT_DISABLED))
			{
			/* Retransmit the Rts frame.	*/
			wlan_prepare_frame_to_send (WlanC_Rts);
			}

		/* For the retransmission of data frame first check whether Rts needs to be sent or not.	*/
		/* If it Rts needs to be sent and it is not already sent then first transmit Rts and then	*/
		/* transmit data frame.																		*/
		else if (last_frametx_type == WlanC_Data)
			{
			if ((op_pk_total_size_get (wlan_transmit_frame_copy_ptr) > (8 * rts_threshold + WLANC_MSDU_HEADER_SIZE)) && 
				(rts_threshold != -1) && (wlan_flags->rts_sent == OPC_BOOLINT_DISABLED))
				{
				/* Retransmit the Rts frame to again contend for the data .	*/
				wlan_prepare_frame_to_send (WlanC_Rts);				 
				}
			else
				{
				wlan_prepare_frame_to_send (WlanC_Data);
				}
			}
		else
			{ 
			/* We continue with the retransmission process. We	*/
			/* received the expected Cts for our last Rts.		*/
			/* Hence, now we can retransmit our data frame.		*/
			wlan_prepare_frame_to_send (WlanC_Data);
			}
			
		FOUT;
		}

	/* If higher layer queue is not empty then dequeue a packet	*/
	/* from the higher layer and insert it into fragmentation 	*/
	/* buffer check whether fragmentation and Rts-Cts exchange 	*/
	/* is needed  based on thresholds							*/
	/* Check if fragmenetation buffer is empty. If it is empty  */
   	/* then dequeue a packet from the higher layer queue.		*/ 
	else if ((op_prg_list_size (hld_list_ptr) != 0) && (op_sar_buf_size (fragmentation_buffer_ptr) == 0))
		{
		/* If rts is already sent then transmit data otherwise	*/
		/* check if rts needs to be sent or not.				*/
		if (wlan_flags->rts_sent == OPC_BOOLINT_DISABLED)
			{
			/* Remove packet from higher layer queue. */
			hld_ptr = (WlanT_Hld_List_Elem*) op_prg_list_remove (hld_list_ptr, 0);
			
			//$$$$$$$$$$$$$$$$$$ DSR $$$$$$$$$$$$$$$$$$$$$$$$
			op_pk_nfd_get((Packet*)(hld_ptr->pkptr),"Type",&data_packet_type);
			if (data_packet_type==DATA_PACKET_TYPE)
				 {
				 op_pk_nfd_get((Packet*)(hld_ptr->pkptr),"RELAY",&data_packet_dest);
			  	 op_pk_nfd_get((Packet*)(hld_ptr->pkptr),"DEST",&data_packet_final_dest);
				 }
			else
				 {
				 data_packet_dest=-1;
				 data_packet_final_dest=-1;
				 }
			//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
			
			/* Update the higher layer queue size statistic.				*/
			op_stat_write (hl_packets_rcvd, (double) (op_prg_list_size (hld_list_ptr)));
			
			/* Determine packet size to determine later wether fragmentation	*/
			/* and/or rts-cts exchange is needed.								*/
			packet_size = op_pk_total_size_get (hld_ptr->pkptr);

			/* Updating the total packet size of the higher layer buffer.	*/
			total_hlpk_size = total_hlpk_size - packet_size;

			/* Setting destination address state variable	*/				
			destination_addr = hld_ptr->destination_address;
						
			/* Packet seq number modulo 4096 counter.	*/
			packet_seq_number = (packet_seq_number + 1) % 4096;  

			/* Packet fragment number is initialized.	*/
			packet_frag_number = 0;				
						
			/* Packet needs to be fragmented if it is more 	*/
			/* than fragmentation threshold, provided		*/
			/* fragmentation is enabled.					*/
			if ((packet_size > (frag_threshold * 8)) && (frag_threshold != -1))
				{
				/* Determine number of fragments for the packet	*/
				/* and the size of the last fragment			*/							
				num_fragments =  (int) (packet_size / (frag_threshold * 8));
				remainder_size = packet_size - (num_fragments * frag_threshold * 8);

				/* If the remainder size is non zero it means that the	*/
				/* last fragment is fractional but since the number 	*/
				/* of fragments is a whole number we need to transmit	*/
				/* one additional fragment to ensure that all of the	*/
				/* data bits will be transmitted						*/
				if (remainder_size != 0)
					{
					num_fragments = num_fragments + 1;									 
					}
				}
			else
				{			
				/* If no fragments needed then number of	*/
				/* packets to be transmitted is set to 1	*/								
				num_fragments = 1;
				remainder_size = packet_size;
				}

			/* Storing Data packet id for debugging purposes.	*/			
			pkt_in_service = op_pk_id (hld_ptr->pkptr);		

			/* Insert packet to fragmentation buffer	*/					
			op_sar_segbuf_pk_insert (fragmentation_buffer_ptr, hld_ptr->pkptr, 0);

			/* Computing packet duration in the queue in seconds	*/
			/* and reporting it to the statistics					*/
			pkt_tx_time = (current_time - hld_ptr->time_rcvd);

			/* Printing out information to ODB.	*/
			if (wlan_trace_active == OPC_TRUE)
				{
				sprintf (msg_string, "Data packet %d is removed from higher layer buffer", pkt_in_service);
				sprintf	(msg_string1, "The queueing delay for data packet %d is %fs", 	
				pkt_in_service, pkt_tx_time);	
				op_prg_odb_print_major (msg_string, msg_string1, OPC_NIL);
				}

			/* Store the arrival time of the packet.	*/
			receive_time = hld_ptr->time_rcvd;
			
			/* Freeing up allocated memory for the data packet removed from the higher layer queue.	*/
			op_prg_mem_free (hld_ptr);

			/* Send rts if rts is enabled and packet size is more than rts threshold	*/
			if ((packet_size > (rts_threshold * 8)) && (rts_threshold != -1))
				{						 		
				retry_limit = long_retry_limit;
				/* Prepare Rts frame for transmission	*/
				wlan_prepare_frame_to_send (WlanC_Rts);
					
              	/* Break the routine as Rts is already prepared	*/
				FOUT;
				}
			else
				{
				retry_limit = short_retry_limit;
				}
			}
		}
		
	/* Prepare data frame to transmit	*/
	wlan_prepare_frame_to_send (WlanC_Data);

	FOUT;
	}
		
static void 
wlan_prepare_frame_to_send (int frame_type)
	{
	char							msg_string [120];
	Packet*							hld_pkptr;
	Packet*							seg_pkptr;
	int								dest_addr, src_addr;
	int								protocol_type = -1;	
	int								tx_datapacket_size;
	int								type;
	char							error_string [512];
	int								outstrm_to_phy;
	double							duration, mac_delay;
	WlanT_Data_Header_Fields*		pk_dhstruct_ptr;
	WlanT_Control_Header_Fields*	pk_chstruct_ptr;
	Packet*							wlan_transmit_frame_ptr;

	/** Prepare frames to transmit by setting appropriate fields in the 	**/
    /** packet format for Data,Cts,Rts or Ack.  If data or Rts packet needs **/
    /** to be retransmitted then the older copy of the packet is resent.    **/
	FIN (wlan_prepare_frame_to_send (int frame_type));

	outstrm_to_phy = LOW_LAYER_OUT_STREAM_CH1;

	/* The code is divided as per the frame types */
	switch (frame_type)
		{	
		case WlanC_Data:
			{			
			if (operational_speed == 2000000)
				{
				outstrm_to_phy = LOW_LAYER_OUT_STREAM_CH2;
				}
			else if (operational_speed == 5500000)
				{
				outstrm_to_phy = LOW_LAYER_OUT_STREAM_CH3;
				}
			else if (operational_speed == 11000000)
				{
				outstrm_to_phy = LOW_LAYER_OUT_STREAM_CH4;
				}

			/* If it is a retransmission of a packet then no need 	*/
            /* of preparing data frame.								*/
			if ((retry_count > 0) && (wlan_transmit_frame_copy_ptr != OPC_NIL))
				{
				/* If it is a retransmission then just transmit the previous frame	*/			
				wlan_transmit_frame_ptr = op_pk_copy (wlan_transmit_frame_copy_ptr);

				/* If retry count is non-zero means that the frame is a */
				/* retransmission of the last transmitted frame			*/
				op_pk_nfd_access (wlan_transmit_frame_ptr, "Wlan Header", &pk_dhstruct_ptr);
				pk_dhstruct_ptr->retry = 1;

				/* Printing out information to ODB.	*/
				if (wlan_trace_active == OPC_TRUE)
					{
					sprintf (msg_string, "Data fragment %d for packet %d is retransmitted", pk_dhstruct_ptr->fragment_number, pkt_in_service);							
					op_prg_odb_print_major (msg_string, OPC_NIL);
					}					

				/* Calculate nav duration till the channel will be occupied by 	*/
				/* station. The duration is SIFS time plus the ack frame time  	*/
				/* which the station needs in response to the data frame.		*/		
				duration = WLAN_ACK_DURATION + sifs_time + 5;//WLAN_AIR_PROPAGATION_TIME;				

				/* Since the number of fragments for the last transmitted frame is	*/
				/* already decremented, there will be more fragments to transmit  	*/
				/* if number of fragments is more than zero.					  	*/
				if (num_fragments != 1)	
					{
					/* If more fragments need to be transmitted then the station 	*/
					/* need to broadcast the time until the receipt of the       	*/
					/* the acknowledgement for the next fragment. 224 bits (header	*/
					/* size) is the length of the control fields in the data  		*/
					/* frame and needs to be accounted in the duration calculation	*/
					duration = 2 * duration + ((frag_threshold * 8) + WLANC_MSDU_HEADER_SIZE) / operational_speed + 
							   sifs_time + WLAN_AIR_PROPAGATION_TIME;
					}
			
				/* Station update of it's own nav_duration.	To keep track of the next	*/
				/* available contention window.											*/
				nav_duration = current_time + duration + (double) (op_pk_total_size_get (wlan_transmit_frame_ptr)) / operational_speed;
				}

			else
				{
				/* Creating transmit data packet type	*/
				wlan_transmit_frame_ptr = op_pk_create_fmt ("wlan_mac");
				
				/* Prepare data frame fields for transmission.	*/		
				pk_dhstruct_ptr = wlan_mac_pk_dhstruct_create ();
				type = WlanC_Data;   		

				pk_dhstruct_ptr->retry = 0;				
				pk_dhstruct_ptr->order = 1;
				pk_dhstruct_ptr->sequence_number = packet_seq_number;

				/* Calculate nav duration till the channel will be occupied by  */
				/* station. The duration is SIFS time plus the ack frame time   */
				/* which the station needs in response to the data frame.		*/		
				duration = WLAN_ACK_DURATION + sifs_time + WLAN_AIR_PROPAGATION_TIME;

				/* If there is more than one fragment to transmit and there are  	*/
				/* equal sized fragments then remove fragmentation threshold size	*/
				/* length of data from the buffer for transmission.					*/
				if  ((num_fragments > 1) || (remainder_size == 0))
					{
					/* Remove next fragment from the fragmentation buffer for 	*/
					/* transmission and set the appropriate fragment number.  	*/
					seg_pkptr = op_sar_srcbuf_seg_remove (fragmentation_buffer_ptr, frag_threshold * 8);
			
					/* Indicate in transmission frame that more fragments need to be sent	*/
					/* if more than one fragments are left								 	*/
					if (num_fragments != 1)	
						{
						pk_dhstruct_ptr->more_frag = 1;

						/* If more fragments need to be transmitted then the station	*/
						/* need to broadcast the time until the receipt of the       	*/
						/* the acknowledgement for the next fragment. 224 bits (header	*/
						/* size) is the length of control fields in the data frame  	*/
						/* and need to be accounted for in the duration calculation		*/
						duration = 2 * duration + ((frag_threshold * 8) + WLANC_MSDU_HEADER_SIZE) / operational_speed + 
						           sifs_time + WLAN_AIR_PROPAGATION_TIME;
						}
					else
						{
						/* If no more fragments to transmit then set more fragment field to be 0 */
						pk_dhstruct_ptr->more_frag = 0;
						}
						
					/* Set fragment number in packet field	 */
					pk_dhstruct_ptr->fragment_number = packet_frag_number ;

					/* Printing out information to ODB.	*/
					if (wlan_trace_active == OPC_TRUE)
						{
						sprintf (msg_string, "Data fragment %d for packet %d is transmitted",packet_frag_number, pkt_in_service);							
						op_prg_odb_print_major (msg_string, OPC_NIL);
						}

					/* Setting packet fragment number for next fragment to be transmitted */
					packet_frag_number = packet_frag_number + 1;    	
					}
				else
					{
					/* Remove last fragments (if any left) from the fragmentation buffer for */
					/* transmission and disable more fragmentation bit.				         */
					seg_pkptr = op_sar_srcbuf_seg_remove (fragmentation_buffer_ptr, remainder_size);					

					pk_dhstruct_ptr->more_frag = 0;

					/* Printing out information to ODB.	*/
					if (wlan_trace_active == OPC_TRUE)
						{
						sprintf (msg_string, "Data fragment %d for packet %d is transmitted",packet_frag_number, pkt_in_service);								
						op_prg_odb_print_major (msg_string, OPC_NIL);
						}

					pk_dhstruct_ptr->fragment_number = packet_frag_number;
					}	

				/* Setting the Header field structure.	*/
				pk_dhstruct_ptr->duration  = duration;
				pk_dhstruct_ptr->address1  = destination_addr;
				pk_dhstruct_ptr->address2  = my_address;

				/* In the BSS network the Data frame is going from AP to sta then fromds bit is set.	*/
    			if (ap_flag == OPC_BOOLINT_ENABLED)
					{
					pk_dhstruct_ptr->fromds	 = 1;
					}
				else
					{
					pk_dhstruct_ptr->fromds	 = 0;
					}

				/* if in the BSS network the Data frame is going from sta to AP then tods bit is set.	*/					
    			if ((bss_flag == OPC_BOOLINT_ENABLED) && (ap_flag == OPC_BOOLINT_DISABLED))
					{
					pk_dhstruct_ptr->tods = 1;

					/* If Infrastructure BSS then the immediate destination will be Access point, which 	*/
					/* then forward the frame to the appropriate destination.								*/
					pk_dhstruct_ptr->address1 = bss_id;
					pk_dhstruct_ptr->address3 = destination_addr;
					}
				else
					{
					pk_dhstruct_ptr->tods = 0;
					}
	
				/* If we are sending the first fragment of the data fragment for the first	*/
				/* time, then this is the end of media access duration, hence we must		*/
				/* update the media access delay statistics.								*/
				if (packet_size == op_pk_total_size_get (seg_pkptr) + op_sar_buf_size (fragmentation_buffer_ptr))
					{
					mac_delay = current_time - receive_time;
					op_stat_write (media_access_delay, mac_delay);
					op_stat_write (media_access_delay, 0.0);
					op_stat_write (global_mac_delay_handle, mac_delay);
					op_stat_write (global_mac_delay_handle, 0.0);
					}
				
				op_pk_nfd_set (wlan_transmit_frame_ptr, "Type", type);

				/* Setting the variable which keeps track of the last transmitted frame.	*/
                last_frametx_type = type;

				op_pk_nfd_set (wlan_transmit_frame_ptr, "Accept", OPC_TRUE);
				op_pk_nfd_set (wlan_transmit_frame_ptr, "Data Packet ID", pkt_in_service);
				
				/* Set the frame control field and nav duration.				*/
				op_pk_nfd_set (wlan_transmit_frame_ptr, "Wlan Header", pk_dhstruct_ptr,	
				wlan_mac_pk_dhstruct_copy, wlan_mac_pk_dhstruct_destroy, sizeof (WlanT_Data_Header_Fields));

				/* The actual data is placed in the Frame Body field	*/
				op_pk_nfd_set (wlan_transmit_frame_ptr, "Frame Body", seg_pkptr);

				/* Make copy of the frame before transmission	*/
				wlan_transmit_frame_copy_ptr = op_pk_copy (wlan_transmit_frame_ptr);

			    /* Station update of its own nav_duration	*/
				nav_duration = current_time + duration + (double) (op_pk_total_size_get (wlan_transmit_frame_ptr)) / operational_speed ;
   				}
			
			/* Reporting total number of bits in a data frame.   */
			op_stat_write (data_traffic_sent_handle_inbits, (double) op_pk_total_size_get (wlan_transmit_frame_ptr));
			op_stat_write (data_traffic_sent_handle_inbits, 0.0);

			/* If there is nothing in the higher layer data queue and fragmentation buffer	*/
			/* then disable the data frame flag which will indicate to the station to wait	*/
			/* for the higher layer packet.													*/
			if (op_prg_list_size (hld_list_ptr) == 0 && op_sar_buf_size (fragmentation_buffer_ptr) == 0)
				{
				wlan_flags->data_frame_to_send = OPC_BOOLINT_DISABLED;
				}
				
			/* Only expect Acknowledgement for directed frames.	*/
			if (destination_addr < 0)
				{
				expected_frame_type = WlanC_None;
				}
			else
				{
				/* Ack frame is expected in response to data frame	*/
				expected_frame_type = WlanC_Ack;
				}

			/* Update data traffic sent stat when the transmission is complete	*/
			op_stat_write (data_traffic_sent_handle, 1.0);
			op_stat_write (data_traffic_sent_handle, 0.0);
			break;
			}

		case WlanC_Rts:
			{
			/* Creating Rts packet format type	*/
			wlan_transmit_frame_ptr = op_pk_create_fmt ("wlan_control");

			/* Initializing Rts frame fields	*/
			pk_chstruct_ptr = wlan_mac_pk_chstruct_create ();
		
			/* Type of frame */
	   		type = WlanC_Rts;   						

			/* if in the infrastructure BSS network then the immediate receipient for the transmitting	*/
			/* station will always be an Access point. Otherwise the frame is directly sent to the 		*/
			/* final destination.																		*/
    		if ((bss_flag == OPC_BOOLINT_ENABLED) && (ap_flag == OPC_BOOLINT_DISABLED))
				{
				/* If Infrastructure BSS then the immediate destination will be Access point, which 	*/
				/* then forward the frame to the appropriate destination.								*/
				pk_chstruct_ptr->rx_addr = bss_id;
				}
			else
				{
				/* Otherwise set the final destination address.	*/				   
				pk_chstruct_ptr->rx_addr = destination_addr;
				}

			/* Source station address.	*/
			pk_chstruct_ptr->tx_addr = my_address;

			/* Setting the Rts frame type.	*/
			op_pk_nfd_set (wlan_transmit_frame_ptr, "Type", type);

			/* Setting the accept field to true, meaning the frame is a good frame.	*/
			op_pk_nfd_set (wlan_transmit_frame_ptr, "Accept", OPC_TRUE);
				
			/* Setting the variable which keeps track of the last transmitted frame that needs response.	*/
            last_frametx_type = type;
				
			/* Determining the size of the first data fragment or frame that need */
			/* to be transmitted following the Rts transmission.				  */				
			if (num_fragments > 1)
				{
				/* If there are more than one fragment to transmit then the */
				/* data segment of the first data frame will be the size of */
				/* fragmentation threshold. The total packet size will be   */
				/* data plus the overhead (which is 224 bits).				*/
				tx_datapacket_size = frag_threshold * 8 + WLANC_MSDU_HEADER_SIZE;
				}
			else
				/* If there is one data frame to transmit then the          */
				/* data segment of the first data frame will be the size of */
				/* the remainder computed earlier. The total packet size    */
				/* will be data plus the overhead (which is 224 bits).		*/
				{
				tx_datapacket_size = remainder_size + WLANC_MSDU_HEADER_SIZE;
				}

			/* Station is reserving channel bandwidth by using Rts frame, so    */
			/* in Rts the station will broadcast the duration it needs to send  */ 		 		
			/* one data frame and receive ack for it. The total duration is the */
			/* the time required to transmit one data frame, plus one Cts frame */
			/* plus one ack frame, plus three sifs interval, and plus           */
			/* air propagation time for three frames							*/
			duration = (WLAN_CTS_DURATION + WLAN_ACK_DURATION) + ((double) (tx_datapacket_size) / operational_speed) + 3 * (sifs_time + WLAN_AIR_PROPAGATION_TIME);            
			pk_chstruct_ptr->duration = duration;
				
			/* Setting Rts frame fields.	*/
			op_pk_nfd_set (wlan_transmit_frame_ptr, "Wlan Header", pk_chstruct_ptr, wlan_mac_pk_chstruct_copy, wlan_mac_pk_chstruct_destroy, sizeof (WlanT_Control_Header_Fields));				
			op_pk_total_size_set (wlan_transmit_frame_ptr, WLAN_RTS_LENGTH);
				
			/* Station update of its own nav_duration	*/
			nav_duration = current_time + duration + (double) (op_pk_total_size_get (wlan_transmit_frame_ptr)) / WLAN_MAN_DATA_RATE ; 								 						
			
			/* Cts is expected in response to Rts.	*/						
			expected_frame_type = WlanC_Cts;

			/* Printing out information to ODB.	*/
			if (wlan_trace_active == OPC_TRUE)
				{
				sprintf (msg_string, "Rts is being transmitted for data packet %d", pkt_in_service);
				op_prg_odb_print_major (msg_string, OPC_NIL);
				}

			/* Reporting total number of bits in a control frame.   */
			op_stat_write (ctrl_traffic_sent_handle_inbits, (double) op_pk_total_size_get (wlan_transmit_frame_ptr));
			op_stat_write (ctrl_traffic_sent_handle_inbits, 0.0);

			/* Update control traffic sent stat when the transmission is complete	*/
			op_stat_write (ctrl_traffic_sent_handle, 1.0);
			op_stat_write (ctrl_traffic_sent_handle, 0.0);
			break;
			}

		case WlanC_Cts:
			{
			/** Preparing Cts frame in response to the received Rts frame 	**/
			/** from the remote station. No response needed for Cts frame.	**/
			
			/* Creating Cts packet format type */
			wlan_transmit_frame_ptr = op_pk_create_fmt ("wlan_control");

			/* Initializing Rts frame fields */
			pk_chstruct_ptr = wlan_mac_pk_chstruct_create ();
		
			/* Type of frame */
   			type = WlanC_Cts;

			/* Destination station address.	*/
			pk_chstruct_ptr->rx_addr = remote_sta_addr;
			
			/* Station is reserving channel bandwidth by using Rts frame, so    */
			/* in Rts the station will broadcast the duration it needs to send  */ 		 		
			/* one data frame and receive ack for it. The total duration is the */
			/* the time required to transmit one Cts frame, plus one data		*/
			/* frame, plus one Ack frame, plus three sifs interval, and plus	*/
			/* three air propagation time for three frames.						*/
			/* In Cts frame the station will transmit the remaining time needed	*/
			/* by the station after the exchange of Rts-Cts 					*/
			duration = nav_duration - (sifs_time + WLAN_CTS_DURATION + WLAN_AIR_PROPAGATION_TIME + current_time);
			pk_chstruct_ptr->duration = duration;

			/* Setting Cts frame type.	*/
			op_pk_nfd_set (wlan_transmit_frame_ptr, "Type", type);

			/* Setting the accept field to true, meaning the frame is a good frame.	*/
			op_pk_nfd_set (wlan_transmit_frame_ptr, "Accept", OPC_TRUE);
				
			/* Setting Cts frame fields.	*/
			op_pk_nfd_set (wlan_transmit_frame_ptr, "Wlan Header", pk_chstruct_ptr, wlan_mac_pk_chstruct_copy, 
			wlan_mac_pk_chstruct_destroy, sizeof (WlanT_Control_Header_Fields));

			/* Setting the total frame size to Cts length.	*/
			op_pk_total_size_set (wlan_transmit_frame_ptr, WLAN_CTS_LENGTH);			
			
			/* Once Cts is transmitted in response to Rts then set the frame    		*/
			/* response indicator to none frame as the response is already generated	*/
			fresp_to_send = WlanC_None;										
			
			/* No frame is expected once Cts is transmitted	*/
			expected_frame_type = WlanC_None;	

			/* Printing out information to ODB.	*/
			if (wlan_trace_active == OPC_TRUE)
				{
				sprintf (msg_string, "Cts is being transmitted in response to Rts");
				op_prg_odb_print_major (msg_string, OPC_NIL);
				}

			/* Reporting total number of bits in a control frame	*/
			op_stat_write (ctrl_traffic_sent_handle_inbits, (double) op_pk_total_size_get (wlan_transmit_frame_ptr));
			op_stat_write (ctrl_traffic_sent_handle_inbits, 0.0);

			/* Update control traffic sent stat when the transmission is complete	*/
			op_stat_write (ctrl_traffic_sent_handle, 1.0);
			op_stat_write (ctrl_traffic_sent_handle, 0.0);
			break;
    		}

		case WlanC_Ack:
			{	
			/** Preparing acknowledgement frame in response to the data **/
			/** frame received from the remote stations. Note that no	**/
			/** response is needed for the ack frame.					**/
			
			/* Creating ack packet format type 	 */
			wlan_transmit_frame_ptr = op_pk_create_fmt ("wlan_control");

			/* Setting ack frame fields	 */
			pk_chstruct_ptr = wlan_mac_pk_chstruct_create ();
   			type = WlanC_Ack;   		
			pk_chstruct_ptr->retry = duplicate_entry;

			/* If there are more fragments to transmit then broadcast the remaining duration for which	*/
			/* the station will be using the channel.													*/
			duration = nav_duration - (WLAN_ACK_DURATION + WLAN_AIR_PROPAGATION_TIME + current_time);
			pk_chstruct_ptr->duration = duration;

			/* Destination station address.	*/
			pk_chstruct_ptr->rx_addr = remote_sta_addr;

			/* Setting Ack type.	*/
			op_pk_nfd_set (wlan_transmit_frame_ptr, "Type", type);
			
			/* Setting the accept field to true, meaning the frame is a good frame.	*/
			op_pk_nfd_set (wlan_transmit_frame_ptr, "Accept", OPC_TRUE);

			op_pk_nfd_set (wlan_transmit_frame_ptr, "Wlan Header", pk_chstruct_ptr, wlan_mac_pk_chstruct_copy, 
			wlan_mac_pk_chstruct_destroy, sizeof (WlanT_Control_Header_Fields));

			/* Setting the total frame size to Ack length.	*/
			op_pk_total_size_set (wlan_transmit_frame_ptr, WLAN_ACK_LENGTH);            

			/* since no frame is expected,the expected frame type field */
			/* to nil.                                                  */
			expected_frame_type = WlanC_None;	
	
			/* Once Ack is transmitted in response to Data frame then set the frame		*/
			/* response indicator to none frame as the response is already generated	*/
			fresp_to_send = WlanC_None;			

			/* Printing out information to ODB.	*/
			if (wlan_trace_active == OPC_TRUE)
				{
				sprintf (msg_string, "Ack is being transmitted for data packet received");
				op_prg_odb_print_major (msg_string, OPC_NIL);
				}

			/* Reporting total number of bits in a control frame.   */
			op_stat_write (ctrl_traffic_sent_handle_inbits, (double) op_pk_total_size_get (wlan_transmit_frame_ptr));
			op_stat_write (ctrl_traffic_sent_handle_inbits, 0.0);

			/* Update control traffic sent stat when the transmission is complete*/
			op_stat_write (ctrl_traffic_sent_handle, 1.0);
			op_stat_write (ctrl_traffic_sent_handle, 0.0);
			break;
			}

		default:
			{
			wlan_mac_error ("Transmission request for unexpected frame type.", OPC_NIL, OPC_NIL);
			break;
			}
		}
	
	/* Sending packet to the transmitter */
	op_pk_send (wlan_transmit_frame_ptr, outstrm_to_phy);
	wlan_flags->transmitter_busy = OPC_BOOLINT_ENABLED;
	
	FOUT;
	}

void 
wlan_interrupts_process ()
	{
	/** This routine handles the appropriate processing need for each type 	**/
	/** of remote interrupt. The type of interrupts are: stream interrupts	**/
	/** (from lower and higher layers), stat interrupts (from receiver and 	**/
	/** transmitter).                                                      	**/
	FIN (wlan_interrupts_process ());

	/* Check if debugging is enabled.	*/
	wlan_trace_active = op_prg_odb_ltrace_active ("wlan");

	/* Determine the current simualtion time	*/	
	current_time = op_sim_time ();
	
	/* Determine interrupt type and code to divide treatment	*/
	/* along the lines of interrupt type  						*/
	intrpt_type = op_intrpt_type ();
	intrpt_code = op_intrpt_code ();

	/* Stream interrupts are either arrivals from the higher layer,	*/
	/* or from the physical layer									*/
	if (intrpt_type == OPC_INTRPT_STRM)
		{
		/* Determine the stream on which the arrival occurred	*/
		i_strm = op_intrpt_strm ();

		/* If the event arrived from higher layer then queue the packet	*/
		/* and the destination address									*/
		if (i_strm == instrm_from_mac_if)
			{
			/* Process stream interrupt received from higher layer	*/
			wlan_higher_layer_data_arrival ();
			}

		/* If the event was an arrival from the physical layer,	*/
		/* accept the packet and decapsulate it			 		*/
		else 
			{
			/* Process stream interrupt received from physical layer	*/			 
			wlan_physical_layer_data_arrival ();		
			}
	 	}	

	/* Handle stat interrupt received from the receiver	*/
	else if (intrpt_type == OPC_INTRPT_STAT)
		{
		/* Make sure it is not a stat interrupt from the transmitter.	*/
		if (intrpt_code == RECEIVER_BUSY_INSTAT)
			{
			/* Update the flag value based on the new status of the	*/
			/* receiver.											*/
			if (op_stat_local_read (RECEIVER_BUSY_INSTAT) == 1.0)
				{				
				wlan_flags->receiver_busy = OPC_BOOLINT_ENABLED;
				}
			else
				{
				wlan_flags->receiver_busy = OPC_BOOLINT_DISABLED;

				/* Reset the receiver idle timer to the current time since	*/
				/* it became available.										*/
				rcv_idle_time = current_time;
				}
			}
		}

	else if (intrpt_type == OPC_INTRPT_SELF)
		{
		if (intrpt_code == WlanC_CW_Elapsed)
			{
			/* Reset the CW timer, since the period is over, to		*/
			/* enable state transitions.							*/
			cw_end = 0.0;
			}
		}
	
	FOUT;
	}

static void 
wlan_physical_layer_data_arrival ()
	{
	char										msg_string [120];
	int											dest_addr, src_addr;
	int											accept;
	int											data_pkt_id;
	int											final_dest_addr;
	int											rcvd_sta_bssid;
	WlanT_Data_Header_Fields*					pk_dhstruct_ptr;
	WlanT_Control_Header_Fields*				pk_chstruct_ptr;
	WlanT_Mac_Frame_Type						rcvd_frame_type;	
	Packet*										wlan_rcvd_frame_ptr;
	Packet*										seg_pkptr;
	//$$$$$$$$$$$$$$$$$$ DSR $$$$$$$$$$$$$$$$$$$$$$$$
	int tmp_remote_sta_addr;
	Ici* iciptr;
	Objid process_id;
	//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
	
	/** Process the frame received from the lower layer.         **/
	/** This routine decapsulate the frame and set appropriate   **/
	/** flags if the station needs to generate a response to the **/
	/** received frame.											 **/
	FIN (wlan_physical_layer_data_arrival ());

	/*  Access received packet from the physical layer stream.	*/
	wlan_rcvd_frame_ptr = op_pk_get (i_strm);	

	op_pk_nfd_access (wlan_rcvd_frame_ptr, "Accept", &accept);
				
	/* If the packet is received while the station is in transmission	*/
	/* the packet will not be processed and if needed the station will	*/
	/* need to retransmit the packet.									*/
	if ((wlan_flags->rcvd_bad_packet == OPC_BOOLINT_ENABLED) || (accept == OPC_FALSE))
		{
		/* If the pipeline stage set the accept flag to be false then it means that	*/
		/* the packet is erroneous.  Enable the EIFS duration flag and set			*/
		/* nav duration to be EIFS duration.										*/ 	
		if (accept == OPC_FALSE)
			{
			wlan_flags->wait_eifs_dur = OPC_BOOLINT_ENABLED;
     
			/* Setting nav duration to EIFS.	*/
			nav_duration = current_time + eifs_time; 

			/* Reporting the amount of time the channel will be busy.	*/	   
			op_stat_write (channel_reserv_handle, (nav_duration - current_time));
			op_stat_write (channel_reserv_handle, 0.0);
			}

		/* We have experienced a collision during transmission. We		*/
		/* could be transmitting a packet which requires a response (an	*/
		/* Rts or a data frame requiring an Ack). Even, this is the		*/
		/* case, we do not take any action right now and wait for the	*/
		/* related timers to expire; then we will retransmit the frame.	*/
		/* This is the approach described in the standard, and it is	*/
		/* necessary because of the slight possibility that our peer	*/
		/* may receive the frame without collision and send us the		*/
		/* response back, which we should be still expecting.			*/

		/* Check whether the timer for the expected response has		*/
		/* already expired. If yes, we must initiate the retransmission.*/
		if ((expected_frame_type != WlanC_None) && (wlan_flags->transmitter_busy == OPC_BOOLINT_DISABLED) &&
			(op_ev_valid (frame_timeout_evh) == OPC_FALSE))
			{
			retry_count = retry_count + 1;

			/* If Rts sent flag was enable then disable it as the station will recontend for the channel.	*/
			if (wlan_flags->rts_sent == OPC_BOOLINT_ENABLED)
				{
				wlan_flags->rts_sent = OPC_BOOLINT_DISABLED;
				}

			/* Check whether further retries are possible or	*/
			/* the data frame needs to be discarded.			*/
			wlan_frame_discard ();

			/* Set expected frame type flag to none as the station needs to retransmit the frame.	*/
			expected_frame_type = WlanC_None;
			
			/* Reset the NAV duration so that the				*/
			/* retransmission is not unnecessarily delayed.		*/
			nav_duration = current_time;
			}

		//$$$$$$$$$$$$$$$$$$ DSR $$$$$$$$$$$$$$$$$$$$$$$$
		/* No frame response will be generated for bad frame.	*/
		//fresp_to_send = WlanC_None;
		//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

		/* Reset the bad packet receive flag for subsequent receptions.	*/
		wlan_flags->rcvd_bad_packet = OPC_BOOLINT_DISABLED;		

		/* Printing out information to ODB.	*/
		if (wlan_trace_active == OPC_TRUE)
			{
			sprintf (msg_string, "Received bad packet. Discarding received packet");
			op_prg_odb_print_major (msg_string, OPC_NIL);
			}
		
		/* Destroy the bad packet.									*/
		op_pk_destroy (wlan_rcvd_frame_ptr);

		/* Break the routine as no further processing is needed.	*/
		FOUT;
		}

	/* If waiting for EIFS duration then set the nav duration such that	*/
	/* the normal operation is resumed.									*/
	if (wlan_flags->wait_eifs_dur == OPC_BOOLINT_ENABLED)
		{
		nav_duration = current_time;
		wlan_flags->wait_eifs_dur = OPC_BOOLINT_DISABLED;	
		}	

	/* Getting frame control field and duration information from	*/
    /* the received packet.											*/		
	op_pk_nfd_access (wlan_rcvd_frame_ptr, "Type", &rcvd_frame_type) ;

	/* Divide processing based on frame type	*/		
	switch (rcvd_frame_type)
		{		
		case WlanC_Data:
			{
			/** First check that wether the station is expecting	**/
			/** any frame or not. If not then decapsulate relevant	**/
			/** information from the packet fields and set the		**/
			/** frame response variable with appropriate			**/
			/** frame type.											**/

			/*  Data traffic received report in terms of number of bits.	*/
			op_stat_write (data_traffic_rcvd_handle_inbits, (double) op_pk_total_size_get (wlan_rcvd_frame_ptr));
			op_stat_write (data_traffic_rcvd_handle_inbits, 0.0);

			/*  Data traffic received report in terms of number of packets.	*/
			op_stat_write (data_traffic_rcvd_handle, 1.0);
			op_stat_write (data_traffic_rcvd_handle, 0.0);

			/* Address information, sequence control fields,	*/
			/* and the data is extracted from the rcvd packet.	*/
			op_pk_nfd_access (wlan_rcvd_frame_ptr, "Wlan Header",	&pk_dhstruct_ptr);

			/* Data packet id of the received data frame is extracted.	*/
			op_pk_nfd_access (wlan_rcvd_frame_ptr, "Data Packet ID", &data_pkt_id);

			dest_addr = pk_dhstruct_ptr->address1;	
			//$$$$$$$$$$$$$$$$$$ DSR $$$$$$$$$$$$$$$$$$$$$$$$
			tmp_remote_sta_addr = pk_dhstruct_ptr->address2;
			//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
			
			remote_sta_addr = pk_dhstruct_ptr->address2;

			/* If the station is an AP then it will need to forward the receiving data to this address.	*/ 
			/* Otherwise this field will be zero and will be ignored.									*/
			final_dest_addr = pk_dhstruct_ptr->address3;

			//$$$$$$$$$$$$$$$$$$ DSR $$$$$$$$$$$$$$$$$$$$$$$$
			//fresp_to_send = WlanC_None;
			//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

			//$$$$$$$$$$$$$$$$$$ DSR $$$$$$$$$$$$$$$$$$$$$$$$
			///* Process frame only if it is destined for this station.	*/
			///* Or it is a broadcast frame.								*/
			//if ((dest_addr == my_address) || (dest_addr < 0))
			if (1) // DRS: promiscuous mode, thus every frame need to be tranmistted to the upper layer
				{	
				/* Extracting the MSDU from the packet only if the packet	*/
				/* is destined for this station.							*/		
				op_pk_nfd_get (wlan_rcvd_frame_ptr, "Frame Body", &seg_pkptr);

				/* Only send acknowledgement if the data frame is destined for this station.	*/
				/* No Acks for broadcast frame.													*/
				if (dest_addr == my_address)
					{
					// remote address saved in the state variable only if an ack is expected
					remote_sta_addr = tmp_remote_sta_addr;
					/* Send the acknowledgement to any received data frame.	*/
					fresp_to_send = WlanC_Ack;
					}
             	
				/* If its a duplicate packet then destroy it and do nothing otherwise, 	*/
				/* insert it in the defragmentation list.								*/
				if (wlan_tuple_find (tmp_remote_sta_addr, pk_dhstruct_ptr->sequence_number, pk_dhstruct_ptr->fragment_number) == OPC_FALSE)
					{
					wlan_data_process (seg_pkptr, tmp_remote_sta_addr, final_dest_addr, pk_dhstruct_ptr->fragment_number, 
										pk_dhstruct_ptr->more_frag, data_pkt_id, rcvd_sta_bssid);
					}
				//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
			    }
			else
				{
				/* Printing out information to ODB.	*/
				if (wlan_trace_active == OPC_TRUE)
					{
					sprintf (msg_string, "Data packet %d is received and discarded", data_pkt_id);
					op_prg_odb_print_major (msg_string, OPC_NIL);
					}
				
				//$$$$$$$$$$$$$$$$$$ DSR $$$$$$$$$$$$$$$$$$$$$$$$
				/* If the frame is not destined for this station	*/
				/* then do not respond with any frame.				*/
				//fresp_to_send = WlanC_None;
				//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
				}						
			
			if (expected_frame_type != WlanC_None) 
				{
				/* Since the station did not receive the expected frame	*/
				/* it has to retransmit the packet.						*/
				retry_count = retry_count + 1;

				/* If Rts sent flag was enable then disable it as the station will recontend for the channel.	*/
				if (wlan_flags->rts_sent == OPC_BOOLINT_ENABLED)
					{
					wlan_flags->rts_sent = OPC_BOOLINT_DISABLED;
					}

				/* Reset the NAV duration so that the				*/
				/* retransmission is not unnecessarily delayed.		*/
				nav_duration = current_time;
				}
			
			/* Update nav duration if the received nav duration is greater	*/
			/* than the current nav duration.							  	*/    	
			if (nav_duration < (pk_dhstruct_ptr->duration + current_time))
				{
				nav_duration = pk_dhstruct_ptr->duration + current_time;
				
				/* Set the flag that indicates updated NAV value.			*/
				wlan_flags->nav_updated = OPC_BOOLINT_ENABLED;
				}
			break;
			}

		case WlanC_Rts:
			{
			/** First check that wether the station is expecting any frame or not	**/
			/** If not then decapsulate the Rts frame and set a Cts frame response	**/
			/** if frame is destined for this station. Otherwise, just update the	**/
			/** network allocation vector for this station.							**/

			/*  Control Traffic received report in terms of number of bits.		*/
			op_stat_write (ctrl_traffic_rcvd_handle_inbits, (double) op_pk_total_size_get (wlan_rcvd_frame_ptr));
			op_stat_write (ctrl_traffic_rcvd_handle_inbits, 0.0);

			/*  Control Traffic received report in terms of number of packets.	*/
			op_stat_write (ctrl_traffic_rcvd_handle, 1.0);
			op_stat_write (ctrl_traffic_rcvd_handle, 0.0);

			op_pk_nfd_access (wlan_rcvd_frame_ptr, "Wlan Header",	&pk_chstruct_ptr);
			
			dest_addr = pk_chstruct_ptr->rx_addr;
			//$$$$$$$$$$$$$$$$$$ DSR $$$$$$$$$$$$$$$$$$$$$$$$
			tmp_remote_sta_addr = pk_chstruct_ptr->tx_addr;
			//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
			
			if (expected_frame_type == WlanC_None)
				{
				/* We will respond to the Rts with a Cts only if a) the	*/
				/* Rts is destined for us, and b) our NAV duration is	*/
				/* not larger than current simulation time.				*/
				if ((my_address == dest_addr) && (current_time >= nav_duration))
					{
					//$$$$$$$$$$$$$$$$$$ DSR $$$$$$$$$$$$$$$$$$$$$$$$
					remote_sta_addr = tmp_remote_sta_addr;
					//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
					
					/* Set the frame response field to Cts.				*/
					fresp_to_send = WlanC_Cts;

					/* Printing out information to ODB.	*/
					if (wlan_trace_active == OPC_TRUE)
						{
						sprintf (msg_string, "Rts is received and Cts will be transmitted");
						op_prg_odb_print_major (msg_string, OPC_NIL);
						}
					}			
				else
					{
					//$$$$$$$$$$$$$$$$$$ DSR $$$$$$$$$$$$$$$$$$$$$$$$
					/* If Rts is not destined for this station then set the	*/
					/* frame response field to None							*/
					//fresp_to_send = WlanC_None;
					//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

					/* Printing out information to ODB.	*/
					if (wlan_trace_active == OPC_TRUE)
						{
						sprintf (msg_string, "Rts is received and discarded");
						op_prg_odb_print_major (msg_string, OPC_NIL);
						}
					}				
				}
			else
				{				
				/* Since the station did not receive the expected frame it has to retransmit the packet	*/
				retry_count = retry_count + 1;				

				/* If Rts sent flag was enable then disable it as the station will recontend for the channel.	*/
				if (wlan_flags->rts_sent == OPC_BOOLINT_ENABLED)
					{
					wlan_flags->rts_sent = OPC_BOOLINT_DISABLED;
					}
				
				/* Reset the NAV duration so that the				*/
				/* retransmission is not unnecessarily delayed.		*/
				nav_duration = current_time;

				/* Reset the expected frame type variable since we	*/
				/* will retransmit.									*/
				fresp_to_send = WlanC_None;
				}

			/* Update nav duration if the received nav duration is greater	*/
			/* than the current nav duration.							  	*/    	
			if (nav_duration < (pk_chstruct_ptr->duration + current_time))
				{
				nav_duration = pk_chstruct_ptr->duration + current_time;

				/* Set the flag that indicates updated NAV value.			*/
				wlan_flags->nav_updated = OPC_BOOLINT_ENABLED;
				}			
			break;			
			}		
		
		case WlanC_Cts:
			{
			/** First check that whether the station is expecting any frame or not	**/
			/** If not then decapsulate the Rts frame and set a Cts frame response	**/
			/** if frame is destined for this station. Otherwise, just update the	**/
			/** network allocation vector for this station.							**/

			/*  Control Traffic received report in terms of number of bits.	*/
			op_stat_write (ctrl_traffic_rcvd_handle_inbits, (double) op_pk_total_size_get (wlan_rcvd_frame_ptr));
			op_stat_write (ctrl_traffic_rcvd_handle_inbits, 0.0);

			/*  Control Traffic received report in terms of number of packets.	*/
			op_stat_write (ctrl_traffic_rcvd_handle, 1.0);
			op_stat_write (ctrl_traffic_rcvd_handle, 0.0);	

			op_pk_nfd_access (wlan_rcvd_frame_ptr, "Wlan Header",	&pk_chstruct_ptr);
			dest_addr = pk_chstruct_ptr->rx_addr;

			/* If the frame is destined for this station and the station is expecting	*/
			/* Cts frame then set appropriate indicators.								*/
			if ((dest_addr == my_address) && (expected_frame_type == rcvd_frame_type)) 				
				{
				/* The receipt of Cts frame indicates that Rts is successfully	*/
				/* transmitted and the station can now respond with Data frame	*/
				fresp_to_send = WlanC_Data;

				/* Set the flag indicating that Rts is succesfully transmitted	*/
				wlan_flags->rts_sent = OPC_BOOLINT_ENABLED;

				op_stat_write (retrans_handle, (double) (retry_count * 1.0));
				op_stat_write (retrans_handle, 0.0);

				/* Printing out information to ODB.	*/
				if (wlan_trace_active == OPC_TRUE)
					{	
					sprintf (msg_string, "Cts is received for Data packet %d", pkt_in_service);
					op_prg_odb_print_major (msg_string, OPC_NIL);
					}
				}
			else
				{
				/* Printing out information to ODB.	*/
				if (wlan_trace_active == OPC_TRUE)
					{
					sprintf (msg_string, "Cts is received and discarded.");
					op_prg_odb_print_major (msg_string, OPC_NIL);
					}
				
				//$$$$$$$$$$$$$$$$$$ DSR $$$$$$$$$$$$$$$$$$$$$$$$
				/* No response needed as the frame is either not destined for this station	*/
				/* and/or the station is not expecting this frame.							*/
				//fresp_to_send = WlanC_None;
				//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
				
				/* Check whether we were expecting another frame. If yes then	*/
				/* we need to retransmit the frame for which we were expecting	*/
				/* a reply.														*/
				if (expected_frame_type != WlanC_None)
					{				
					/* Since the station did not receive the expected frame it has to retransmit the packet	*/
					retry_count = retry_count + 1;				

					/* If Rts sent flag was enable then disable it as the station will recontend for the channel.	*/
					if (wlan_flags->rts_sent == OPC_BOOLINT_ENABLED)
						{
						wlan_flags->rts_sent = OPC_BOOLINT_DISABLED;
						}
				
					/* Reset the NAV duration so that the				*/
					/* retransmission is not unnecessarily delayed.		*/
					nav_duration = current_time;
					}
				}
						
			/* If network allocation vector is less than the received duration	*/
			/* value then update its value.  									*/
			if (nav_duration < (pk_chstruct_ptr->duration + current_time))
				{
				nav_duration = pk_chstruct_ptr->duration + current_time;				

				/* Set the flag that indicates updated NAV value.			*/
				wlan_flags->nav_updated = OPC_BOOLINT_ENABLED;
				}			
			break;
			}

		case WlanC_Ack:
			{
			//$$$$$$$$$$$$$$$$$$ DSR $$$$$$$$$$$$$$$$$$$$$$$$
			/* No response needed for ack frame.  */
			//fresp_to_send = WlanC_None;	
			//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
			
			op_pk_nfd_access (wlan_rcvd_frame_ptr,"Wlan Header", &pk_chstruct_ptr); 		
			dest_addr = pk_chstruct_ptr->rx_addr;
			
			/*  Control Traffic received report in terms of number of bits.		*/
			op_stat_write (ctrl_traffic_rcvd_handle_inbits, (double) op_pk_total_size_get (wlan_rcvd_frame_ptr));
			op_stat_write (ctrl_traffic_rcvd_handle_inbits, 0.0);

			/*  Control Traffic received report in terms of number of packets.	*/
			op_stat_write (ctrl_traffic_rcvd_handle, 1.0);
			op_stat_write (ctrl_traffic_rcvd_handle, 0.0);

			if ((dest_addr == my_address) && (rcvd_frame_type == expected_frame_type))
				{
				/* Printing out information to ODB.	*/
				if (wlan_trace_active == OPC_TRUE)
					{
					sprintf (msg_string, "Ack received for data packet %d", pkt_in_service);
					op_prg_odb_print_major (msg_string, OPC_NIL);
					}			

				op_stat_write (retrans_handle, (double) (retry_count * 1.0));
				op_stat_write (retrans_handle, 0.0);

				/* Reset the retry counter as the expected frame is received	*/
				retry_count = 0;

				/* Decrement number of fragment count because one fragment is successfully transmitted.	*/
				num_fragments = num_fragments - 1;				

				/* When there are no more fragments to transmit then disable the Rts sent flag	*/
				/* if it was enabled because the contention period due to Rts/Cts exchange is 	*/
				/* over and another Rts/Cts exchange is needed for next contention period.		*/
				if ((num_fragments == 0) && (wlan_flags->rts_sent == OPC_BOOLINT_ENABLED))
					{
					wlan_flags->rts_sent = OPC_BOOLINT_DISABLED;

					/* Set the contention window flag. Since the ACK for the last 		*/
					/* fragment indicates a	sucessful transmission of the entire data, 	*/
					/* we need to back-off for a contention window period.				*/
					wlan_flags->cw_required = OPC_TRUE;
					}

				/* Data packet is successfully delivered to remote station,			*/
				/* since no further retransmission is needed the copy of the data	*/
				/* packet will be destroyed.										*/
				op_pk_destroy (wlan_transmit_frame_copy_ptr);
				wlan_transmit_frame_copy_ptr = OPC_NIL;
				
				//$$$$$$$$$$$$$$$$$$ DSR $$$$$$$$$$$$$$$$$$$$$$$$
				// DSR: send an ack intrpt and an ici to the DSR routing layer
				if ((num_fragments==0)&&(data_packet_type==DATA_PACKET_TYPE))
					{
					iciptr = op_ici_create("Dsr_Ack_Ici");
					op_ici_attr_set(iciptr,"Relay_Destination",data_packet_dest);    
					op_ici_attr_set(iciptr,"Final_Destination",data_packet_final_dest);
					op_ici_install(iciptr);
					// schedule an intrpt for the DSR process model	
					process_id = op_id_from_name(op_topo_parent (op_id_self()),OPC_OBJTYPE_QUEUE,"dsr_routing");
					op_intrpt_schedule_remote(op_sim_time(),ACK_CODE,process_id);
					if (1)
						{
						op_prg_odb_bkpt("ack_mac");
						}
					}
				 //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
				
				
				}
			
			else
				{
				/* Printing out information to ODB.	*/
				if (wlan_trace_active == OPC_TRUE)
					{
					sprintf (msg_string, "Ack is received and discarded.");
					op_prg_odb_print_major (msg_string, OPC_NIL);
					}
				
				/* Check whether we were expecting another frame. If yes then	*/
				/* we need to retransmit the frame for which we were expecting	*/
				/* a reply.														*/
				if (expected_frame_type != WlanC_None)
					{				
					/* Since the station did not receive the expected frame it has to retransmit the packet	*/
					retry_count = retry_count + 1;				

					/* If Rts sent flag was enable then disable it as the station will recontend for the channel.	*/
					if (wlan_flags->rts_sent == OPC_BOOLINT_ENABLED)
						{
						wlan_flags->rts_sent = OPC_BOOLINT_DISABLED;
						}
				
					/* Reset the NAV duration so that the				*/
					/* retransmission is not unnecessarily delayed.		*/
					nav_duration = current_time;
					}
				}

			/* If network allocation vector is less than the received duration	*/
			/* value then update its value. 									*/
			if (nav_duration < (pk_chstruct_ptr->duration + current_time))
				{
				nav_duration = pk_chstruct_ptr->duration + current_time;

				/* Set the flag that indicates updated NAV value.			*/
				wlan_flags->nav_updated = OPC_BOOLINT_ENABLED;
				}
			break;
			}

		default:
			{
			wlan_mac_error ("Unexpected frame type received.", OPC_NIL, OPC_NIL);
			break;
			}
	    }

	/* Reporting the amount of time the channel will be busy.	*/	   
	op_stat_write (channel_reserv_handle, (nav_duration - current_time));
	op_stat_write (channel_reserv_handle, 0.0);

	/* Check whether further retries are possible or	*/
	/* the data frame needs to be discarded.			*/
	wlan_frame_discard ();

	/* Set the expected frame type to None because either the 	*/
	/* expected frame is recieved or the station will have to 	*/
	/* retransmit the frame										*/
	expected_frame_type = WlanC_None;

	/* Destroying the received frame once relevant information is taken out of it.	*/
	op_pk_destroy (wlan_rcvd_frame_ptr);														

	FOUT;
	}

Boolean
wlan_tuple_find (int sta_addr, int seq_id, int frag_num)
	{
	Boolean 							result = OPC_BOOLINT_DISABLED;
	int 								list_index;
	int 								list_size;
	WlanT_Mac_Duplicate_Buffer_Entry*	tuple_ptr;

	/** This routine determines whether the received data frame already exists in the duplicate buffer.	**/
	/** If it is not then it will be added to the list and the list is updated such that its size will	**/
	/** will not be greater then the MAX TUPLE SIZE.													**/
	FIN (wlan_tuple_find (sta_addr, seq_id, frag_num));

	/* Finding the index of the station address in the list,	*/
	/* if the station belongs to this subnet.					*/
	list_index = oms_aa_address_find (oms_aa_wlan_handle, sta_addr);

	/* If remote station entry doesn't exist then create new node.	*/
	if (list_index >= 0)					
		{			
		if (duplicate_list_ptr [list_index] == OPC_NIL)
			{	
			/* Creating struct type for duplicate frame (or tuple) structure. 	*/
			tuple_ptr = (WlanT_Mac_Duplicate_Buffer_Entry *)
				                       op_prg_mem_alloc (sizeof (WlanT_Mac_Duplicate_Buffer_Entry));

			/* Generate error and abort simulation if no more memory left to allocate for duplicate buffer	*/
			if (tuple_ptr == OPC_NIL)
				{
				wlan_mac_error ("Cannot allocate memory for duplicate buffer entry", OPC_NIL, OPC_NIL);
				}	

			tuple_ptr->tx_station_address 	= remote_sta_addr;						
			tuple_ptr->sequence_id 			= seq_id;
			tuple_ptr->fragment_number 		= frag_num;

			/* Insert new entry in the list.	*/					
			duplicate_list_ptr [list_index] = tuple_ptr;						
			}			
		else
			{
			if (duplicate_list_ptr [list_index]->sequence_id == seq_id &&
				duplicate_list_ptr [list_index]->fragment_number == frag_num)
				{
				/* This will be set in the retry field of Acknowledgement.	*/
				duplicate_entry = 1;

				/* Break the routine as the packet is already received by the station.	*/
				FRET (OPC_TRUE);
				}
			else
				{
				/* Update the sequence id and fragment number fields of the	*/
				/* remote station in the duplicate buffer list. The list	*/
				/* maintains the sequence id and fragment number of the 	*/
				/* previously received frame from this remote station. 		*/
				duplicate_list_ptr [list_index]->sequence_id = seq_id;
				duplicate_list_ptr [list_index]->fragment_number = frag_num;							
				}
			}
        }
	else
		{
		/* Its not possible for a station to directly receive packet from a station that	*/
		/* does not exist in its BSS.														*/
		wlan_mac_error ("Receiving packet from a station that does not exist in this BSS", "Possibly wrong destination address", "Please check the configuration");
		}

	/* This will be set in the retry field of Acknowledgement.	*/
	duplicate_entry = 0;

	/* Packet is not already received by the station.	*/ 
	FRET (OPC_FALSE);				
	}

	
static void
wlan_data_process (Packet* seg_pkptr, int sta_addr, int final_dest_addr, int frag_num, int more_frag, int pkt_id, int rcvd_sta_bssid)
	{
	char										msg_string [120];
	int	 										current_index;
	int 										list_index;
	int 										list_size;
	int											protocol_type;
	WlanT_Mac_Defragmentation_Buffer_Entry*     defrag_ptr;
	
	/** This routine handles defragmentation process and also sends data to the	**/
	/** higher layer if all the fragments have been received by the station.	**/
	FIN (wlan_data_process (seg_pkptr, sta_addr, final_dest_addr, frag_num, more_frag, pkt_id, rcvd_sta_bssid));

	/* Defragmentation of the received data frame.					*/
	/* Inserting fragments into the reassembly buffer. There are	*/
	/* two possible cases:											*/
	/* 1. The remote station has just started sending the 			*/
	/* fragments and it doesn't exist in the list.					*/
	/* 2. The remote station does exist in the list and the 		*/
	/* and the new fragment is a series of fragments for the data 	*/
	/* packet.													  	*/

	/* Get the size of the defragmentation list.	*/
	list_size = op_prg_list_size (defragmentation_list_ptr);

	/* Initialize the current node index which will indicate whether	*/
	/* the entry for the station exists in the list.					*/
	current_index = -1;

	/* Searching through the list to find if the remote station address 	*/
	/* exists i.e. the source station has received fragments for this   	*/
	/* data packet before.													*/
	/* Also, removing entries from the defragmentation buffer which has		*/
	/* reached its maximum receieve lifetime.								*/
	for (list_index = 0; list_index < list_size; list_index++)
		{
		/* Accessing node of the list for search purposes.	*/						
		defrag_ptr = (WlanT_Mac_Defragmentation_Buffer_Entry*) 
						op_prg_list_access (defragmentation_list_ptr, list_index);

		/* Removing station entry if the receive lifetime has expired.	*/
		if ((current_time - defrag_ptr->time_rcvd) >= max_receive_lifetime)
			{
			/* Removing the partially completed fragment once its lifetime has reached.	*/
			defrag_ptr =(WlanT_Mac_Defragmentation_Buffer_Entry *)
					op_prg_list_remove (defragmentation_list_ptr, list_index);
			op_sar_buf_destroy (defrag_ptr->reassembly_buffer_ptr);					
			op_prg_mem_free (defrag_ptr);	

			/* Updating the total list size.	*/
			list_size = list_size - 1;
			}

		/* If the station entry already exists in the list then store its index for future use.	*/
		else if (remote_sta_addr == defrag_ptr->tx_station_address)
			{
			current_index = list_index;
			}
		}                             						

	/* If remote station entry doesn't exist then create new node	*/
	if (current_index == -1)											
		{
		/* If the entry of the station does not exist in the defrag list		*/
		/* and the fragment received is not the first fragment of the packet    */
		/* then it implies that the maximum receive lifetime of the packet		*/
		/* has expired. In this case the received packet will be destroyed and	*/
		/* the acknowledgement is sent to the receiver as specified by the		*/
		/* protocol.															*/
		if (frag_num > 0)
			{
			op_pk_destroy (seg_pkptr);
			FOUT;
			}

		/* Creating struct type for defragmentation structure	*/  						
		defrag_ptr = (WlanT_Mac_Defragmentation_Buffer_Entry *) op_prg_mem_alloc (sizeof (WlanT_Mac_Defragmentation_Buffer_Entry));

		/* Generate error and abort simulation if no more memory left to allocate for duplicate buffer	*/
		if (defrag_ptr == OPC_NIL)
			{
			wlan_mac_error ("Cannot allocate memory for defragmentation buffer entry", OPC_NIL, OPC_NIL);
			}	

		/* Source station address is store in the list for future reference.	*/
		defrag_ptr->tx_station_address = sta_addr;

		/* For new node creating a reassembly buffer	*/
		defrag_ptr->reassembly_buffer_ptr = op_sar_buf_create (OPC_SAR_BUF_TYPE_REASSEMBLY, OPC_SAR_BUF_OPT_DEFAULT);
		op_prg_list_insert (defragmentation_list_ptr, defrag_ptr, OPC_LISTPOS_TAIL);
		}

    /* Record the received time of this fragment.	*/
	defrag_ptr->time_rcvd = current_time;
					
	/* Insert fragment into the reassembly buffer 	*/
	op_sar_rsmbuf_seg_insert (defrag_ptr->reassembly_buffer_ptr, seg_pkptr);

	/* If this is the last fragment then send the data to higher layer.	*/
	if (more_frag == 0)
		{
		/* If no more fragments to rcv then send the data to higher	*/
		/* layer and increment rcvd fragment count.					*/
		seg_pkptr = op_sar_rsmbuf_pk_remove (defrag_ptr->reassembly_buffer_ptr);

		if (ap_flag == OPC_BOOLINT_ENABLED)
			{
			/* If the address is not found in the address list then access point will sent the data to higher	*/
			/* layer for address resolution. Note that if destination address is same as AP's address then		*/
			/* the packet is sent to higher layer for address resolution.										*/
			if ((oms_aa_address_find (oms_aa_wlan_handle, final_dest_addr) >= 0) && (final_dest_addr != my_address))
				{
				/* Printing out information to ODB.	*/
				if (wlan_trace_active == OPC_TRUE)
					{
					sprintf (msg_string, "All fragments of Data packet %d is received and enqueued for transmission within a subnet", pkt_id);
					op_prg_odb_print_major (msg_string, OPC_NIL);
					}

				/* Enqueuing packet for transmission within a subnet.	*/
				wlan_hlpk_enqueue (seg_pkptr, final_dest_addr);
				}
			else
				{
				
				/* Update the local/global throughput and end-to-end	*/
				/* delay statistics based on the packet that will be	*/
				/* forwarded to the higher layer.						*/
				wlan_accepted_frame_stats_update (seg_pkptr);

				/* Set the contents of the LLC-destined ICI -- set the address	*/
				/* of the transmitting station.									*/
				if (op_ici_attr_set (llc_iciptr, "src_addr", remote_sta_addr) == OPC_COMPCODE_FAILURE)
					{
					wlan_mac_error ("Unable to set source address in LLC ICI.", OPC_NIL, OPC_NIL);
					}

				/* Set the destination address (this mainly serves to			*/
				/* distinguish packets received under broadcast conditions.)	*/
				if (op_ici_attr_set (llc_iciptr, "dest_addr", final_dest_addr) == OPC_COMPCODE_FAILURE)
					{
					wlan_mac_error("Unable to set destination address in LLC ICI.", OPC_NIL, OPC_NIL);
					}
		
				/* Set the protocol type field contained in the Wlan frame.	*/
				protocol_type = 0;
				if (op_ici_attr_set (llc_iciptr, "protocol_type", protocol_type) == OPC_COMPCODE_FAILURE)
					{
					wlan_mac_error("Unable to set protocol type in LLC ICI.", OPC_NIL, OPC_NIL);
					}				

				/* Printing out information to ODB.	*/
				if (wlan_trace_active == OPC_TRUE)
					{
					sprintf (msg_string, "All fragments of Data packet %d is received and sent to the higher layer", pkt_id);
					op_prg_odb_print_major (msg_string, OPC_NIL);
					}
				
				/* Setting an ici for the higher layer */
				op_ici_install (llc_iciptr);

				/* Sending data to higher layer through mac interface.	*/
				op_pk_send (seg_pkptr, outstrm_to_mac_if);
				}
			}
		else
			{
			/* If the station is a gateway and not an access point then do not send		*/
			/* data to higher layer for address resolution.  This is for not allowing   */
			/* data to go out of the Adhoc BSS.											*/
			if ((wlan_flags->gateway_flag == OPC_BOOLINT_ENABLED) || 
				(wlan_flags->bridge_flag == OPC_BOOLINT_ENABLED))
				{				
				/* Printing out information to ODB.	*/
				if (wlan_trace_active == OPC_TRUE)
					{
					sprintf (msg_string, "Gateway is not an access point so all received fragments are discarded.");
					op_prg_odb_print_major (msg_string, OPC_NIL);
					}
			   	op_pk_destroy (seg_pkptr);
				}
			else
				{
				/* Update the local/global throughput and end-to-end	*/
				/* delay statistics based on the packet that will be	*/
				/* forwarded to the higher layer.						*/
				wlan_accepted_frame_stats_update (seg_pkptr);
				
				/* Printing out information to ODB.	*/
				if (wlan_trace_active == OPC_TRUE)
					{
					sprintf (msg_string, "All fragments of Data packet %d is received and sent to the higher layer", pkt_id);
					op_prg_odb_print_major (msg_string, OPC_NIL);
					}
				
				/* Sending data to higher layer through mac interface	*/
				op_pk_send (seg_pkptr, outstrm_to_mac_if);
				}
			}
			
		/* Freeing up memory space once the received data frame is sent to higher layer.	*/
		defrag_ptr =(WlanT_Mac_Defragmentation_Buffer_Entry *)
		op_prg_list_remove (defragmentation_list_ptr, current_index);
		op_sar_buf_destroy (defrag_ptr->reassembly_buffer_ptr);					
		op_prg_mem_free (defrag_ptr);	
		}
	else
		{
		/* Printing out information to ODB.	*/
		if (wlan_trace_active == OPC_TRUE)
			{			
			sprintf (msg_string, "Data packet %d is received and waiting for more fragments ", pkt_id);
			op_prg_odb_print_major (msg_string, OPC_NIL);
			}
		}

	FOUT;
	}

static void
wlan_accepted_frame_stats_update (Packet* seg_pkptr)
	{
	double	ete_delay, pk_size;
	
	/** This function is called just before a frame received from	**/
	/** physical layer being forwarded to the higher layer to		**/
	/** update end-to-end delay and throughput statistics.			**/
	FIN (wlan_accepted_frame_stats_update (seg_pkptr));
	
	/* Total number of bits sent to higher layer is equivalent to a	*/
	/* throughput.													*/
	pk_size = (double) op_pk_total_size_get (seg_pkptr);
	op_stat_write (throughput_handle, pk_size);
	op_stat_write (throughput_handle, 0.0);

	/* Also update the global WLAN throughput statistic.			*/
	op_stat_write (global_throughput_handle, pk_size);
	op_stat_write (global_throughput_handle, 0.0);
	
	/* Compute the end-to-end delay for the frame and record it.	*/
	ete_delay = current_time - op_pk_stamp_time_get (seg_pkptr);
	op_stat_write (ete_delay_handle, 		ete_delay);
	op_stat_write (ete_delay_handle,		0.0);
	op_stat_write (global_ete_delay_handle, ete_delay);
	op_stat_write (global_ete_delay_handle, 0.0);
	
	FOUT;
	}

static void 
wlan_schedule_deference ()
	{
	/** This routine schedules self interrupt for deference **/
	/** to avoid collision and also deference to observe	**/
	/** interframe gap between the frame transmission.	**/
	FIN (wlan_schedule_deference ());

	/* Check the status of the receiver. If it is busy, exit the	*/
	/* function, since we will schedule the end of the deference	*/
	/* when it becomes idle.										*/
	if (wlan_flags->receiver_busy == OPC_BOOLINT_ENABLED)
		{
		FOUT;
		} 

	/* Extracting current time at each interrupt */
	current_time = op_sim_time ();
	
	/* Adjust the NAV if necessary.									*/
	if (nav_duration < rcv_idle_time)
		{
		nav_duration = rcv_idle_time;
		}
	
	/* Station needs to wait SIFS duration before responding to any */	
	/* frame. Also, if Rts/Cts is enabled then the station needs	*/
	/* to wait for SIFS duration after acquiring the channel using	*/
	/* Rts/Cts exchange.						*/
	if ((fresp_to_send != WlanC_None) || (wlan_flags->rts_sent == OPC_BOOLINT_ENABLED))
		{
		deference_evh = op_intrpt_schedule_self (current_time + sifs_time, WlanC_Deference_Off);
		
		/* Disable backoff flag because this frame is a response frame to the	*/
		/* previously received frame (this could be Ack or Cts)					*/
		wlan_flags->backoff_flag = OPC_BOOLINT_DISABLED;
		}

	/* If more fragments to send then wait for SIFS duration and transmit. 	*/
	/* Station need to contend for the channel if one of the fragments is  	*/
	/* not successfully transmitted.					*/
	else if ((retry_count == 0) && (op_sar_buf_size (fragmentation_buffer_ptr) > 0))
		{
		/* Scheduling a self interrupt after SIFS duration */
		deference_evh = op_intrpt_schedule_self (current_time + sifs_time, WlanC_Deference_Off);
		
		/* Disable backoff because the frame need to be transmitted after SIFS duration	*/
		/* This frame is part of the fragment burst 					*/
		wlan_flags->backoff_flag = OPC_BOOLINT_DISABLED;
		}
    else
		{
		/* If the station needs to transmit or retransmit frame, it will */
		/* defer for nav duration plus  DIFS duration and then backoff   */
		// ############# DEBUG to avoid negative interupt
		if ((nav_duration + difs_time)>current_time)
			{
			deference_evh = op_intrpt_schedule_self ((nav_duration + difs_time), WlanC_Deference_Off);		
			}
		else
			{
			deference_evh = op_intrpt_schedule_self ((current_time + difs_time), WlanC_Deference_Off);
			}
		
		/* Before sending data frame or Rts backoff is needed. */
		wlan_flags->backoff_flag = OPC_BOOLINT_ENABLED;
		}
	
	/* Reset the updated NAV flag, since as of now we scheduled a new	*/
	/* "end of deference" interrupt after the last update.				*/
	wlan_flags->nav_updated = OPC_BOOLINT_DISABLED;
	
	FOUT;
	}

static void
wlan_frame_discard ()
	{
	int seg_bufsize;
	Packet* seg_pkptr;
	//$$$$$$$$$$$$$$$$$$ DSR $$$$$$$$$$$$$$$$$$$$$$$$
	Objid process_id;
	Ici* iciptr;
	//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

	/** No further retries for the data frame for which the retry limit has reached.	**/
	/** As a result these frames are discarded.											**/
	FIN (wlan_frame_discard ());

	/* If retry limit has reached then drop the frame.	*/
	if (retry_count == retry_limit)
		{
		/* Update retransmission count statistic.	*/						
		op_stat_write (retrans_handle, (double) (retry_count * 1.0));
		op_stat_write (retrans_handle, 0.0);

		/* Update the local and global dropped packet statistics.	*/
		op_stat_write (drop_packet_handle, 1.0);
	    op_stat_write (drop_packet_handle, 0.0);
		op_stat_write (drop_packet_handle_inbits, (double) packet_size);
	    op_stat_write (drop_packet_handle_inbits, 0.0);
		op_stat_write (global_dropped_data_handle, (double) packet_size);
		op_stat_write (global_dropped_data_handle, 0.0);
		
		//$$$$$$$$$$$$$$$$$$ DSR $$$$$$$$$$$$$$$$$$$$$$$$
		// DSR: Send an error intrpt to the DSR routing layer and collect statistic
		if (data_packet_type==DATA_PACKET_TYPE)
			{
			iciptr = op_ici_create("Dsr_Error_Ici");
			op_ici_attr_set(iciptr,"Relay_Destination",data_packet_dest);    
			op_ici_attr_set(iciptr,"Final_Destination",data_packet_final_dest);
			op_ici_install(iciptr);
			process_id=op_id_from_name(op_topo_parent (op_id_self()),OPC_OBJTYPE_QUEUE,"dsr_routing");
			op_intrpt_schedule_remote(op_sim_time(),ERROR_CODE,process_id);
			}
		if ((data_packet_type!=0)&&(retry_count!=0)&&(expected_frame_type!=0))
			{
			if (data_packet_type==DATA_PACKET_TYPE) op_stat_write(stat_mac_failed_data,++mac_failed_data);
			else if (data_packet_type==REPLY_PACKET_TYPE) op_stat_write(stat_mac_failed_reply,++mac_failed_reply);
			else if (data_packet_type==ERROR_PACKET_TYPE) op_stat_write(stat_mac_failed_error,++mac_failed_error);
			op_stat_write(stat_mac_total_failed,++mac_total_failed);
			}
		if (1)
			{
			op_prg_odb_bkpt("mac_failed");
			}
		//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
		
		/* Reset the retry count for the next packet.	*/
		retry_count = 0;

		/* Get the segmenation buffer size to check if there are more fragments left to be transmitted.	*/
		seg_bufsize =  (int) op_sar_buf_size (fragmentation_buffer_ptr); 

		if (seg_bufsize != 0)
			{
			/* Discard remaining fragments	*/
			seg_pkptr = op_sar_srcbuf_seg_remove (fragmentation_buffer_ptr, seg_bufsize);
			op_pk_destroy (seg_pkptr);
			}

		/* If expecting Ack frame then destroy the tx data frame as this frame will	*/
		/* no longer be transmitted (even if we are not expecting an Ack at this	*/
		/* moment, we still may have a copy of the frame if at one point in the		*/
		/* retransmission history of the original packet we received a Cts for our	*/
		/* Rts but then didn't receive an Ack for our data transmission; hence		*/
		/* consider this case as well).												*/
		if ((expected_frame_type == WlanC_Ack) || (wlan_transmit_frame_copy_ptr != OPC_NIL))
			{	
			/* Destroy the copy of the frame as the packet is discarded.	*/
			op_pk_destroy (wlan_transmit_frame_copy_ptr);
			wlan_transmit_frame_copy_ptr = OPC_NIL;
			}
			
		/* Reset the flag that indicates successful RTS transmission.		*/
		wlan_flags->rts_sent = OPC_BOOLINT_DISABLED;
		
		/* Reset the "frame to respond" variable unless we have a CTS or  	*/
		/* ACK to send.														*/
		if (fresp_to_send == WlanC_Data)
			{
			fresp_to_send = WlanC_None;
			}
		
		/* If there is not any other data packet sent from higher layer and	*/
		/* waiting in the buffer for transmission, reset the related flag.	*/
		if (op_prg_list_size (hld_list_ptr) == 0)
			{
			wlan_flags->data_frame_to_send = OPC_BOOLINT_DISABLED;
			}
		}
	
	//$$$$$$$$$$$$$$$$$$ DSR $$$$$$$$$$$$$$$$$$$$$$$$
	// DSR: stat collection
	if ((data_packet_type!=0)&&(retry_count>0)&&(expected_frame_type!=0))
		{
		if (expected_frame_type == WlanC_Cts) op_stat_write(stat_mac_retry_rts,++mac_retry_rts);
		else if (expected_frame_type == WlanC_Ack)
			{
			if (data_packet_type==DATA_PACKET_TYPE) op_stat_write(stat_mac_retry_data,++mac_retry_data);
			else if (data_packet_type==REPLY_PACKET_TYPE) op_stat_write(stat_mac_retry_reply,++mac_retry_reply);
			else if (data_packet_type==ERROR_PACKET_TYPE) op_stat_write(stat_mac_retry_error,++mac_retry_error);
			}
		op_stat_write(stat_mac_retry_rts,++mac_total_retry);
		}
	//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
	
	FOUT;
	}

/****** Error handling procedure ******/
static void
wlan_mac_error (char* msg1, char* msg2, char* msg3)
	{
	/** Terminates simulation with an error message.	**/
	FIN (wlan_mac_error (msg1, msg2, msg3));

	op_sim_end ("Error in Wireless LAN MAC process:", msg1, msg2, msg3);

	FOUT;
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
	void wlan_mac_lpw (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Obtype wlan_mac_lpw_init (int * init_block_ptr);
	VosT_Address wlan_mac_lpw_alloc (VOS_THREAD_INDEX_ARG_COMMA VosT_Obtype, int);
	void wlan_mac_lpw_diag (OP_SIM_CONTEXT_ARG_OPT);
	void wlan_mac_lpw_terminate (OP_SIM_CONTEXT_ARG_OPT);
	void wlan_mac_lpw_svar (void *, const char *, void **);


	VosT_Fun_Status Vos_Define_Object (VosT_Obtype * obst_ptr, const char * name, unsigned int size, unsigned int init_obs, unsigned int inc_obs);
	VosT_Address Vos_Alloc_Object_MT (VOS_THREAD_INDEX_ARG_COMMA VosT_Obtype ob_hndl);
	VosT_Fun_Status Vos_Poolmem_Dealloc_MT (VOS_THREAD_INDEX_ARG_COMMA VosT_Address ob_ptr);
#if defined (__cplusplus)
} /* end of 'extern "C"' */
#endif




/* Process model interrupt handling procedure */


void
wlan_mac_lpw (OP_SIM_CONTEXT_ARG_OPT)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	FIN_MT (wlan_mac_lpw ());
	if (1)
		{
		/* variables used for registering and discovering process models */
		OmsT_Pr_Handle			process_record_handle;
		List*					proc_record_handle_list_ptr;
		int						record_handle_list_size;
		int						ap_count;
		int						count;
		double					sta_addr;
		double					statype ;
		Objid					mac_if_module_objid;
		char					proc_model_name [300];
		Objid					my_subnet_objid;
		Objid					my_objid; 
		Objid					my_node_objid;
		Objid					params_attr_objid;
		Objid					wlan_params_comp_attr_objid;
		Objid					strm_objid;
		int						strm;
		int						i,j;
		int						addr_index;
		int						num_out_assoc;
		int						node_count;
		int						node_objid;
		WlanT_Hld_List_Elem*	hld_ptr;
		Prohandle				own_prohandle;
		double					timer_duration;
		double					cw_slots;
		char					msg1 [120];
		char					msg2 [120];
		WlanT_Phy_Char_Code		sta_phy_char_flag;


		FSM_ENTER ("wlan_mac_lpw")

		FSM_BLOCK_SWITCH
			{
			/*---------------------------------------------------------*/
			/** state (INIT) enter executives **/
			FSM_STATE_ENTER_UNFORCED_NOLABEL (0, "INIT", "wlan_mac_lpw [INIT enter execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw [INIT enter execs], state0_enter_exec)
				{
				/* Initialization of the process model.				*/  
				/* All the attributes are loaded in this routine	*/
				wlan_mac_sv_init (); 
				 
				/* Schedule a self interrupt to wait for mac interface 	*/
				/* to move to next state after registering				*/
				op_intrpt_schedule_self (op_sim_time (), 0);
				
				}

				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [INIT enter execs], state0_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (1,"wlan_mac_lpw")


			/** state (INIT) exit executives **/
			FSM_STATE_EXIT_UNFORCED (0, "INIT", "wlan_mac_lpw [INIT exit execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw [INIT exit execs], state0_exit_exec)
				{
				/* object id of the surrounding processor	*/
				my_objid = op_id_self ();
				
				/* Obtain the node's object identifier	*/
				my_node_objid = op_topo_parent (my_objid);
				
				my_subnet_objid = op_topo_parent (my_node_objid);
				
				/* Obtain the process's process handle	*/
				own_prohandle = op_pro_self ();
				
				/* Obtain the values assigned to the various attributes	*/
				op_ima_obj_attr_get (my_objid, "Wireless LAN Parameters", &wlan_params_comp_attr_objid);
				params_attr_objid = op_topo_child (wlan_params_comp_attr_objid, OPC_OBJTYPE_GENERIC, 0);
				
				/* Obtain the name of the process	*/
				op_ima_obj_attr_get (my_objid, "process model", proc_model_name);
				
				/* Determine the assigned MAC address which will be used for address resolution.	*/
				/* Note this is not the final MAC address as there may be static assignments in 	*/
				/* the network.																		*/
				op_ima_obj_attr_get (my_objid, "station_address", &my_address);
				
				/* Perform auto-addressing for the MAC address. Apart	*/
				/* from dynamically addressing, if auto-assigned, the	*/
				/* address resolution function also detects duplicate	*/
				/* static assignments. The function also initializes 	*/
				/* every MAC address as a valid destination.			*/
				oms_aa_address_resolve (oms_aa_handle, my_objid, &my_address);
				
				/* Register Wlan MAC process in the model wide registry	*/
				process_record_handle = (OmsT_Pr_Handle) oms_pr_process_register (
				my_node_objid, my_objid, own_prohandle, proc_model_name);
				
				/* If this station is an access point then it has to be registered as an Access Point.	*/
				/* This is because the network will be treated as Infrastructure network once AP is		*/
				/* detected.																			*/
				if (ap_flag == OPC_BOOLINT_ENABLED)
					{
					/* Register this protocol attribute and the station address of	*/
					/* this process into the model-wide registry.					*/
					oms_pr_attr_set (process_record_handle,
						"protocol",				OMSC_PR_STRING,			"mac",
						"mac_type",				OMSC_PR_STRING,			"wireless_lan",
						"subprotocol", 			OMSC_PR_NUMBER,			 (double) WLAN_AP,
						"subnetid",				OMSC_PR_OBJID,			 my_subnet_objid,
						"address",				OMSC_PR_NUMBER,			(double) my_address,
						"auto address handle",	OMSC_PR_ADDRESS,		oms_aa_handle,
						OPC_NIL);                                       
					}
				else
					{
					/* Register this protocol attribute and the station address of	*/
					/* this process into the model-wide registry.					*/
					oms_pr_attr_set (process_record_handle,
						"protocol",				OMSC_PR_STRING,			"mac",
						"mac_type",				OMSC_PR_STRING,			"wireless_lan",
						"subprotocol", 			OMSC_PR_NUMBER,			(double) WLAN_STA,
						"subnetid",				OMSC_PR_OBJID,			 my_subnet_objid,
						"address",				OMSC_PR_NUMBER,			(double) my_address,
						"auto address handle",	OMSC_PR_ADDRESS,		oms_aa_handle,
						OPC_NIL);                                       
					}
				
				/* Obtain the MAC layer information for the local MAC	*/
				/* process from the model-wide registry.				*/
				/* This is to check if the node is a gateway or not.	*/
				proc_record_handle_list_ptr = op_prg_list_create ();
				
				oms_pr_process_discover (OPC_OBJID_INVALID, proc_record_handle_list_ptr, 
					"node objid",					OMSC_PR_OBJID,			 my_node_objid,
					"protocol", 					OMSC_PR_STRING, 		 "bridge",
				 	 OPC_NIL);
				
				/* If the MAC interface process registered itself,	*/
				/* then there must be a valid match					*/
				record_handle_list_size = op_prg_list_size (proc_record_handle_list_ptr);
				
				if (record_handle_list_size != 0)
					{
					wlan_flags->bridge_flag = OPC_BOOLINT_ENABLED;
					}
				
				/* If the station is not a bridge only then check for arp	*/
				if (wlan_flags->bridge_flag == OPC_BOOLINT_DISABLED)
					{
					/* Deallocate memory used for process discovery	*/
					while (op_prg_list_size (proc_record_handle_list_ptr))
						{
						op_prg_list_remove (proc_record_handle_list_ptr, OPC_LISTPOS_HEAD);
						}
					op_prg_mem_free (proc_record_handle_list_ptr);
				
					/* Obtain the MAC layer information for the local MAC	*/
					/* process from the model-wide registry.				*/
					proc_record_handle_list_ptr = op_prg_list_create ();
					
					oms_pr_process_discover (my_objid, proc_record_handle_list_ptr, 
						"node objid",			OMSC_PR_OBJID,			my_node_objid,
						"protocol", 			OMSC_PR_STRING,			"arp", 
						OPC_NIL);
				
					/* If the MAC interface process registered itself,	*/
					/* then there must be a valid match					*/
					record_handle_list_size = op_prg_list_size (proc_record_handle_list_ptr);
				
					}
				
				
				if (record_handle_list_size != 1)
					{
					/* An error should be created if there are more	*/
					/* than one WLAN-MAC process in the local node,	*/
					/* or if no match is found.						*/
					wlan_mac_error ("Either zero or several WLAN MAC interface processes found in the node.", OPC_NIL, OPC_NIL);
					}
				else
					{
					/*	Obtain a handle on the process record	*/
					process_record_handle = (OmsT_Pr_Handle) op_prg_list_access (proc_record_handle_list_ptr, OPC_LISTPOS_HEAD);
				
					/* Obtain the module objid for the Wlan MAC Interface module	*/
					oms_pr_attr_get (process_record_handle, "module objid", OMSC_PR_OBJID, &mac_if_module_objid);
				
					/* Obtain the stream numbers connected to and from the	*/
					/* Wlan MAC Interface layer process						*/
					oms_tan_neighbor_streams_find (my_objid, mac_if_module_objid, &instrm_from_mac_if, &outstrm_to_mac_if);
					}
					
				/* Deallocate memory used for process discovery	*/
				while (op_prg_list_size (proc_record_handle_list_ptr))
					{
					op_prg_list_remove (proc_record_handle_list_ptr, OPC_LISTPOS_HEAD);
					}
				
				op_prg_mem_free (proc_record_handle_list_ptr);
				
				
				if (wlan_trace_active)
					{
					/* Cache the state name from which this function was	*/
					/* called.												*/
					strcpy (current_state_name, "init");  
					}
				
				// $$$$$$$$$$$$$$$$ DSR TEMPORARY "BUG" FIXING $$$$$$$$$$$$$$$$$$$$$$$$$$$
				//instrm_from_mac_if=2; 
				//outstrm_to_mac_if=2;
				// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
				}
				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [INIT exit execs], state0_exit_exec)


			/** state (INIT) transition processing **/
			FSM_TRANSIT_FORCE (8, state8_enter_exec, ;, "default", "", "INIT", "BSS_INIT")
				/*---------------------------------------------------------*/



			/** state (IDLE) enter executives **/
			FSM_STATE_ENTER_UNFORCED (1, "IDLE", state1_enter_exec, "wlan_mac_lpw [IDLE enter execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw [IDLE enter execs], state1_enter_exec)
				{
				/** The purpose of this state is to wait until the packet has	**/
				/** arrived from the higher or lower layer.					 	**/
				/** In this state following intrpts can occur:				 	**/
				/** 1. Data arrival from application layer   				 	**/
				/** 2. Frame (DATA,ACK,RTS,CTS) rcvd from PHY layer			 	**/
				/** 3. Busy intrpt stating that frame is being rcvd			 	**/
				/** 4. Coll intrpt indicating that more than one frame is rcvd  **/
				/* When Data arrives from the application layer, insert it	*/
				/* in the queue.											*/
				/* If rcvr is not busy then	set Deference to DIFS 			*/
				/* and Change state to "DEFER" state						*/
				
				/* Rcvd RTS,CTS,DATA,or ACK (frame rcvd intrpt)				*/
				/* Set Backoff flag if the station needs to backoff			*/
				/* If the frame is destined for this station then send 		*/
				/* appropriate response and set deference to SIFS 			*/
				/* clear the rcvr busy flag and clamp any data transmission	*/
				/*															*/
				/* If it's a broadcast frame then set deference to NAV		*/
				/* and schedule self intrpt and change state to "DEFER".	*/
				/* Copy the frame (RTS/DATA) in retransmission variable		*/
				/* if rcvr start receiving frame (busy stat intrpt) then set*/
				/* a flag indicating rcvr is busy,if rcvr start receiving   */
				/* more than one frame (collision stat intrpt) then set the	*/
				/* rcvd frame as invalid frame set deference time to EIFS	*/
				
				if (wlan_trace_active)
					{
					/* Determine the current state name.	*/
					strcpy (current_state_name, "idle");
					}
				}

				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [IDLE enter execs], state1_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (3,"wlan_mac_lpw")


			/** state (IDLE) exit executives **/
			FSM_STATE_EXIT_UNFORCED (1, "IDLE", "wlan_mac_lpw [IDLE exit execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw [IDLE exit execs], state1_exit_exec)
				{
				/* Interrupt processing routine	*/
				wlan_interrupts_process ();
				
				/* Schedule deference interrupt when there is a frame to transmit	*/
				/* at the stream interrupt and the receiver is not busy				*/
				if (READY_TO_TRANSMIT)
					{
					/* If the medium was idling for a period equal or longer than	*/
					/* DIFS time then we don't need to defer.						*/
					if (MEDIUM_IS_IDLE)
						{
						/* We can start the transmission immediately.				*/
						wlan_flags->immediate_xmt = OPC_TRUE;
						backoff_slots = 0;
						}
					else
						{
						/* We need to defer. Schedule the end of it.				*/
						wlan_schedule_deference ();
						}
					
					/* If we are in the contention window period, cancel the self	*/
					/* interrupt that indicates the end of it. We will reschedule	*/
					/* if it will be necessary.										*/
					if (intrpt_type == OPC_INTRPT_STRM && op_ev_valid (cw_end_evh) == OPC_TRUE)
						{
						op_ev_cancel (cw_end_evh);
						}
					}
				}
				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [IDLE exit execs], state1_exit_exec)


			/** state (IDLE) transition processing **/
			FSM_PROFILE_SECTION_IN (wlan_mac_lpw [IDLE trans conditions], state1_trans_conds)
			FSM_INIT_COND (READY_TO_TRANSMIT && !MEDIUM_IS_IDLE)
			FSM_TEST_COND (READY_TO_TRANSMIT && MEDIUM_IS_IDLE)
			FSM_DFLT_COND
			FSM_TEST_LOGIC ("IDLE")
			FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [IDLE trans conditions], state1_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 2, state2_enter_exec, ;, "READY_TO_TRANSMIT && !MEDIUM_IS_IDLE", "", "IDLE", "DEFER")
				FSM_CASE_TRANSIT (1, 4, state4_enter_exec, ;, "READY_TO_TRANSMIT && MEDIUM_IS_IDLE", "", "IDLE", "TRANSMIT")
				FSM_CASE_TRANSIT (2, 1, state1_enter_exec, ;, "default", "", "IDLE", "IDLE")
				}
				/*---------------------------------------------------------*/



			/** state (DEFER) enter executives **/
			FSM_STATE_ENTER_UNFORCED (2, "DEFER", state2_enter_exec, "wlan_mac_lpw [DEFER enter execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw [DEFER enter execs], state2_enter_exec)
				{
				/** This state defer until the medium is available for transmission **/
				/** Interrupts that can occur in this state are:   					**/
				/** 1. Data arrival from application layer         					**/
				/** 2. Frame (DATA,ACK,RTS,CTS) rcvd from PHY layer					**/
				/** 3. Busy intrpt stating that frame is being rcvd					**/
				/** 4. Collision intrpt stating that more than one frame is rcvd    **/
				/** 5. Deference timer has expired (self intrpt)                    **/
				/** For Data arrival from application layer queue the packet. 		**/
				/** Set Backoff flag if the station needs to backoff				**/
				/** after deference because the medium is busy 						**/
				/** If the frame is destined for this station then set  			**/
				/** frame to respond and set a deference timer to SIFS. 			**/
				/** Set deference timer to SIFS and don't change states				**/
				/** If rcvr start receiving more than one frame then flag the		**/
				/** rcvd frame as invalid frame and set a deference to EIFS.		**/
				
				if (wlan_trace_active)
					{
					/* Determine the current state name.	*/
					strcpy (current_state_name, "defer");
					}
				}

				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [DEFER enter execs], state2_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (5,"wlan_mac_lpw")


			/** state (DEFER) exit executives **/
			FSM_STATE_EXIT_UNFORCED (2, "DEFER", "wlan_mac_lpw [DEFER exit execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw [DEFER exit execs], state2_exit_exec)
				{
				/* Call the interrupt processing routine for each interrupt	*/
				wlan_interrupts_process ();
				
				/* If the receiver is busy while the station is deferring		*/
				/* then clear the self interrupt. As there will be a new self	*/ 
				/* interrupt generated once the receiver becomes idle again.  	*/
				if (RECEIVER_BUSY_HIGH  && (op_ev_valid (deference_evh) == OPC_TRUE))
					{
					op_ev_cancel (deference_evh);
					}
				
				/* If the receiver becomes idle again schedule the end of the	*/
				/* deference.													*/
				if (RECEIVER_BUSY_LOW)
					{
					wlan_schedule_deference ();
					}
				
				/* While we were deferring, if we receive a frame which			*/
				/* requires a response, then we need to re-schedule our end of	*/
				/* deference interrupt, since the deference time for response	*/
				/* frames is shorter. Similarly, we need to re-schedule it if	*/
				/* the received frame made us set our NAV to a higher value.	*/
				else if (FRAME_RCVD && (fresp_to_send != WlanC_None || wlan_flags->nav_updated == OPC_BOOLINT_ENABLED) && 
					(op_ev_valid (deference_evh) == OPC_TRUE))
					{
					/* Cancel the current event and schedule a new one.			*/
					op_ev_cancel (deference_evh);
					wlan_schedule_deference ();
					}
				}
				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [DEFER exit execs], state2_exit_exec)


			/** state (DEFER) transition processing **/
			FSM_PROFILE_SECTION_IN (wlan_mac_lpw [DEFER trans conditions], state2_trans_conds)
			FSM_INIT_COND (DEFERENCE_OFF)
			FSM_DFLT_COND
			FSM_TEST_LOGIC ("DEFER")
			FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [DEFER trans conditions], state2_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 3, state3_enter_exec, ;, "DEFERENCE_OFF", "", "DEFER", "BKOFF_NEEDED")
				FSM_CASE_TRANSIT (1, 2, state2_enter_exec, ;, "default", "", "DEFER", "DEFER")
				}
				/*---------------------------------------------------------*/



			/** state (BKOFF_NEEDED) enter executives **/
			FSM_STATE_ENTER_FORCED (3, "BKOFF_NEEDED", state3_enter_exec, "wlan_mac_lpw [BKOFF_NEEDED enter execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw [BKOFF_NEEDED enter execs], state3_enter_exec)
				{
				/** Determining wether to backoff. It is needed when station preparing  **/
				/** to transmit frame discovers that the medium is busy or when the     **/
				/** the station infers collision.								        **/
				/** Backoff is not needed when the station is responding to the frame.	**/
				/** If backoff needed then check wether the station completed its 		**/
				/** backoff in the last attempt. If not then resume the backoff 		**/
				/** from the same point, otherwise generate a new random number 	 	**/
				/** for the number of backoff slots. 									**/
				
				/* Checking wether backoff is needed or not	*/ 
				if (wlan_flags->backoff_flag == OPC_BOOLINT_ENABLED)
					{
					if (backoff_slots == 0)
						{                                                             
						/* Compute backoff interval using binary exponential process	*/
						if (retry_count != 0)
							{			 
							/* Set the maximum backoff for the uniform distribution	*/  
							max_backoff = max_backoff * 2 + 1; 				
							}
						else
							{
							/* if retry count is set to 0 then set the	*/
							/* maximum backoff slots to min window size	*/	
							max_backoff = cw_min;
							}
				
						/* The number of possible slots grows exponentially	*/ 
						/* until it exceeds a fixed limit.					*/
						if (max_backoff > cw_max) 
							{
							max_backoff = cw_max;
							}
				 
						/* Obtain a uniformly distributed random integer between 0 and the minimum contention window size	*/
						/* Scale the number of slots according to the number of retransmissions.							*/
						backoff_slots = floor (op_dist_uniform (max_backoff + 1));
						}
				
					/* Set a timer for the end of the backoff interval.				 	 */
					intrpt_time = (current_time + backoff_slots * slot_time);
				
					/* Scheduling self interrupt for backoff */	
					backoff_elapsed_evh = op_intrpt_schedule_self (intrpt_time, WlanC_Backoff_Elapsed);
					
					/* Reporting number of backoff slots as a statistic */
					op_stat_write (backoff_slots_handle, backoff_slots);
					op_stat_write (backoff_slots_handle, 0.0);
					} 
				}

				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [BKOFF_NEEDED enter execs], state3_enter_exec)

			/** state (BKOFF_NEEDED) exit executives **/
			FSM_STATE_EXIT_FORCED (3, "BKOFF_NEEDED", "wlan_mac_lpw [BKOFF_NEEDED exit execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw [BKOFF_NEEDED exit execs], state3_exit_exec)
				{
				
				}
				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [BKOFF_NEEDED exit execs], state3_exit_exec)


			/** state (BKOFF_NEEDED) transition processing **/
			FSM_PROFILE_SECTION_IN (wlan_mac_lpw [BKOFF_NEEDED trans conditions], state3_trans_conds)
			FSM_INIT_COND (TRANSMIT_FRAME)
			FSM_TEST_COND (PERFORM_BACKOFF)
			FSM_TEST_LOGIC ("BKOFF_NEEDED")
			FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [BKOFF_NEEDED trans conditions], state3_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 4, state4_enter_exec, ;, "TRANSMIT_FRAME", "", "BKOFF_NEEDED", "TRANSMIT")
				FSM_CASE_TRANSIT (1, 5, state5_enter_exec, ;, "PERFORM_BACKOFF", "", "BKOFF_NEEDED", "BACKOFF")
				}
				/*---------------------------------------------------------*/



			/** state (TRANSMIT) enter executives **/
			FSM_STATE_ENTER_UNFORCED (4, "TRANSMIT", state4_enter_exec, "wlan_mac_lpw [TRANSMIT enter execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw [TRANSMIT enter execs], state4_enter_exec)
				{
				/** In this state following intrpts can occur:     				**/
				/** 1. Data arrival from application layer.			        	**/
				/** 2. Frame (DATA,ACK,RTS,CTS) rcvd from PHY layer.			**/
				/** 3. Busy intrpt stating that frame is being rcvd.			**/
				/** 4. Collision intrpt means more than one frame is rcvd.  	**/
				/** 5. Transmission completed intrpt from physical layer		**/
				/** Queue the packe for Data Arrival from the higher layer,		**/
				/** and do not change state.									**/
				/** After Transmission is completed change state to FRM_END		**/
				/** No response is generated for any lower layer packet arrival	**/
				
				/* Prepare transmission frame by setting appropriate	*/
				/* fields in the control/data frame.					*/ 
				/* Skip this routine if any frame is received from the	*/
				/* higher or lower layer(s)							  	*/
				if (wlan_flags->immediate_xmt == OPC_TRUE)
					{
					wlan_frame_transmit ();
					wlan_flags->immediate_xmt = OPC_FALSE;
					}
				
				else if (wlan_flags->rcvd_bad_packet == OPC_BOOLINT_DISABLED &&
				    intrpt_type == OPC_INTRPT_SELF)
					{
					wlan_frame_transmit ();
					}
				
				if (wlan_trace_active)
					{
					/* Determine the current state name	*/
					strcpy (current_state_name, "transmit");
					}
				}

				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [TRANSMIT enter execs], state4_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (9,"wlan_mac_lpw")


			/** state (TRANSMIT) exit executives **/
			FSM_STATE_EXIT_UNFORCED (4, "TRANSMIT", "wlan_mac_lpw [TRANSMIT exit execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw [TRANSMIT exit execs], state4_exit_exec)
				{
				/* If the packet is received while the the station is      */ 
				/* transmitting then mark the received packet as bad.	   */
				if (op_intrpt_type () == OPC_INTRPT_STAT)
					{
					if (op_intrpt_stat () == RECEIVER_BUSY_INSTAT && op_stat_local_read (RECEIVER_BUSY_INSTAT) == 1.0)
						{	
						wlan_flags->rcvd_bad_packet = OPC_BOOLINT_ENABLED;
						}
					
					/* If we completed the transmission then reset the		*/
					/* transmitter flag.									*/
					else if (op_intrpt_stat () == TRANSMITTER_INSTAT)
						{
						wlan_flags->transmitter_busy = OPC_BOOLINT_DISABLED;
						
						/* Also reset the receiver idle time, since with	*/
						/* the end of our transmission, the medium became	*/
						/* idle again.										*/
						rcv_idle_time = op_sim_time ();
						}
					}
				
				else if ((op_intrpt_type () == OPC_INTRPT_STRM) && (op_intrpt_strm () != instrm_from_mac_if))
					{
					/* While transmitting, we received a packet from		*/
					/* physical layer. Mark the packet as bad.				*/
					wlan_flags->rcvd_bad_packet = OPC_BOOLINT_ENABLED;
					}
				
				/* Call the interrupt processing routine for each interrupt.*/
				wlan_interrupts_process ();
				}
				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [TRANSMIT exit execs], state4_exit_exec)


			/** state (TRANSMIT) transition processing **/
			FSM_PROFILE_SECTION_IN (wlan_mac_lpw [TRANSMIT trans conditions], state4_trans_conds)
			FSM_INIT_COND (TRANSMISSION_COMPLETE)
			FSM_DFLT_COND
			FSM_TEST_LOGIC ("TRANSMIT")
			FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [TRANSMIT trans conditions], state4_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 6, state6_enter_exec, ;, "TRANSMISSION_COMPLETE", "", "TRANSMIT", "FRM_END")
				FSM_CASE_TRANSIT (1, 4, state4_enter_exec, ;, "default", "", "TRANSMIT", "TRANSMIT")
				}
				/*---------------------------------------------------------*/



			/** state (BACKOFF) enter executives **/
			FSM_STATE_ENTER_UNFORCED (5, "BACKOFF", state5_enter_exec, "wlan_mac_lpw [BACKOFF enter execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw [BACKOFF enter execs], state5_enter_exec)
				{
				/** Processing Random Backoff								**/
				/** In this state following intrpts can occur: 				**/
				/** 1. Data arrival from application layer   				**/
				/** 2. Frame (DATA,ACK,RTS,CTS) rcvd from PHY layer			**/
				/** 3. Busy intrpt stating that frame is being rcvd			**/
				/** 4. Coll intrpt stating that more than one frame is rcvd	**/  
				/** Queue the packet for Data Arrival from application 		**/
				/** layer and do not change the state.						**/ 
				/** If the frame is destined for this station then prepare  **/
				/** appropriate frame to respond and set deference to SIFS	**/
				/** Update NAV value (if needed) and reschedule deference	**/
				/** Change state to "DEFER"									**/
				/** If it's a broadcast frame then check wether NAV needs 	**/
				/** to be updated. Schedule self interrupt and change		**/
				/** state to Deference										**/
				/** If rcvr start receiving frame (busy stat intrpt) then 	**/
				/** set a flag indicating rcvr is busy. 					**/ 
				/** if rcvr start receiving more than one frame then flag 	**/ 
				/** the rcvd frame as invalid and set deference				**/
				/** timer to EIFS   										**/ 
				/* Change State to DEFER									**/
				
				if (wlan_trace_active)
					{
					/* Determine the current state name.	*/
					strcpy (current_state_name, "backoff");
					}
				}

				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [BACKOFF enter execs], state5_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (11,"wlan_mac_lpw")


			/** state (BACKOFF) exit executives **/
			FSM_STATE_EXIT_UNFORCED (5, "BACKOFF", "wlan_mac_lpw [BACKOFF exit execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw [BACKOFF exit execs], state5_exit_exec)
				{
				/* Call the interrupt processing routine for each interrupt	*/
				wlan_interrupts_process ();
				
				/* Set the number of slots to zero, once the backoff is completed	*/
				if (BACKOFF_COMPLETED)	
					{
					backoff_slots = 0.0;
					}
				
				/* Storing remaining backoff slots if the frame is rcvd from the remote station*/
				if (RECEIVER_BUSY_HIGH) 
					{
					/* Computing remaining backoff slots for next iteration */
					backoff_slots =  ceil ((intrpt_time - current_time) / slot_time);
				
					if (op_ev_valid (backoff_elapsed_evh) == OPC_TRUE)
						{
						/* clear the self interrupt as station needs to defer */
						op_ev_cancel (backoff_elapsed_evh);
						}
					}
				
				/* Schedule deference if the frame is received while the station is backing off.*/
				if (FRAME_RCVD)
					{
					
					/* Disable the self intrpt for backoff if the frame is rcvd while backing off	*/
					if (op_ev_valid (backoff_elapsed_evh) == OPC_TRUE)
						{
						/* Computing remaining backoff slots for next iteration */
						backoff_slots =  ceil ((intrpt_time - current_time) / slot_time);
				
						/* clear the self interrupt as station needs to defer */
						op_ev_cancel (backoff_elapsed_evh);
						}
				
					wlan_schedule_deference ();
					}		
				
				}
				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [BACKOFF exit execs], state5_exit_exec)


			/** state (BACKOFF) transition processing **/
			FSM_PROFILE_SECTION_IN (wlan_mac_lpw [BACKOFF trans conditions], state5_trans_conds)
			FSM_INIT_COND (BACKOFF_COMPLETED)
			FSM_TEST_COND (FRAME_RCVD)
			FSM_DFLT_COND
			FSM_TEST_LOGIC ("BACKOFF")
			FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [BACKOFF trans conditions], state5_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 4, state4_enter_exec, ;, "BACKOFF_COMPLETED", "", "BACKOFF", "TRANSMIT")
				FSM_CASE_TRANSIT (1, 2, state2_enter_exec, ;, "FRAME_RCVD", "", "BACKOFF", "DEFER")
				FSM_CASE_TRANSIT (2, 5, state5_enter_exec, ;, "default", "", "BACKOFF", "BACKOFF")
				}
				/*---------------------------------------------------------*/



			/** state (FRM_END) enter executives **/
			FSM_STATE_ENTER_FORCED (6, "FRM_END", state6_enter_exec, "wlan_mac_lpw [FRM_END enter execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw [FRM_END enter execs], state6_enter_exec)
				{
				/** The purpose of this state is to determine the next unforced	**/
				/** state after completing transmission.					    **/
				 
				/** 3 cases											     		**/
				/** 1.If just transmitted RTS or DATA frame then wait for 	 	**/
				/** response with expected_frame_type variable set and change   **/
				/** the states to Wait for Response otherwise just DEFER for 	**/
				/** next transmission											**/
				/** 2.If expected frame is rcvd then check to see what is the	**/
				/** next frame to transmit and set appropriate deference timer	**/
				/** 2a.If all the data fragments are transmitted then check		**/
				/** wether the queue is empty or not                            **/
				/** If not then based on threshold fragment the packet 	 		**/
				/** and based on threshold decide wether to send RTS or not   	**/
				/** If there is a data to be transmitted then wait for DIFS		**/
				/**	duration before contending for the channel					**/
				/** If nothing to transmit then go to IDLE state          		**/
				/** and wait for the packet arrival from higher or lower layer	**/
				/** 3.If expected frame is not rcvd then infer collision, 		**/
				/** set backoff flag, if retry limit is not reached       		**/
				/** retransmit the frame by contending for the channel  	 	**/
				
				/* If there is no frame expected then check to see if there  		*/
				/* is any other frame to transmit. Also mark the channel as idle	*/
				if (expected_frame_type == WlanC_None)
					{
					/* If the frame needs to be retransmitted or there is   	*/
					/* something in the fragmentation buffer to transmit or the	*/
					/* station needs to respond to a frame then schedule		*/
					/* deference.												*/
					if (op_sar_buf_size (fragmentation_buffer_ptr) != 0 || retry_count != 0 || fresp_to_send != WlanC_None)
						{
						/* Schedule deference before frame transmission	*/
						wlan_schedule_deference ();
						}
					
					/* After completing a successful frame transmission, even	*/
					/* though we don't have any other frame to transmit, still	*/
					/* we need to execute to backoff algorithm to generate a	*/
					/* contention window period and back-off during that period	*/
					/* as stated in the protocol.								*/
					else if (wlan_flags->cw_required == OPC_TRUE)
						{
						/* Determine the size of the contentions window.		*/
						cw_slots = floor (op_dist_uniform (cw_min + 1));
						cw_end = current_time + difs_time +  cw_slots * slot_time;
				
						/* Schedule a self interrupt indicating the end of the	*/
						/* contention window.									*/
						cw_end_evh = op_intrpt_schedule_self (cw_end, WlanC_CW_Elapsed);
						
						/* Update the backoff time statistic.					*/
						op_stat_write (backoff_slots_handle, cw_slots);
						op_stat_write (backoff_slots_handle, 0.0);
						
						/* Reset the flag since we scheduled the period.		*/
						wlan_flags->cw_required = OPC_FALSE;
						}
					
					else if (cw_end > current_time)
						{
						/* We are in the contention window period, but we had	*/
						/* to leave the "idle" state to send a response (Cts,	*/
						/* Ack) for a frame we received. Now we are moving back	*/
						/* to idle state. Hence, re-schedule the self interrupt	*/
						/* that will indicate the end of the contention window.	*/
						cw_end_evh = op_intrpt_schedule_self (cw_end, WlanC_CW_Elapsed);
						}
					
					else
						{
						/* Schedule the deference if we have a frame in the		*/
						/* buffer sent from higher layer for transmission,		*/
						/* since the contention window period is over.			*/
						if (op_prg_list_size (hld_list_ptr) != 0)
							{
							/* Schedule deference before frame transmission	*/
							wlan_schedule_deference ();
							}
						
				   		/* Reset the end of the CW timer, since it is over.		*/
						cw_end = 0.0;
						}
					}
				else
					{
					/* The station needs to wait for the expected frame type	*/
					/* So it will set the frame timeout interrupt which will be	*/
				    /* exectued if no frame is received in the set duration.   	*/
					timer_duration =  WLAN_ACK_DURATION + sifs_time + WLAN_AIR_PROPAGATION_TIME;
					frame_timeout_evh = op_intrpt_schedule_self (current_time + timer_duration, WlanC_Frame_Timeout);
					}
				}

				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [FRM_END enter execs], state6_enter_exec)

			/** state (FRM_END) exit executives **/
			FSM_STATE_EXIT_FORCED (6, "FRM_END", "wlan_mac_lpw [FRM_END exit execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw [FRM_END exit execs], state6_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [FRM_END exit execs], state6_exit_exec)


			/** state (FRM_END) transition processing **/
			FSM_PROFILE_SECTION_IN (wlan_mac_lpw [FRM_END trans conditions], state6_trans_conds)
			FSM_INIT_COND (!FRAME_TO_TRANSMIT && !EXPECTING_FRAME)
			FSM_TEST_COND (WAIT_FOR_FRAME)
			FSM_TEST_COND (FRAME_TO_TRANSMIT && !EXPECTING_FRAME)
			FSM_TEST_LOGIC ("FRM_END")
			FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [FRM_END trans conditions], state6_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 1, state1_enter_exec, ;, "!FRAME_TO_TRANSMIT && !EXPECTING_FRAME", "", "FRM_END", "IDLE")
				FSM_CASE_TRANSIT (1, 7, state7_enter_exec, ;, "WAIT_FOR_FRAME", "", "FRM_END", "WAIT_FOR_RESPONSE")
				FSM_CASE_TRANSIT (2, 2, state2_enter_exec, ;, "FRAME_TO_TRANSMIT && !EXPECTING_FRAME", "", "FRM_END", "DEFER")
				}
				/*---------------------------------------------------------*/



			/** state (WAIT_FOR_RESPONSE) enter executives **/
			FSM_STATE_ENTER_UNFORCED (7, "WAIT_FOR_RESPONSE", state7_enter_exec, "wlan_mac_lpw [WAIT_FOR_RESPONSE enter execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw [WAIT_FOR_RESPONSE enter execs], state7_enter_exec)
				{
				/** The purpose of this state is to wait for the response after	**/
				/** transmission. The only frames which require acknowlegements	**/
				/**  are RTS and DATA frame. 									**/
				/** In this state following intrpts can occur:				   	**/
				/** 1. Data arrival from application layer   					**/
				/** 2. Frame (DATA,ACK,RTS,CTS) rcvd from PHY layer				**/
				/** 3. Frame timeout if expected frame is not rcvd 				**/
				/** 4. Busy intrpt stating that frame is being rcvd           	**/
				/** 5. Collision intrpt stating that more than one frame is rcvd**/		
				/** Queue the packet as Data Arrives from application layer		**/
				/** If Rcvd unexpected frame then collision is inferred and		**/
				/** retry count is incremented							    	**/
				/** if a collision stat interrupt from the rcvr then flag the   **/
				/** received frame as bad 										**/
				
				if (wlan_trace_active)
					{
					/* Determine the current state name.	*/
					strcpy (current_state_name, "wait_for_response");
					}
				}

				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [WAIT_FOR_RESPONSE enter execs], state7_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (15,"wlan_mac_lpw")


			/** state (WAIT_FOR_RESPONSE) exit executives **/
			FSM_STATE_EXIT_UNFORCED (7, "WAIT_FOR_RESPONSE", "wlan_mac_lpw [WAIT_FOR_RESPONSE exit execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw [WAIT_FOR_RESPONSE exit execs], state7_exit_exec)
				{
				/* Clear the frame timeout interrupt once the receiver is busy	*/
				/* or the frame is received (in case of collisions, the			*/
				/* frames whose reception has started while we were				*/
				/* transmitting are excluded in the FRAME_RCVD macro).			*/
				intrpt_type = op_intrpt_type ();	
				if (((intrpt_type == OPC_INTRPT_STAT && op_intrpt_stat () == RECEIVER_BUSY_INSTAT && 
					  op_stat_local_read (RECEIVER_BUSY_INSTAT) == 1.0) || FRAME_RCVD) &&
					(op_ev_valid (frame_timeout_evh) == OPC_TRUE))
					{
					op_ev_cancel (frame_timeout_evh);
					}
				
				/* Call the interrupt processing routine for each interrupt	*/
				/* request.													*/
				wlan_interrupts_process ();
				
				/* If expected frame is not received in the set 	*/
				/* duration or the there is a collision at the		*/
				/* receiver then set the expected frame type to 	*/
				/* be none because the station needs to retransmit	*/
				/* the frame.									 	*/
				if (FRAME_TIMEOUT)
					{
					/* Setting expected frame type to none frame */
					expected_frame_type = WlanC_None;
					
					/* retransmission counter will be incremented */
					retry_count = retry_count + 1;
				
					/* Reset the NAV duration so that the				*/
					/* retransmission is not unnecessarily delayed.		*/
					nav_duration = current_time;
				
					/* Check whether further retries are possible or	*/
					/* the data frame needs to be discarded.			*/
					wlan_frame_discard ();
					}
				}
				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [WAIT_FOR_RESPONSE exit execs], state7_exit_exec)


			/** state (WAIT_FOR_RESPONSE) transition processing **/
			FSM_PROFILE_SECTION_IN (wlan_mac_lpw [WAIT_FOR_RESPONSE trans conditions], state7_trans_conds)
			FSM_INIT_COND (FRAME_TIMEOUT || FRAME_RCVD)
			FSM_DFLT_COND
			FSM_TEST_LOGIC ("WAIT_FOR_RESPONSE")
			FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [WAIT_FOR_RESPONSE trans conditions], state7_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 6, state6_enter_exec, ;, "FRAME_TIMEOUT || FRAME_RCVD", "", "WAIT_FOR_RESPONSE", "FRM_END")
				FSM_CASE_TRANSIT (1, 7, state7_enter_exec, ;, "default", "", "WAIT_FOR_RESPONSE", "WAIT_FOR_RESPONSE")
				}
				/*---------------------------------------------------------*/



			/** state (BSS_INIT) enter executives **/
			FSM_STATE_ENTER_UNFORCED (8, "BSS_INIT", state8_enter_exec, "wlan_mac_lpw [BSS_INIT enter execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw [BSS_INIT enter execs], state8_enter_exec)
				{
				/* Schedule a self interrupt to wait for mac interface 	*/
				/* to move to next state after registering				*/
				op_intrpt_schedule_self (op_sim_time (), 0);
				}

				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [BSS_INIT enter execs], state8_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (17,"wlan_mac_lpw")


			/** state (BSS_INIT) exit executives **/
			FSM_STATE_EXIT_UNFORCED (8, "BSS_INIT", "wlan_mac_lpw [BSS_INIT exit execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw [BSS_INIT exit execs], state8_exit_exec)
				{
				/* object id of the surrounding processor	*/
				my_objid = op_id_self ();
				 
				/* Obtain the node's object identifier	*/
				my_node_objid = op_topo_parent (my_objid);
				my_subnet_objid = op_topo_parent (my_node_objid);
				
				/* Obtain the values assigned to the various attributes	*/
				op_ima_obj_attr_get (my_objid, "Wireless LAN Parameters", &wlan_params_comp_attr_objid);
				params_attr_objid = op_topo_child (wlan_params_comp_attr_objid, OPC_OBJTYPE_GENERIC, 0);
				
				/* Determining the final MAC address after address resolution.	*/
				op_ima_obj_attr_get (my_objid, "station_address", &my_address);
				
				/* Once the station addresses are resolved, then create a pool for wlan addresses.	*/
				oms_aa_address_resolve (oms_aa_wlan_handle, my_objid, &my_address);
				
				/* Obtain the MAC layer information for the local MAC	*/
				/* process from the model-wide registry.				*/
				proc_record_handle_list_ptr = op_prg_list_create ();
				
				oms_pr_process_discover (OPC_OBJID_INVALID, proc_record_handle_list_ptr, 
					"subnetid",				OMSC_PR_OBJID,			 my_subnet_objid,
					"mac_type",				OMSC_PR_STRING,			"wireless_lan",
					"protocol",				OMSC_PR_STRING,			"mac",
				 	 OPC_NIL);
				
				/* If the MAC interface process registered itself,	*/
				/* then there must be a valid match					*/
				record_handle_list_size = op_prg_list_size (proc_record_handle_list_ptr);
				
				/* Allocating memory for the duplicate buffer based on number of stations in the subnet.	*/
				duplicate_list_ptr = (WlanT_Mac_Duplicate_Buffer_Entry**) 
									 op_prg_mem_alloc (record_handle_list_size * sizeof (WlanT_Mac_Duplicate_Buffer_Entry*));
				
				/* Initializing duplicate buffer entries.	*/
				for (i = 0; i <= (record_handle_list_size - 1); i++)
					{
					duplicate_list_ptr [i] = OPC_NIL;
					}
				
				/* Initialize the address list index to zero.	*/
				addr_index = 0; 
				
				/* Variable to counting number of access point in the network.	*/
				ap_count = 0;
				
				/* Maintain a list of stations in the BSS if it is an AP and a bridge	*/
				if (ap_flag == OPC_BOOLINT_ENABLED && wlan_flags->bridge_flag == OPC_BOOLINT_ENABLED)
					{
					bss_stn_list = op_prg_mem_alloc ((record_handle_list_size - 1) * sizeof (int));
					count = 0;
					
					/* Number of stations in the BSS	*/
					bss_stn_count = record_handle_list_size - 1;	  
					}
				
				/* Traversing the process record handle list to determine if there is any access point in the subnet.	*/
				for (i = 0; i < record_handle_list_size; i++ )
					{
					/*	Obtain a handle on the process record	*/
					process_record_handle = (OmsT_Pr_Handle) op_prg_list_access (proc_record_handle_list_ptr, i);
				
					/* Get the Station type.	*/
					oms_pr_attr_get (process_record_handle, "subprotocol", OMSC_PR_NUMBER, &statype);
				
					/* If the station is an Access Point then its station id will be a BSS id for all the station in that subnet.	*/
					if (statype == (double) WLAN_AP)
						{
						/* If access point found then it means that it is a Infrastructured BSS.	*/
						bss_flag = OPC_BOOLINT_ENABLED;
					
						/* Get the BSS ID.	*/
						oms_pr_attr_get (process_record_handle, "address",	OMSC_PR_NUMBER, &sta_addr);	 
						bss_id  = (int) sta_addr;
				
						/* According to IEEE802.11 there cannot be more than one Access point in the same subnet.	*/
						ap_count = ap_count + 1;
						if (ap_count == 2)
							{
							wlan_mac_error ("More than one Access Point found.", "Check the configuration.", OPC_NIL);
							}
						}
					
					/* If the station is a bridge and an access point then	*/
					/* maintain a list of stations in the BSS				*/
					if (ap_flag == OPC_BOOLINT_ENABLED && wlan_flags->bridge_flag == OPC_BOOLINT_ENABLED)
						{
						/* Get the station id	*/
						oms_pr_attr_get (process_record_handle, "address",	OMSC_PR_NUMBER, &sta_addr);
				
						/* Maintain a list of stations in the BSS not including itself	*/
						if ((int) sta_addr != my_address)
							{
							bss_stn_list [count] = (int) sta_addr;
							count = count + 1;
							}
						}
					
					/* Checking the physical characteristic configuration for the subnet.	*/
					oms_pr_attr_get (process_record_handle, "module objid", OMSC_PR_OBJID, &my_objid);
				
					/* Obtain the values assigned to the various attributes	*/
					op_ima_obj_attr_get (my_objid, "Wireless LAN Parameters", &wlan_params_comp_attr_objid);
					params_attr_objid = op_topo_child (wlan_params_comp_attr_objid, OPC_OBJTYPE_GENERIC, 0);
				
					/* Load the appropriate physical layer characteristics.	*/	
					op_ima_obj_attr_get (params_attr_objid, "Physical Characteristics", &sta_phy_char_flag);
				
					if (sta_phy_char_flag != phy_char_flag)
						{
						wlan_mac_error ("Physical Characteristic configuration mismatch in the subnet.",
										"All stations in the subnet should have same physical characteristics", "Check the configuration");			
						}				
				    }
				
				/* Deallocate memory used for process discovery	*/
				while (op_prg_list_size (proc_record_handle_list_ptr))
					{
					op_prg_list_remove (proc_record_handle_list_ptr, OPC_LISTPOS_HEAD);
					}
				op_prg_mem_free (proc_record_handle_list_ptr);
				
				/* Obtain the MAC layer information for the local MAC	*/
				/* process from the model-wide registry.				*/
				/* This is to check if the node is a gateway or not.	*/
				proc_record_handle_list_ptr = op_prg_list_create ();
				
				oms_pr_process_discover (OPC_OBJID_INVALID, proc_record_handle_list_ptr, 
					"node objid",					OMSC_PR_OBJID,			 my_node_objid,
					"gateway node",					OMSC_PR_STRING,			"gateway",
				 	 OPC_NIL);
				
				/* If the MAC interface process registered itself,	*/
				/* then there must be a valid match					*/
				record_handle_list_size = op_prg_list_size (proc_record_handle_list_ptr);
				
				if (record_handle_list_size != 0)
					{
					wlan_flags->gateway_flag = OPC_BOOLINT_ENABLED;
					}
				
				/* Deallocate memory used for process discovery	*/
				while (op_prg_list_size (proc_record_handle_list_ptr))
					{
					op_prg_list_remove (proc_record_handle_list_ptr, OPC_LISTPOS_HEAD);
					}
				op_prg_mem_free (proc_record_handle_list_ptr);
				
				}
				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw [BSS_INIT exit execs], state8_exit_exec)


			/** state (BSS_INIT) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "BSS_INIT", "IDLE")
				/*---------------------------------------------------------*/



			}


		FSM_EXIT (0,"wlan_mac_lpw")
		}
	}




void
wlan_mac_lpw_diag (OP_SIM_CONTEXT_ARG_OPT)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = __LINE__;
#endif

	FIN_MT (wlan_mac_lpw_diag ())

	if (1)
		{
		/* variables used for registering and discovering process models */
		OmsT_Pr_Handle			process_record_handle;
		List*					proc_record_handle_list_ptr;
		int						record_handle_list_size;
		int						ap_count;
		int						count;
		double					sta_addr;
		double					statype ;
		Objid					mac_if_module_objid;
		char					proc_model_name [300];
		Objid					my_subnet_objid;
		Objid					my_objid; 
		Objid					my_node_objid;
		Objid					params_attr_objid;
		Objid					wlan_params_comp_attr_objid;
		Objid					strm_objid;
		int						strm;
		int						i,j;
		int						addr_index;
		int						num_out_assoc;
		int						node_count;
		int						node_objid;
		WlanT_Hld_List_Elem*	hld_ptr;
		Prohandle				own_prohandle;
		double					timer_duration;
		double					cw_slots;
		char					msg1 [120];
		char					msg2 [120];
		WlanT_Phy_Char_Code		sta_phy_char_flag;

		/* Diagnostic Block */


		BINIT
		/* Print information about this process.	*/
		if (wlan_trace_active == OPC_TRUE)
			{
			printf ("Current state name:\t", current_state_name);
			}
		
		printf ("Station MAC Address:%d\n", my_address);
		printf ("printing the higher layer queue contents (packet ids)\n");
		for (i = 0; i < op_prg_list_size (hld_list_ptr); i++)
			{
			/* Remove packet from higher layer queue. */
			hld_ptr = (WlanT_Hld_List_Elem*) op_prg_list_access (hld_list_ptr, i);
			printf ("%d\t", op_pk_id (hld_ptr->pkptr));
			if ((i % 4) == 0)
				{
				printf ("\n");
				}
			} 

		/* End of Diagnostic Block */

		}

	FOUT
	}




void
wlan_mac_lpw_terminate (OP_SIM_CONTEXT_ARG_OPT)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = __LINE__;
#endif

	FIN_MT (wlan_mac_lpw_terminate ())

	if (1)
		{
		/* variables used for registering and discovering process models */
		OmsT_Pr_Handle			process_record_handle;
		List*					proc_record_handle_list_ptr;
		int						record_handle_list_size;
		int						ap_count;
		int						count;
		double					sta_addr;
		double					statype ;
		Objid					mac_if_module_objid;
		char					proc_model_name [300];
		Objid					my_subnet_objid;
		Objid					my_objid; 
		Objid					my_node_objid;
		Objid					params_attr_objid;
		Objid					wlan_params_comp_attr_objid;
		Objid					strm_objid;
		int						strm;
		int						i,j;
		int						addr_index;
		int						num_out_assoc;
		int						node_count;
		int						node_objid;
		WlanT_Hld_List_Elem*	hld_ptr;
		Prohandle				own_prohandle;
		double					timer_duration;
		double					cw_slots;
		char					msg1 [120];
		char					msg2 [120];
		WlanT_Phy_Char_Code		sta_phy_char_flag;

		/* Termination Block */


		BINIT
		{
		
		}

		/* End of Termination Block */

		}
	Vos_Poolmem_Dealloc_MT (OP_SIM_CONTEXT_THREAD_INDEX_COMMA pr_state_ptr);

	FOUT
	}


/* Undefine shortcuts to state variables to avoid */
/* syntax error in direct access to fields of */
/* local variable prs_ptr in wlan_mac_lpw_svar function. */
#undef retry_count
#undef intrpt_type
#undef intrpt_code
#undef my_address
#undef hld_list_ptr
#undef operational_speed
#undef frag_threshold
#undef packet_seq_number
#undef packet_frag_number
#undef destination_addr
#undef fragmentation_buffer_ptr
#undef fresp_to_send
#undef nav_duration
#undef rts_threshold
#undef duplicate_entry
#undef expected_frame_type
#undef remote_sta_addr
#undef backoff_slots
#undef packet_load_handle
#undef intrpt_time
#undef wlan_transmit_frame_copy_ptr
#undef backoff_slots_handle
#undef instrm_from_mac_if
#undef outstrm_to_mac_if
#undef num_fragments
#undef remainder_size
#undef defragmentation_list_ptr
#undef wlan_flags
#undef oms_aa_handle
#undef current_time
#undef rcv_idle_time
#undef cw_end
#undef duplicate_list_ptr
#undef hld_pmh
#undef max_backoff
#undef current_state_name
#undef hl_packets_rcvd
#undef media_access_delay
#undef ete_delay_handle
#undef global_ete_delay_handle
#undef global_throughput_handle
#undef global_load_handle
#undef global_dropped_data_handle
#undef global_mac_delay_handle
#undef ctrl_traffic_rcvd_handle_inbits
#undef ctrl_traffic_sent_handle_inbits
#undef ctrl_traffic_rcvd_handle
#undef ctrl_traffic_sent_handle
#undef data_traffic_rcvd_handle_inbits
#undef data_traffic_sent_handle_inbits
#undef data_traffic_rcvd_handle
#undef data_traffic_sent_handle
#undef sifs_time
#undef slot_time
#undef cw_min
#undef cw_max
#undef difs_time
#undef channel_reserv_handle
#undef retrans_handle
#undef throughput_handle
#undef long_retry_limit
#undef short_retry_limit
#undef retry_limit
#undef last_frametx_type
#undef deference_evh
#undef backoff_elapsed_evh
#undef frame_timeout_evh
#undef cw_end_evh
#undef eifs_time
#undef i_strm
#undef wlan_trace_active
#undef pkt_in_service
#undef bits_load_handle
#undef ap_flag
#undef bss_flag
#undef bss_id
#undef hld_max_size
#undef max_receive_lifetime
#undef phy_char_flag
#undef oms_aa_wlan_handle
#undef total_hlpk_size
#undef drop_packet_handle
#undef drop_packet_handle_inbits
#undef drop_pkt_log_handle
#undef drop_pkt_entry_log_flag
#undef packet_size
#undef receive_time
#undef llc_iciptr
#undef bss_stn_list
#undef bss_stn_count
#undef data_packet_type
#undef data_packet_dest
#undef data_packet_final_dest

#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE

#define FIN_PREAMBLE_DEC
#define FIN_PREAMBLE_CODE

VosT_Obtype
wlan_mac_lpw_init (int * init_block_ptr)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	VosT_Obtype obtype = OPC_NIL;
	FIN_MT (wlan_mac_lpw_init (init_block_ptr))

	Vos_Define_Object (&obtype, "proc state vars (wlan_mac_lpw)",
		sizeof (wlan_mac_lpw_state), 0, 20);
	*init_block_ptr = 0;

	FRET (obtype)
	}

VosT_Address
wlan_mac_lpw_alloc (VOS_THREAD_INDEX_ARG_COMMA VosT_Obtype obtype, int init_block)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	wlan_mac_lpw_state * ptr;
	FIN_MT (wlan_mac_lpw_alloc (obtype))

	ptr = (wlan_mac_lpw_state *)Vos_Alloc_Object_MT (VOS_THREAD_INDEX_COMMA obtype);
	if (ptr != OPC_NIL)
		ptr->_op_current_block = init_block;
	FRET ((VosT_Address)ptr)
	}



void
wlan_mac_lpw_svar (void * gen_ptr, const char * var_name, void ** var_p_ptr)
	{
	wlan_mac_lpw_state		*prs_ptr;

	FIN_MT (wlan_mac_lpw_svar (gen_ptr, var_name, var_p_ptr))

	if (var_name == OPC_NIL)
		{
		*var_p_ptr = (void *)OPC_NIL;
		FOUT
		}
	prs_ptr = (wlan_mac_lpw_state *)gen_ptr;

	if (strcmp ("retry_count" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->retry_count);
		FOUT
		}
	if (strcmp ("intrpt_type" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->intrpt_type);
		FOUT
		}
	if (strcmp ("intrpt_code" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->intrpt_code);
		FOUT
		}
	if (strcmp ("my_address" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_address);
		FOUT
		}
	if (strcmp ("hld_list_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->hld_list_ptr);
		FOUT
		}
	if (strcmp ("operational_speed" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->operational_speed);
		FOUT
		}
	if (strcmp ("frag_threshold" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->frag_threshold);
		FOUT
		}
	if (strcmp ("packet_seq_number" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->packet_seq_number);
		FOUT
		}
	if (strcmp ("packet_frag_number" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->packet_frag_number);
		FOUT
		}
	if (strcmp ("destination_addr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->destination_addr);
		FOUT
		}
	if (strcmp ("fragmentation_buffer_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->fragmentation_buffer_ptr);
		FOUT
		}
	if (strcmp ("fresp_to_send" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->fresp_to_send);
		FOUT
		}
	if (strcmp ("nav_duration" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->nav_duration);
		FOUT
		}
	if (strcmp ("rts_threshold" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->rts_threshold);
		FOUT
		}
	if (strcmp ("duplicate_entry" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->duplicate_entry);
		FOUT
		}
	if (strcmp ("expected_frame_type" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->expected_frame_type);
		FOUT
		}
	if (strcmp ("remote_sta_addr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->remote_sta_addr);
		FOUT
		}
	if (strcmp ("backoff_slots" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->backoff_slots);
		FOUT
		}
	if (strcmp ("packet_load_handle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->packet_load_handle);
		FOUT
		}
	if (strcmp ("intrpt_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->intrpt_time);
		FOUT
		}
	if (strcmp ("wlan_transmit_frame_copy_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->wlan_transmit_frame_copy_ptr);
		FOUT
		}
	if (strcmp ("backoff_slots_handle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->backoff_slots_handle);
		FOUT
		}
	if (strcmp ("instrm_from_mac_if" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->instrm_from_mac_if);
		FOUT
		}
	if (strcmp ("outstrm_to_mac_if" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->outstrm_to_mac_if);
		FOUT
		}
	if (strcmp ("num_fragments" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->num_fragments);
		FOUT
		}
	if (strcmp ("remainder_size" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->remainder_size);
		FOUT
		}
	if (strcmp ("defragmentation_list_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->defragmentation_list_ptr);
		FOUT
		}
	if (strcmp ("wlan_flags" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->wlan_flags);
		FOUT
		}
	if (strcmp ("oms_aa_handle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->oms_aa_handle);
		FOUT
		}
	if (strcmp ("current_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->current_time);
		FOUT
		}
	if (strcmp ("rcv_idle_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->rcv_idle_time);
		FOUT
		}
	if (strcmp ("cw_end" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->cw_end);
		FOUT
		}
	if (strcmp ("duplicate_list_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->duplicate_list_ptr);
		FOUT
		}
	if (strcmp ("hld_pmh" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->hld_pmh);
		FOUT
		}
	if (strcmp ("max_backoff" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->max_backoff);
		FOUT
		}
	if (strcmp ("current_state_name" , var_name) == 0)
		{
		*var_p_ptr = (void *) (prs_ptr->current_state_name);
		FOUT
		}
	if (strcmp ("hl_packets_rcvd" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->hl_packets_rcvd);
		FOUT
		}
	if (strcmp ("media_access_delay" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->media_access_delay);
		FOUT
		}
	if (strcmp ("ete_delay_handle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->ete_delay_handle);
		FOUT
		}
	if (strcmp ("global_ete_delay_handle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->global_ete_delay_handle);
		FOUT
		}
	if (strcmp ("global_throughput_handle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->global_throughput_handle);
		FOUT
		}
	if (strcmp ("global_load_handle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->global_load_handle);
		FOUT
		}
	if (strcmp ("global_dropped_data_handle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->global_dropped_data_handle);
		FOUT
		}
	if (strcmp ("global_mac_delay_handle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->global_mac_delay_handle);
		FOUT
		}
	if (strcmp ("ctrl_traffic_rcvd_handle_inbits" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->ctrl_traffic_rcvd_handle_inbits);
		FOUT
		}
	if (strcmp ("ctrl_traffic_sent_handle_inbits" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->ctrl_traffic_sent_handle_inbits);
		FOUT
		}
	if (strcmp ("ctrl_traffic_rcvd_handle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->ctrl_traffic_rcvd_handle);
		FOUT
		}
	if (strcmp ("ctrl_traffic_sent_handle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->ctrl_traffic_sent_handle);
		FOUT
		}
	if (strcmp ("data_traffic_rcvd_handle_inbits" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->data_traffic_rcvd_handle_inbits);
		FOUT
		}
	if (strcmp ("data_traffic_sent_handle_inbits" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->data_traffic_sent_handle_inbits);
		FOUT
		}
	if (strcmp ("data_traffic_rcvd_handle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->data_traffic_rcvd_handle);
		FOUT
		}
	if (strcmp ("data_traffic_sent_handle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->data_traffic_sent_handle);
		FOUT
		}
	if (strcmp ("sifs_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->sifs_time);
		FOUT
		}
	if (strcmp ("slot_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->slot_time);
		FOUT
		}
	if (strcmp ("cw_min" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->cw_min);
		FOUT
		}
	if (strcmp ("cw_max" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->cw_max);
		FOUT
		}
	if (strcmp ("difs_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->difs_time);
		FOUT
		}
	if (strcmp ("channel_reserv_handle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->channel_reserv_handle);
		FOUT
		}
	if (strcmp ("retrans_handle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->retrans_handle);
		FOUT
		}
	if (strcmp ("throughput_handle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->throughput_handle);
		FOUT
		}
	if (strcmp ("long_retry_limit" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->long_retry_limit);
		FOUT
		}
	if (strcmp ("short_retry_limit" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->short_retry_limit);
		FOUT
		}
	if (strcmp ("retry_limit" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->retry_limit);
		FOUT
		}
	if (strcmp ("last_frametx_type" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->last_frametx_type);
		FOUT
		}
	if (strcmp ("deference_evh" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->deference_evh);
		FOUT
		}
	if (strcmp ("backoff_elapsed_evh" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->backoff_elapsed_evh);
		FOUT
		}
	if (strcmp ("frame_timeout_evh" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->frame_timeout_evh);
		FOUT
		}
	if (strcmp ("cw_end_evh" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->cw_end_evh);
		FOUT
		}
	if (strcmp ("eifs_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->eifs_time);
		FOUT
		}
	if (strcmp ("i_strm" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->i_strm);
		FOUT
		}
	if (strcmp ("wlan_trace_active" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->wlan_trace_active);
		FOUT
		}
	if (strcmp ("pkt_in_service" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->pkt_in_service);
		FOUT
		}
	if (strcmp ("bits_load_handle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->bits_load_handle);
		FOUT
		}
	if (strcmp ("ap_flag" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->ap_flag);
		FOUT
		}
	if (strcmp ("bss_flag" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->bss_flag);
		FOUT
		}
	if (strcmp ("bss_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->bss_id);
		FOUT
		}
	if (strcmp ("hld_max_size" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->hld_max_size);
		FOUT
		}
	if (strcmp ("max_receive_lifetime" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->max_receive_lifetime);
		FOUT
		}
	if (strcmp ("phy_char_flag" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->phy_char_flag);
		FOUT
		}
	if (strcmp ("oms_aa_wlan_handle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->oms_aa_wlan_handle);
		FOUT
		}
	if (strcmp ("total_hlpk_size" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->total_hlpk_size);
		FOUT
		}
	if (strcmp ("drop_packet_handle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->drop_packet_handle);
		FOUT
		}
	if (strcmp ("drop_packet_handle_inbits" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->drop_packet_handle_inbits);
		FOUT
		}
	if (strcmp ("drop_pkt_log_handle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->drop_pkt_log_handle);
		FOUT
		}
	if (strcmp ("drop_pkt_entry_log_flag" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->drop_pkt_entry_log_flag);
		FOUT
		}
	if (strcmp ("packet_size" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->packet_size);
		FOUT
		}
	if (strcmp ("receive_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->receive_time);
		FOUT
		}
	if (strcmp ("llc_iciptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->llc_iciptr);
		FOUT
		}
	if (strcmp ("bss_stn_list" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->bss_stn_list);
		FOUT
		}
	if (strcmp ("bss_stn_count" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->bss_stn_count);
		FOUT
		}
	if (strcmp ("data_packet_type" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->data_packet_type);
		FOUT
		}
	if (strcmp ("data_packet_dest" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->data_packet_dest);
		FOUT
		}
	if (strcmp ("data_packet_final_dest" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->data_packet_final_dest);
		FOUT
		}
	*var_p_ptr = (void *)OPC_NIL;

	FOUT
	}

