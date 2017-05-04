/* Process model C form file: lpw_simple_source.pr.c */
/* Portions of this file copyright 1992-2003 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
const char lpw_simple_source_pr_c [] = "MIL_3_Tfile_Hdr_ 100A 30A op_runsim 7 49B08F4F 49B08F4F 1 lpw Administrator 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 8f3 1                                                                                                                                                                                                                                                                                                                                                                                                       ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>



/* Header Block */

/* Include files.					*/
#include	<oms_dist_support.h>

/* Special attribute values.		*/
#define		SSC_INFINITE_TIME		-1.0

/* Interrupt code values.			*/
#define		SSC_START				0
#define		SSC_GENERATE			1
#define		SSC_STOP				2

/* Node configuration constants.	*/
#define		SSC_STRM_TO_LOW			0

/* Macro definitions for state		*/
/* transitions.						*/
#define		START				(intrpt_code == SSC_START)
#define		DISABLED			(intrpt_code == SSC_STOP)
#define		STOP				(intrpt_code == SSC_STOP)
#define		PACKET_GENERATE		(intrpt_code == SSC_GENERATE)

/* Function prototypes.				*/
static void			ss_packet_generate (void);

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
	Objid	                  		own_id;
	char	                   		format_str [64];
	double	                 		start_time;
	double	                 		stop_time;
	OmsT_Dist_Handle	       		interarrival_dist_ptr;
	OmsT_Dist_Handle	       		pksize_dist_ptr;
	Boolean	                		generate_unformatted;
	Evhandle	               		next_pk_evh;
	double	                 		next_intarr_time;
	Stathandle	             		bits_sent_hndl;
	Stathandle	             		packets_sent_hndl;
	Stathandle	             		packet_size_hndl;
	Stathandle	             		interarrivals_hndl;
	} lpw_simple_source_state;

#define pr_state_ptr            		((lpw_simple_source_state*) (OP_SIM_CONTEXT_PTR->mod_state_ptr))
#define own_id                  		pr_state_ptr->own_id
#define format_str              		pr_state_ptr->format_str
#define start_time              		pr_state_ptr->start_time
#define stop_time               		pr_state_ptr->stop_time
#define interarrival_dist_ptr   		pr_state_ptr->interarrival_dist_ptr
#define pksize_dist_ptr         		pr_state_ptr->pksize_dist_ptr
#define generate_unformatted    		pr_state_ptr->generate_unformatted
#define next_pk_evh             		pr_state_ptr->next_pk_evh
#define next_intarr_time        		pr_state_ptr->next_intarr_time
#define bits_sent_hndl          		pr_state_ptr->bits_sent_hndl
#define packets_sent_hndl       		pr_state_ptr->packets_sent_hndl
#define packet_size_hndl        		pr_state_ptr->packet_size_hndl
#define interarrivals_hndl      		pr_state_ptr->interarrivals_hndl

/* These macro definitions will define a local variable called	*/
/* "op_sv_ptr" in each function containing a FIN statement.	*/
/* This variable points to the state variable data structure,	*/
/* and can be used from a C debugger to display their values.	*/
#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE
#if defined (OPD_PARALLEL)
#  define FIN_PREAMBLE_DEC	lpw_simple_source_state *op_sv_ptr; OpT_Sim_Context * tcontext_ptr;
#  define FIN_PREAMBLE_CODE	\
		if (VosS_Mt_Perform_Lock) \
			VOS_THREAD_SPECIFIC_DATA_GET (VosI_Globals.simi_mt_context_data_key, tcontext_ptr, SimT_Context *); \
		else \
			tcontext_ptr = VosI_Globals.simi_sequential_context_ptr; \
		op_sv_ptr = ((lpw_simple_source_state *)(tcontext_ptr->mod_state_ptr));
#else
#  define FIN_PREAMBLE_DEC	lpw_simple_source_state *op_sv_ptr;
#  define FIN_PREAMBLE_CODE	op_sv_ptr = pr_state_ptr;
#endif


/* Function Block */


#if !defined (VOSD_NO_FIN)
enum { _op_block_origin = __LINE__ };
#endif
static void
ss_packet_generate (void)
	{
	Packet*				pkptr;
	double				pksize;

	/** This function creates a packet based on the packet generation		**/
	/** specifications of the source model and sends it to the lower layer.	**/
	FIN (ss_packet_generate ());

	/* Generate a packet size outcome.					*/
	pksize = (double) ceil (oms_dist_outcome (pksize_dist_ptr));
	
	/* Create a packet of specified format and size.	*/
	if (generate_unformatted == OPC_TRUE)
		{
		/* We produce unformatted packets. Create one.	*/
		pkptr = op_pk_create (pksize);
		}
	else
		{
		/* Create a packet with the specified format.	*/
		pkptr = op_pk_create_fmt (format_str);
		op_pk_total_size_set (pkptr, pksize);
		}

	/* Update the packet generation statistics.			*/
	op_stat_write (packets_sent_hndl, 1.0);
	op_stat_write (packets_sent_hndl, 0.0);
	op_stat_write (bits_sent_hndl, (double) pksize);
	op_stat_write (bits_sent_hndl, 0.0);
	op_stat_write (packet_size_hndl, (double) pksize);
	op_stat_write (interarrivals_hndl, next_intarr_time);

	/* Send the packet via the stream to the lower layer.	*/
	op_pk_send (pkptr, SSC_STRM_TO_LOW);

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
	void lpw_simple_source (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Obtype lpw_simple_source_init (int * init_block_ptr);
	VosT_Address lpw_simple_source_alloc (VOS_THREAD_INDEX_ARG_COMMA VosT_Obtype, int);
	void lpw_simple_source_diag (OP_SIM_CONTEXT_ARG_OPT);
	void lpw_simple_source_terminate (OP_SIM_CONTEXT_ARG_OPT);
	void lpw_simple_source_svar (void *, const char *, void **);


	VosT_Fun_Status Vos_Define_Object (VosT_Obtype * obst_ptr, const char * name, unsigned int size, unsigned int init_obs, unsigned int inc_obs);
	VosT_Address Vos_Alloc_Object_MT (VOS_THREAD_INDEX_ARG_COMMA VosT_Obtype ob_hndl);
	VosT_Fun_Status Vos_Poolmem_Dealloc_MT (VOS_THREAD_INDEX_ARG_COMMA VosT_Address ob_ptr);
#if defined (__cplusplus)
} /* end of 'extern "C"' */
#endif




/* Process model interrupt handling procedure */


void
lpw_simple_source (OP_SIM_CONTEXT_ARG_OPT)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	FIN_MT (lpw_simple_source ());
	if (1)
		{
		/* Variables used in the "init" state.		*/
		char		interarrival_str [128];
		char		size_str [128];
		Prg_List*	pk_format_names_lptr;
		char*		found_format_str;
		int			low, high;
		Boolean		format_found;
		int			i;
		
		/* Variables used in state transitions.		*/
		int			intrpt_code;


		FSM_ENTER ("lpw_simple_source")

		FSM_BLOCK_SWITCH
			{
			/*---------------------------------------------------------*/
			/** state (init) enter executives **/
			FSM_STATE_ENTER_UNFORCED_NOLABEL (0, "init", "lpw_simple_source [init enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_simple_source [init enter execs], state0_enter_exec)
				{
				/* At this initial state, we read the values of source attributes	*/
				/* and schedule a selt interrupt that will indicate our start time	*/
				/* for packet generation.											*/
				
				/* Obtain the object id of the surrounding module.					*/
				own_id = op_id_self ();
				
				/* Read the values of the packet generation parameters, i.e. the	*/
				/* attribute values of the surrounding module.						*/
				op_ima_obj_attr_get (own_id, "Packet Interarrival Time", interarrival_str);
				op_ima_obj_attr_get (own_id, "Packet Size",              size_str);
				op_ima_obj_attr_get (own_id, "Packet Format",            format_str);
				op_ima_obj_attr_get (own_id, "Start Time",               &start_time);
				op_ima_obj_attr_get (own_id, "Stop Time",                &stop_time);
				
				/* Load the PDFs that will be used in computing the packet			*/
				/* interarrival times and packet sizes.								*/
				interarrival_dist_ptr = oms_dist_load_from_string (interarrival_str);
				pksize_dist_ptr       = oms_dist_load_from_string (size_str);
				
				/* Verify the existence of the packet format to be used for			*/
				/* generated packets.												*/
				if (strcmp (format_str, "NONE") == 0)
					{
					/* We will generate unformatted packets. Set the flag.			*/
					generate_unformatted = OPC_TRUE;
					}
				else
					{
					/* We will generate formatted packets. Turn off the flag.		*/
					generate_unformatted = OPC_FALSE;
				
					/* Get the list of all available packet formats.				*/
					pk_format_names_lptr = prg_tfile_name_list_get (PrgC_Tfile_Type_Packet_Format);
				
					/* Search the list for the requested packet format.				*/
					format_found = OPC_FALSE;
					for (i = prg_list_size (pk_format_names_lptr); ((format_found == OPC_FALSE) && (i > 0)); i--)
						{
						/* Access the next format name and compare with requested 	*/
						/* format name.												*/
						found_format_str = (char *) prg_list_access (pk_format_names_lptr, i - 1); 
						if (strcmp (found_format_str, format_str) == 0)
							format_found = OPC_TRUE;
						}
				
				    if (format_found == OPC_FALSE)
						{
						/* The requested format does not exist. Generate 			*/
						/* unformatted packets.										*/
						generate_unformatted = OPC_TRUE;
					
						/* Display an appropriate warning.							*/
						op_prg_odb_print_major ("Warning from simple packet generator model (simple_source):", 
												"The specified packet format", format_str,
												"is not found. Generating unformatted packets instead.", OPC_NIL);
						}
				
					/* Destroy the lits and its elements since we don't need it		*/
					/* anymore.														*/
					prg_list_free (pk_format_names_lptr);
					prg_mem_free  (pk_format_names_lptr);
					}	
				 
					 
				/* Make sure we have valid start and stop times, i.e. stop time is	*/
				/* not earlier than start time.										*/
				if ((stop_time <= start_time) && (stop_time != SSC_INFINITE_TIME))
					{
					/* Stop time is earlier than start time. Disable the source.	*/
					start_time = SSC_INFINITE_TIME;
				
					/* Display an appropriate warning.								*/
					op_prg_odb_print_major ("Warning from simple packet generator model (simple_source):", 
											"Although the generator is not disabled (start time is set to a finite value),",
											"a stop time that is not later than the start time is specified.",
											"Disabling the generator.", OPC_NIL);
					}
				
				/* Schedule a self interrupt that will indicate our start time for	*/
				/* packet generation activities. If the source is disabled,			*/
				/* schedule it at current time with the appropriate code value.		*/
				if (start_time == SSC_INFINITE_TIME)
					{
					op_intrpt_schedule_self (op_sim_time (), SSC_STOP);
					}
				else
					{
					op_intrpt_schedule_self (start_time, SSC_START);
				
					/* In this case, also schedule the interrupt when we will stop	*/
					/* generating packets, unless we are configured to run until	*/
					/* the end of the simulation.									*/
					if (stop_time != SSC_INFINITE_TIME)
						{
						op_intrpt_schedule_self (stop_time, SSC_STOP);
						}
					
					next_intarr_time = oms_dist_outcome (interarrival_dist_ptr);
				
					/* Make sure that interarrival time is not negative. In that case it */
					/* will be set to 0.												 */
					if (next_intarr_time <0)
						{
						next_intarr_time = 0.0;
						}
				
					}
				
				/* Register the statistics that will be maintained by this model.	*/
				bits_sent_hndl     	= op_stat_reg ("Generator.Traffic Sent (bits/sec)",			OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
				packets_sent_hndl   = op_stat_reg ("Generator.Traffic Sent (packets/sec)",		OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
				packet_size_hndl    = op_stat_reg ("Generator.Packet Size (bits)",              OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
				interarrivals_hndl  = op_stat_reg ("Generator.Packet Interarrival Time (secs)", OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
				
				}

				FSM_PROFILE_SECTION_OUT (lpw_simple_source [init enter execs], state0_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (1,"lpw_simple_source")


			/** state (init) exit executives **/
			FSM_STATE_EXIT_UNFORCED (0, "init", "lpw_simple_source [init exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_simple_source [init exit execs], state0_exit_exec)
				{
				/* Determine the code of the interrupt, which is used in evaluating	*/
				/* state transition conditions.										*/
				intrpt_code = op_intrpt_code ();
				}
				FSM_PROFILE_SECTION_OUT (lpw_simple_source [init exit execs], state0_exit_exec)


			/** state (init) transition processing **/
			FSM_PROFILE_SECTION_IN (lpw_simple_source [init trans conditions], state0_trans_conds)
			FSM_INIT_COND (START)
			FSM_TEST_COND (DISABLED)
			FSM_TEST_LOGIC ("init")
			FSM_PROFILE_SECTION_OUT (lpw_simple_source [init trans conditions], state0_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 1, state1_enter_exec, ss_packet_generate();, "START", "ss_packet_generate()", "init", "generate")
				FSM_CASE_TRANSIT (1, 2, state2_enter_exec, ;, "DISABLED", "", "init", "stop")
				}
				/*---------------------------------------------------------*/



			/** state (generate) enter executives **/
			FSM_STATE_ENTER_UNFORCED (1, "generate", state1_enter_exec, "lpw_simple_source [generate enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_simple_source [generate enter execs], state1_enter_exec)
				{
				/* At the enter execs of the "generate" state we schedule the		*/
				/* arrival of the next packet.										*/
				next_intarr_time = oms_dist_outcome (interarrival_dist_ptr);
				
				/* Make sure that interarrival time is not negative. In that case it */
				/* will be set to 0.												 */
				if (next_intarr_time <0)
					{
					next_intarr_time = 0;
					}
				
				next_pk_evh      = op_intrpt_schedule_self (op_sim_time () + next_intarr_time, SSC_GENERATE);
				
				}

				FSM_PROFILE_SECTION_OUT (lpw_simple_source [generate enter execs], state1_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (3,"lpw_simple_source")


			/** state (generate) exit executives **/
			FSM_STATE_EXIT_UNFORCED (1, "generate", "lpw_simple_source [generate exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_simple_source [generate exit execs], state1_exit_exec)
				{
				/* Determine the code of the interrupt, which is used in evaluating	*/
				/* state transition conditions.										*/
				intrpt_code = op_intrpt_code ();
				
				}
				FSM_PROFILE_SECTION_OUT (lpw_simple_source [generate exit execs], state1_exit_exec)


			/** state (generate) transition processing **/
			FSM_PROFILE_SECTION_IN (lpw_simple_source [generate trans conditions], state1_trans_conds)
			FSM_INIT_COND (STOP)
			FSM_TEST_COND (PACKET_GENERATE)
			FSM_TEST_LOGIC ("generate")
			FSM_PROFILE_SECTION_OUT (lpw_simple_source [generate trans conditions], state1_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 2, state2_enter_exec, ;, "STOP", "", "generate", "stop")
				FSM_CASE_TRANSIT (1, 1, state1_enter_exec, ss_packet_generate();, "PACKET_GENERATE", "ss_packet_generate()", "generate", "generate")
				}
				/*---------------------------------------------------------*/



			/** state (stop) enter executives **/
			FSM_STATE_ENTER_UNFORCED (2, "stop", state2_enter_exec, "lpw_simple_source [stop enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_simple_source [stop enter execs], state2_enter_exec)
				{
				/* When we enter into the "stop" state, it is the time for us to	*/
				/* stop generating traffic. We simply cancel the generation of the	*/
				/* next packet and go into a silent mode by not scheduling anything	*/
				/* else.															*/
				if (op_ev_valid (next_pk_evh) == OPC_TRUE)
					{
					op_ev_cancel (next_pk_evh);
					}
				
				}

				FSM_PROFILE_SECTION_OUT (lpw_simple_source [stop enter execs], state2_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (5,"lpw_simple_source")


			/** state (stop) exit executives **/
			FSM_STATE_EXIT_UNFORCED (2, "stop", "lpw_simple_source [stop exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_simple_source [stop exit execs], state2_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_simple_source [stop exit execs], state2_exit_exec)


			/** state (stop) transition processing **/
			FSM_TRANSIT_MISSING ("stop")
				/*---------------------------------------------------------*/



			}


		FSM_EXIT (0,"lpw_simple_source")
		}
	}




void
lpw_simple_source_diag (OP_SIM_CONTEXT_ARG_OPT)
	{
	/* No Diagnostic Block */
	}




void
lpw_simple_source_terminate (OP_SIM_CONTEXT_ARG_OPT)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = __LINE__;
#endif

	FIN_MT (lpw_simple_source_terminate ())

	Vos_Poolmem_Dealloc_MT (OP_SIM_CONTEXT_THREAD_INDEX_COMMA pr_state_ptr);

	FOUT
	}


/* Undefine shortcuts to state variables to avoid */
/* syntax error in direct access to fields of */
/* local variable prs_ptr in lpw_simple_source_svar function. */
#undef own_id
#undef format_str
#undef start_time
#undef stop_time
#undef interarrival_dist_ptr
#undef pksize_dist_ptr
#undef generate_unformatted
#undef next_pk_evh
#undef next_intarr_time
#undef bits_sent_hndl
#undef packets_sent_hndl
#undef packet_size_hndl
#undef interarrivals_hndl

#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE

#define FIN_PREAMBLE_DEC
#define FIN_PREAMBLE_CODE

VosT_Obtype
lpw_simple_source_init (int * init_block_ptr)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	VosT_Obtype obtype = OPC_NIL;
	FIN_MT (lpw_simple_source_init (init_block_ptr))

	Vos_Define_Object (&obtype, "proc state vars (lpw_simple_source)",
		sizeof (lpw_simple_source_state), 0, 20);
	*init_block_ptr = 0;

	FRET (obtype)
	}

VosT_Address
lpw_simple_source_alloc (VOS_THREAD_INDEX_ARG_COMMA VosT_Obtype obtype, int init_block)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	lpw_simple_source_state * ptr;
	FIN_MT (lpw_simple_source_alloc (obtype))

	ptr = (lpw_simple_source_state *)Vos_Alloc_Object_MT (VOS_THREAD_INDEX_COMMA obtype);
	if (ptr != OPC_NIL)
		ptr->_op_current_block = init_block;
	FRET ((VosT_Address)ptr)
	}



void
lpw_simple_source_svar (void * gen_ptr, const char * var_name, void ** var_p_ptr)
	{
	lpw_simple_source_state		*prs_ptr;

	FIN_MT (lpw_simple_source_svar (gen_ptr, var_name, var_p_ptr))

	if (var_name == OPC_NIL)
		{
		*var_p_ptr = (void *)OPC_NIL;
		FOUT
		}
	prs_ptr = (lpw_simple_source_state *)gen_ptr;

	if (strcmp ("own_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->own_id);
		FOUT
		}
	if (strcmp ("format_str" , var_name) == 0)
		{
		*var_p_ptr = (void *) (prs_ptr->format_str);
		FOUT
		}
	if (strcmp ("start_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->start_time);
		FOUT
		}
	if (strcmp ("stop_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->stop_time);
		FOUT
		}
	if (strcmp ("interarrival_dist_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->interarrival_dist_ptr);
		FOUT
		}
	if (strcmp ("pksize_dist_ptr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->pksize_dist_ptr);
		FOUT
		}
	if (strcmp ("generate_unformatted" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->generate_unformatted);
		FOUT
		}
	if (strcmp ("next_pk_evh" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->next_pk_evh);
		FOUT
		}
	if (strcmp ("next_intarr_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->next_intarr_time);
		FOUT
		}
	if (strcmp ("bits_sent_hndl" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->bits_sent_hndl);
		FOUT
		}
	if (strcmp ("packets_sent_hndl" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->packets_sent_hndl);
		FOUT
		}
	if (strcmp ("packet_size_hndl" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->packet_size_hndl);
		FOUT
		}
	if (strcmp ("interarrivals_hndl" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->interarrivals_hndl);
		FOUT
		}
	*var_p_ptr = (void *)OPC_NIL;

	FOUT
	}

