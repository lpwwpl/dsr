/* Process model C form file: wlan_mac_lpw_interface.pr.c */
/* Portions of this file copyright 1992-2003 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
const char wlan_mac_lpw_interface_pr_c [] = "MIL_3_Tfile_Hdr_ 100A 30A op_runsim 7 49AD4723 49AD4723 1 lpw Administrator 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 8f3 1                                                                                                                                                                                                                                                                                                                                                                                                       ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>



/* Header Block */

/***** Include Files. *****/

/* Address assignment definitions.	*/
#include "oms_auto_addr_support.h"

/* Topology analysis-related definitions. */
#include "oms_tan.h"

/* Process registry-related definitions. */
#include "oms_pr.h"

/***** Transition Macros ******/
#define MAC_LAYER_PKT_ARVL	(intrpt_type == OPC_INTRPT_STRM && intrpt_strm == instrm_from_mac)
#define APPL_LAYER_PKT_ARVL	(intrpt_type == OPC_INTRPT_STRM && intrpt_strm != instrm_from_mac)

/***** Functional declaration ******/
static void			wlan_mac_higher_layer_intf_sv_init ();
static void			wlan_mac_higher_layer_register_as_arp ();


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
	int	                    		instrm_from_mac;
	int	                    		outstrm_to_mac;
	int	                    		destination_address;
	OmsT_Aa_Address_Handle	 		oms_aa_handle;
	int	                    		mac_address;
	Ici*	                   		wlan_mac_req_iciptr;
	} wlan_mac_lpw_interface_state;

#define pr_state_ptr            		((wlan_mac_lpw_interface_state*) (OP_SIM_CONTEXT_PTR->mod_state_ptr))
#define my_objid                		pr_state_ptr->my_objid
#define my_node_objid           		pr_state_ptr->my_node_objid
#define instrm_from_mac         		pr_state_ptr->instrm_from_mac
#define outstrm_to_mac          		pr_state_ptr->outstrm_to_mac
#define destination_address     		pr_state_ptr->destination_address
#define oms_aa_handle           		pr_state_ptr->oms_aa_handle
#define mac_address             		pr_state_ptr->mac_address
#define wlan_mac_req_iciptr     		pr_state_ptr->wlan_mac_req_iciptr

/* These macro definitions will define a local variable called	*/
/* "op_sv_ptr" in each function containing a FIN statement.	*/
/* This variable points to the state variable data structure,	*/
/* and can be used from a C debugger to display their values.	*/
#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE
#if defined (OPD_PARALLEL)
#  define FIN_PREAMBLE_DEC	wlan_mac_lpw_interface_state *op_sv_ptr; OpT_Sim_Context * tcontext_ptr;
#  define FIN_PREAMBLE_CODE	\
		if (VosS_Mt_Perform_Lock) \
			VOS_THREAD_SPECIFIC_DATA_GET (VosI_Globals.simi_mt_context_data_key, tcontext_ptr, SimT_Context *); \
		else \
			tcontext_ptr = VosI_Globals.simi_sequential_context_ptr; \
		op_sv_ptr = ((wlan_mac_lpw_interface_state *)(tcontext_ptr->mod_state_ptr));
#else
#  define FIN_PREAMBLE_DEC	wlan_mac_lpw_interface_state *op_sv_ptr;
#  define FIN_PREAMBLE_CODE	op_sv_ptr = pr_state_ptr;
#endif


/* Function Block */


#if !defined (VOSD_NO_FIN)
enum { _op_block_origin = __LINE__ };
#endif
static void
wlan_mac_higher_layer_intf_sv_init ()
	{
	int			type_of_service;

	/** Initializes all state variables used in this	**/
	/** process model.									**/
	FIN (wlan_mac_higher_layer_intf_sv_init ());

	/* Object identifier for the surrounding module and node.	*/
	my_objid = op_id_self ();
	my_node_objid = op_topo_parent (my_objid);

	/* Stream indices to and from the WLAN MAC process.	*/
	/* these will be set in the "exit execs" of "init".	*/
	outstrm_to_mac  = OPC_INT_UNDEF;
	instrm_from_mac = OPC_INT_UNDEF;

	/* Determine the destination to which packet should	*/
	/* be sent,and the prioritization to be provided to	*/
	/* the transmitted packet.							*/
	printf("init de dest address\n");
	op_ima_obj_attr_get (my_objid, "Destination Address", &destination_address);
	op_ima_obj_attr_get (my_objid, "Type of Service", 	  &type_of_service);

	/* Some interface control information is needed to	*/
	/* indicate to the MAC of the destination to which	*/
	/* a given packet needs to be sent. Create it.		*/
	wlan_mac_req_iciptr = op_ici_create ("wlan_mac_request");
	op_ici_attr_set (wlan_mac_req_iciptr, "type_of_service", type_of_service);
	op_ici_attr_set (wlan_mac_req_iciptr, "protocol_type",   0x800);

	FOUT;
	}

static void
wlan_mac_higher_layer_register_as_arp ()
	{
	char				proc_model_name [128];
	OmsT_Pr_Handle		own_process_record_handle;
	Prohandle			own_prohandle;

	/** Register this process in the model-wide process registry.	**/
	FIN (wlan_mac_higher_layer_register_as_arp ());

	/* Obtain the process model name and process handle.	*/
	op_ima_obj_attr_get (my_objid, "process model", proc_model_name);
	own_prohandle = op_pro_self ();

	/* Register this process in the model-wide process registry	*/
	own_process_record_handle = (OmsT_Pr_Handle) oms_pr_process_register (
			my_node_objid, my_objid, own_prohandle, proc_model_name);

	/* Register this protocol attribute and the element address	*/
	/* of this process into the model-wide registry.			*/
	oms_pr_attr_set (own_process_record_handle,
		"protocol",		OMSC_PR_STRING,		"arp",
		OPC_NIL);

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
	void wlan_mac_lpw_interface (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Obtype wlan_mac_lpw_interface_init (int * init_block_ptr);
	VosT_Address wlan_mac_lpw_interface_alloc (VOS_THREAD_INDEX_ARG_COMMA VosT_Obtype, int);
	void wlan_mac_lpw_interface_diag (OP_SIM_CONTEXT_ARG_OPT);
	void wlan_mac_lpw_interface_terminate (OP_SIM_CONTEXT_ARG_OPT);
	void wlan_mac_lpw_interface_svar (void *, const char *, void **);


	VosT_Fun_Status Vos_Define_Object (VosT_Obtype * obst_ptr, const char * name, unsigned int size, unsigned int init_obs, unsigned int inc_obs);
	VosT_Address Vos_Alloc_Object_MT (VOS_THREAD_INDEX_ARG_COMMA VosT_Obtype ob_hndl);
	VosT_Fun_Status Vos_Poolmem_Dealloc_MT (VOS_THREAD_INDEX_ARG_COMMA VosT_Address ob_ptr);
#if defined (__cplusplus)
} /* end of 'extern "C"' */
#endif




/* Process model interrupt handling procedure */


void
wlan_mac_lpw_interface (OP_SIM_CONTEXT_ARG_OPT)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	FIN_MT (wlan_mac_lpw_interface ());
	if (1)
		{
		List*				proc_record_handle_list_ptr;
		int					record_handle_list_size;
		OmsT_Pr_Handle		process_record_handle;
		Objid				mac_module_objid;
		Boolean				dest_addr_okay = OPC_FALSE;
		double				ne_address = OPC_DBL_UNDEF;
		int					curr_dest_addr = OMSC_AA_AUTO_ASSIGN;
		Packet*				pkptr;
		int					intrpt_type = OPC_INT_UNDEF;
		int					intrpt_strm = OPC_INT_UNDEF;
		int					i;
		OmsT_Aa_Address_Info * ith_address_info_ptr;
		
		Ici* ici_dest_address;


		FSM_ENTER ("wlan_mac_lpw_interface")

		FSM_BLOCK_SWITCH
			{
			/*---------------------------------------------------------*/
			/** state (init) enter executives **/
			FSM_STATE_ENTER_UNFORCED_NOLABEL (0, "init", "wlan_mac_lpw_interface [init enter execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw_interface [init enter execs], state0_enter_exec)
				{
				/* Initialize the state variables used by this model.	*/
				wlan_mac_higher_layer_intf_sv_init ();
				
				/* Register this process as "arp" so that lower layer	*/
				/* MAC process can connect to it.						*/
				wlan_mac_higher_layer_register_as_arp ();
				
				/* Schedule a self interrupt to wait for lower layer	*/
				/* wlan MAC process to initialize and register itself in	*/
				/* the model-wide process registry.						*/
				op_intrpt_schedule_self (op_sim_time (), 0);
				
				}

				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw_interface [init enter execs], state0_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (1,"wlan_mac_lpw_interface")


			/** state (init) exit executives **/
			FSM_STATE_EXIT_UNFORCED (0, "init", "wlan_mac_lpw_interface [init exit execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw_interface [init exit execs], state0_exit_exec)
				{
				/* Schedule a self interrupt to wait for lower layer	*/
				/* wlan MAC process to initialize and register itself in*/
				/* the model-wide process registry.						*/
				op_intrpt_schedule_self (op_sim_time (), 0);
				
				}
				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw_interface [init exit execs], state0_exit_exec)


			/** state (init) transition processing **/
			FSM_TRANSIT_FORCE (5, state5_enter_exec, ;, "default", "", "init", "init2")
				/*---------------------------------------------------------*/



			/** state (idle) enter executives **/
			FSM_STATE_ENTER_UNFORCED (1, "idle", state1_enter_exec, "wlan_mac_lpw_interface [idle enter execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw_interface [idle enter execs], state1_enter_exec)
				{
				
				}

				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw_interface [idle enter execs], state1_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (3,"wlan_mac_lpw_interface")


			/** state (idle) exit executives **/
			FSM_STATE_EXIT_UNFORCED (1, "idle", "wlan_mac_lpw_interface [idle exit execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw_interface [idle exit execs], state1_exit_exec)
				{
				/* The only interrupt expected in this state is a	*/
				/* stream interrupt. It can be either from the MAC	*/
				/* layer for a packet destined for this node or		*/
				/* from the application layer for a packet destined	*/
				/* for some other node.								*/
				intrpt_type = op_intrpt_type ();
				intrpt_strm = op_intrpt_strm ();
				pkptr = op_pk_get (intrpt_strm);
				
				}
				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw_interface [idle exit execs], state1_exit_exec)


			/** state (idle) transition processing **/
			FSM_PROFILE_SECTION_IN (wlan_mac_lpw_interface [idle trans conditions], state1_trans_conds)
			FSM_INIT_COND (APPL_LAYER_PKT_ARVL)
			FSM_TEST_COND (MAC_LAYER_PKT_ARVL)
			FSM_TEST_LOGIC ("idle")
			FSM_PROFILE_SECTION_OUT (wlan_mac_lpw_interface [idle trans conditions], state1_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 2, state2_enter_exec, ;, "APPL_LAYER_PKT_ARVL", "", "idle", "appl layer arrival")
				FSM_CASE_TRANSIT (1, 3, state3_enter_exec, ;, "MAC_LAYER_PKT_ARVL", "", "idle", "mac layer arrival")
				}
				/*---------------------------------------------------------*/



			/** state (appl layer arrival) enter executives **/
			FSM_STATE_ENTER_FORCED (2, "appl layer arrival", state2_enter_exec, "wlan_mac_lpw_interface [appl layer arrival enter execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw_interface [appl layer arrival enter execs], state2_enter_exec)
				{
				/* A packet has arrived from the application layer.	*/
				/* If the destination address specified is "Random"	*/
				/* then generate a destination and forward the appl	*/
				/* packet to the MAC layer with that information.	*/
				
				//$$$$$$$$$$$$$$$$$$ DSR $$$$$$$$$$$$$$$$$$$$$$$$
				// take the destination from the ici
				ici_dest_address = op_intrpt_ici();
				op_ici_attr_get(ici_dest_address,"Packet_Destination",&destination_address);
				//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
				
				if (1) op_prg_odb_bkpt("macs")
				if (destination_address == OMSC_AA_AUTO_ASSIGN)
					{
					/* Initialize current destination address to the Auto */
					/* Assign value										  */
					curr_dest_addr = destination_address;
				
					/* Call function to generate a random destination	*/
					/* from the pool of available addresses.			*/
					oms_aa_dest_addr_get (oms_aa_handle, &curr_dest_addr);
				
					/* Keep on generating the random addresses until source */
					/* address is not same as current address.				*/
					while (curr_dest_addr == mac_address)
						{
						/* Initialize current destination address to the Auto */
						/* Assign value										  */
						curr_dest_addr = destination_address;
				
						/* Call function to generate a random destination	*/
						/* from the pool of available addresses.			*/
						oms_aa_dest_addr_get (oms_aa_handle, &curr_dest_addr);
						}
					
					}
				else
					{
					/* The packet needs to be sent to an explicit	*/
					/* destination as specified in the "Destination	*/
					/* Address" attribute.							*/
					curr_dest_addr = destination_address;
					}
				/* Set this information in the interface control	*/
				/* information to be sent to the MAC layer.			*/
				op_ici_attr_set (wlan_mac_req_iciptr, "dest_addr", curr_dest_addr);
				
				/* Install the control informationand send it to	*/
				/* the MAC layer.									*/
				op_ici_install (wlan_mac_req_iciptr);
				op_pk_send (pkptr, outstrm_to_mac);
				
				}

				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw_interface [appl layer arrival enter execs], state2_enter_exec)

			/** state (appl layer arrival) exit executives **/
			FSM_STATE_EXIT_FORCED (2, "appl layer arrival", "wlan_mac_lpw_interface [appl layer arrival exit execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw_interface [appl layer arrival exit execs], state2_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw_interface [appl layer arrival exit execs], state2_exit_exec)


			/** state (appl layer arrival) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "appl layer arrival", "idle")
				/*---------------------------------------------------------*/



			/** state (mac layer arrival) enter executives **/
			FSM_STATE_ENTER_FORCED (3, "mac layer arrival", state3_enter_exec, "wlan_mac_lpw_interface [mac layer arrival enter execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw_interface [mac layer arrival enter execs], state3_enter_exec)
				{
				/* A packet arrived from the MAC layer. Since the MAC	*/
				/* layer would have forwarded this only if it were		*/
				/* destined for this node, forward this packet to the	*/
				/* sink module.	*/
				op_pk_send (pkptr, 0);
				//op_sim_end("reception packet couche mac at node\n","","","");
				if (1)
					op_prg_odb_bkpt("macr");
				}

				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw_interface [mac layer arrival enter execs], state3_enter_exec)

			/** state (mac layer arrival) exit executives **/
			FSM_STATE_EXIT_FORCED (3, "mac layer arrival", "wlan_mac_lpw_interface [mac layer arrival exit execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw_interface [mac layer arrival exit execs], state3_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw_interface [mac layer arrival exit execs], state3_exit_exec)


			/** state (mac layer arrival) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "mac layer arrival", "idle")
				/*---------------------------------------------------------*/



			/** state (wait) enter executives **/
			FSM_STATE_ENTER_UNFORCED (4, "wait", state4_enter_exec, "wlan_mac_lpw_interface [wait enter execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw_interface [wait enter execs], state4_enter_exec)
				{
				
				}

				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw_interface [wait enter execs], state4_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (9,"wlan_mac_lpw_interface")


			/** state (wait) exit executives **/
			FSM_STATE_EXIT_UNFORCED (4, "wait", "wlan_mac_lpw_interface [wait exit execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw_interface [wait exit execs], state4_exit_exec)
				{
				/* Obtain the MAC layer information for the local MAC	*/
				/* process from the model-wide registry.				*/
				proc_record_handle_list_ptr = op_prg_list_create ();
				oms_pr_process_discover (my_objid, proc_record_handle_list_ptr,
					"node objid",	OMSC_PR_OBJID,		my_node_objid,
					"protocol", 	OMSC_PR_STRING,		"mac",
					OPC_NIL);
				 
				/* If the MAC process regostered itself, then there	*/
				/* must be a valid match							*/
				record_handle_list_size = op_prg_list_size (proc_record_handle_list_ptr);
					
				if (record_handle_list_size !=  1)
					{
					/* An error should be created if there are more	*/
					/* than one WLAN-MAC process in the local node,	*/
					/* or if no match is found.						*/
					op_sim_end ("Error: either zero or several WLAN MAC processes found in the interface", "", "", "");
					}
				else
					{
					/*	Obtain a handle on the process record.	*/
					process_record_handle = (OmsT_Pr_Handle) op_prg_list_access (proc_record_handle_list_ptr, 0);
				 
					/* Obtain the module objid for the WLAN MAC module. 		*/
					oms_pr_attr_get (process_record_handle, "module objid", OMSC_PR_OBJID, &mac_module_objid);
				 
					/* Obtain the stream numbers connected to and from the	*/
					/* WLAN MAC layer process.								*/
					oms_tan_neighbor_streams_find (my_objid, mac_module_objid, &instrm_from_mac, &outstrm_to_mac);
				 
					/* Obtain the address handle maintained by the MAC process.	*/
					oms_pr_attr_get (process_record_handle, "address",             OMSC_PR_NUMBER,  &ne_address);
					oms_pr_attr_get (process_record_handle, "auto address handle", OMSC_PR_ADDRESS, &oms_aa_handle);
				 
					/* Set the variable to indicate the MAC address of the	*/
					/* associated MAC layer process.						*/
					mac_address = (int) ne_address;
					}
				
				/* Check if the specified destination address (via the "Destination	*/
				/* Address" attribute) is valid or not.								*/
				if (destination_address != OMSC_AA_AUTO_ASSIGN)
					{
					/* An explicit destination address has been specified. Check if	*/
					/* it is a valid address.										*/
					if (destination_address == (int) mac_address)
						{
						/* Write a log stating that source node can not be a destination. */
						printf ("\n\nInvalid destination address specification. Changing\n");
						printf ("the specification from its own MAC address (%d) to \"Random\"\n\n", destination_address);
				
						/* The destination address is set same as the lower layer MAC	*/
						/* address of this node. Ignore the original specification and	*/
						/* and set it to random.										*/
						destination_address = OMSC_AA_AUTO_ASSIGN;
						}
					else
						{
						/* Checks if this is a valid address from the pool of addresses	*/
						/* available.													*/
				
						dest_addr_okay = oms_aa_dest_addr_check (oms_aa_handle, destination_address);
						if (dest_addr_okay != OPC_TRUE)
							{
							/* Write a log stating that source node can not be a destination. */
							printf ("\n\nInvalid destination address specification. Changing\n");
							printf ("the specification from %d, to \"Random\"\n\n", destination_address);
				
							/* The specified destination address is invalid. Set it to	*/
							/* random.													*/
							destination_address = OMSC_AA_AUTO_ASSIGN;
							}
						}
					}
				printf("La destination est maintenant %d\n",destination_address);
				
				// $$$$$$$$$$$$$$$$ DSR TEMPORARY "BUG" FIXING $$$$$$$$$$$$$$$$$$$$$$$$$$$
				//instrm_from_mac=1; 
				//outstrm_to_mac=1;
				// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
				}
				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw_interface [wait exit execs], state4_exit_exec)


			/** state (wait) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "wait", "idle")
				/*---------------------------------------------------------*/



			/** state (init2) enter executives **/
			FSM_STATE_ENTER_UNFORCED (5, "init2", state5_enter_exec, "wlan_mac_lpw_interface [init2 enter execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw_interface [init2 enter execs], state5_enter_exec)
				{
				}

				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw_interface [init2 enter execs], state5_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (11,"wlan_mac_lpw_interface")


			/** state (init2) exit executives **/
			FSM_STATE_EXIT_UNFORCED (5, "init2", "wlan_mac_lpw_interface [init2 exit execs]")
				FSM_PROFILE_SECTION_IN (wlan_mac_lpw_interface [init2 exit execs], state5_exit_exec)
				{
				/* Schedule a self interrupt to wait for lower layer	*/
				/* Wlan MAC process to finalize the MAC address			*/
				/* registration and resolution.							*/
				op_intrpt_schedule_self (op_sim_time (), 0);
				}
				FSM_PROFILE_SECTION_OUT (wlan_mac_lpw_interface [init2 exit execs], state5_exit_exec)


			/** state (init2) transition processing **/
			FSM_TRANSIT_FORCE (4, state4_enter_exec, ;, "default", "", "init2", "wait")
				/*---------------------------------------------------------*/



			}


		FSM_EXIT (0,"wlan_mac_lpw_interface")
		}
	}




void
wlan_mac_lpw_interface_diag (OP_SIM_CONTEXT_ARG_OPT)
	{
	/* No Diagnostic Block */
	}




void
wlan_mac_lpw_interface_terminate (OP_SIM_CONTEXT_ARG_OPT)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = __LINE__;
#endif

	FIN_MT (wlan_mac_lpw_interface_terminate ())

	Vos_Poolmem_Dealloc_MT (OP_SIM_CONTEXT_THREAD_INDEX_COMMA pr_state_ptr);

	FOUT
	}


/* Undefine shortcuts to state variables to avoid */
/* syntax error in direct access to fields of */
/* local variable prs_ptr in wlan_mac_lpw_interface_svar function. */
#undef my_objid
#undef my_node_objid
#undef instrm_from_mac
#undef outstrm_to_mac
#undef destination_address
#undef oms_aa_handle
#undef mac_address
#undef wlan_mac_req_iciptr

#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE

#define FIN_PREAMBLE_DEC
#define FIN_PREAMBLE_CODE

VosT_Obtype
wlan_mac_lpw_interface_init (int * init_block_ptr)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	VosT_Obtype obtype = OPC_NIL;
	FIN_MT (wlan_mac_lpw_interface_init (init_block_ptr))

	Vos_Define_Object (&obtype, "proc state vars (wlan_mac_lpw_interface)",
		sizeof (wlan_mac_lpw_interface_state), 0, 20);
	*init_block_ptr = 0;

	FRET (obtype)
	}

VosT_Address
wlan_mac_lpw_interface_alloc (VOS_THREAD_INDEX_ARG_COMMA VosT_Obtype obtype, int init_block)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	wlan_mac_lpw_interface_state * ptr;
	FIN_MT (wlan_mac_lpw_interface_alloc (obtype))

	ptr = (wlan_mac_lpw_interface_state *)Vos_Alloc_Object_MT (VOS_THREAD_INDEX_COMMA obtype);
	if (ptr != OPC_NIL)
		ptr->_op_current_block = init_block;
	FRET ((VosT_Address)ptr)
	}



void
wlan_mac_lpw_interface_svar (void * gen_ptr, const char * var_name, void ** var_p_ptr)
	{
	wlan_mac_lpw_interface_state		*prs_ptr;

	FIN_MT (wlan_mac_lpw_interface_svar (gen_ptr, var_name, var_p_ptr))

	if (var_name == OPC_NIL)
		{
		*var_p_ptr = (void *)OPC_NIL;
		FOUT
		}
	prs_ptr = (wlan_mac_lpw_interface_state *)gen_ptr;

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
	if (strcmp ("instrm_from_mac" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->instrm_from_mac);
		FOUT
		}
	if (strcmp ("outstrm_to_mac" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->outstrm_to_mac);
		FOUT
		}
	if (strcmp ("destination_address" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->destination_address);
		FOUT
		}
	if (strcmp ("oms_aa_handle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->oms_aa_handle);
		FOUT
		}
	if (strcmp ("mac_address" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->mac_address);
		FOUT
		}
	if (strcmp ("wlan_mac_req_iciptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->wlan_mac_req_iciptr);
		FOUT
		}
	*var_p_ptr = (void *)OPC_NIL;

	FOUT
	}

