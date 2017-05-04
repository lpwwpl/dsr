/* Process model C form file: lpw_sink_test.pr.c */
/* Portions of this file copyright 1992-2003 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
const char lpw_sink_test_pr_c [] = "MIL_3_Tfile_Hdr_ 100A 30A op_runsim 7 49AF28B0 49AF28B0 1 lpw Administrator 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 8f3 1                                                                                                                                                                                                                                                                                                                                                                                                       ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>



/* Header Block */

///////////////////////////////////////////////////////////////
// DSR RECEIVER HEADER BLOCK
//
// Declaration of every constant, type, lybrary, glabal 
// variables... used by the dsr receiver process
///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
///////////////// CONSTANTS DEFINITION ////////////////////////
///////////////////////////////////////////////////////////////

// stream number definition
#define FROM_DSR_INTERFACE_STRM		0

///////////////////////////////////////////////////////////////
///////////// TRANSITION MACROS DEFINITION ////////////////////
///////////////////////////////////////////////////////////////

#define PKT_ARRIVAL	(op_intrpt_type ()==OPC_INTRPT_STRM)

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
	} lpw_sink_test_state;

#define pr_state_ptr            		((lpw_sink_test_state*) (OP_SIM_CONTEXT_PTR->mod_state_ptr))

/* These macro definitions will define a local variable called	*/
/* "op_sv_ptr" in each function containing a FIN statement.	*/
/* This variable points to the state variable data structure,	*/
/* and can be used from a C debugger to display their values.	*/
#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE
#if defined (OPD_PARALLEL)
#  define FIN_PREAMBLE_DEC	lpw_sink_test_state *op_sv_ptr; OpT_Sim_Context * tcontext_ptr;
#  define FIN_PREAMBLE_CODE	\
		if (VosS_Mt_Perform_Lock) \
			VOS_THREAD_SPECIFIC_DATA_GET (VosI_Globals.simi_mt_context_data_key, tcontext_ptr, SimT_Context *); \
		else \
			tcontext_ptr = VosI_Globals.simi_sequential_context_ptr; \
		op_sv_ptr = ((lpw_sink_test_state *)(tcontext_ptr->mod_state_ptr));
#else
#  define FIN_PREAMBLE_DEC	lpw_sink_test_state *op_sv_ptr;
#  define FIN_PREAMBLE_CODE	op_sv_ptr = pr_state_ptr;
#endif


/* No Function Block */


#if !defined (VOSD_NO_FIN)
enum { _op_block_origin = __LINE__ };
#endif

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
	void lpw_sink_test (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Obtype lpw_sink_test_init (int * init_block_ptr);
	VosT_Address lpw_sink_test_alloc (VOS_THREAD_INDEX_ARG_COMMA VosT_Obtype, int);
	void lpw_sink_test_diag (OP_SIM_CONTEXT_ARG_OPT);
	void lpw_sink_test_terminate (OP_SIM_CONTEXT_ARG_OPT);
	void lpw_sink_test_svar (void *, const char *, void **);


	VosT_Fun_Status Vos_Define_Object (VosT_Obtype * obst_ptr, const char * name, unsigned int size, unsigned int init_obs, unsigned int inc_obs);
	VosT_Address Vos_Alloc_Object_MT (VOS_THREAD_INDEX_ARG_COMMA VosT_Obtype ob_hndl);
	VosT_Fun_Status Vos_Poolmem_Dealloc_MT (VOS_THREAD_INDEX_ARG_COMMA VosT_Address ob_ptr);
#if defined (__cplusplus)
} /* end of 'extern "C"' */
#endif




/* Process model interrupt handling procedure */


void
lpw_sink_test (OP_SIM_CONTEXT_ARG_OPT)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	FIN_MT (lpw_sink_test ());
	if (1)
		{
		///////////////////////////////////////////////////////////////
		// DSR SINK TEMPORARY VARIABLES BLOCK
		//
		// Declaration of the temporary variables
		///////////////////////////////////////////////////////////////
		
		Packet* pk_ptr;


		FSM_ENTER ("lpw_sink_test")

		FSM_BLOCK_SWITCH
			{
			/*---------------------------------------------------------*/
			/** state (idle) enter executives **/
			FSM_STATE_ENTER_UNFORCED (0, "idle", state0_enter_exec, "lpw_sink_test [idle enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_sink_test [idle enter execs], state0_enter_exec)
				{
				}

				FSM_PROFILE_SECTION_OUT (lpw_sink_test [idle enter execs], state0_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (1,"lpw_sink_test")


			/** state (idle) exit executives **/
			FSM_STATE_EXIT_UNFORCED (0, "idle", "lpw_sink_test [idle exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_sink_test [idle exit execs], state0_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_sink_test [idle exit execs], state0_exit_exec)


			/** state (idle) transition processing **/
			FSM_PROFILE_SECTION_IN (lpw_sink_test [idle trans conditions], state0_trans_conds)
			FSM_INIT_COND (PKT_ARRIVAL)
			FSM_DFLT_COND
			FSM_TEST_LOGIC ("idle")
			FSM_PROFILE_SECTION_OUT (lpw_sink_test [idle trans conditions], state0_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 1, state1_enter_exec, ;, "PKT_ARRIVAL", "", "idle", "sink")
				FSM_CASE_TRANSIT (1, 0, state0_enter_exec, ;, "default", "", "idle", "idle")
				}
				/*---------------------------------------------------------*/



			/** state (sink) enter executives **/
			FSM_STATE_ENTER_FORCED (1, "sink", state1_enter_exec, "lpw_sink_test [sink enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_sink_test [sink enter execs], state1_enter_exec)
				{
				///////////////////////////////////////////////////////////////
				// SINK STATE
				//
				// Just destroyed the received packets
				///////////////////////////////////////////////////////////////
				
				// just extract and destroy the packet received from the dsr interface (as a sink)
				printf("I receive data!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
				pk_ptr=op_pk_get(FROM_DSR_INTERFACE_STRM);
				op_pk_destroy(pk_ptr);
				}

				FSM_PROFILE_SECTION_OUT (lpw_sink_test [sink enter execs], state1_enter_exec)

			/** state (sink) exit executives **/
			FSM_STATE_EXIT_FORCED (1, "sink", "lpw_sink_test [sink exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_sink_test [sink exit execs], state1_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_sink_test [sink exit execs], state1_exit_exec)


			/** state (sink) transition processing **/
			FSM_TRANSIT_FORCE (0, state0_enter_exec, ;, "default", "", "sink", "idle")
				/*---------------------------------------------------------*/



			}


		FSM_EXIT (0,"lpw_sink_test")
		}
	}




void
lpw_sink_test_diag (OP_SIM_CONTEXT_ARG_OPT)
	{
	/* No Diagnostic Block */
	}




void
lpw_sink_test_terminate (OP_SIM_CONTEXT_ARG_OPT)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = __LINE__;
#endif

	FIN_MT (lpw_sink_test_terminate ())

	Vos_Poolmem_Dealloc_MT (OP_SIM_CONTEXT_THREAD_INDEX_COMMA pr_state_ptr);

	FOUT
	}


#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE

#define FIN_PREAMBLE_DEC
#define FIN_PREAMBLE_CODE

VosT_Obtype
lpw_sink_test_init (int * init_block_ptr)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	VosT_Obtype obtype = OPC_NIL;
	FIN_MT (lpw_sink_test_init (init_block_ptr))

	Vos_Define_Object (&obtype, "proc state vars (lpw_sink_test)",
		sizeof (lpw_sink_test_state), 0, 20);
	*init_block_ptr = 0;

	FRET (obtype)
	}

VosT_Address
lpw_sink_test_alloc (VOS_THREAD_INDEX_ARG_COMMA VosT_Obtype obtype, int init_block)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	lpw_sink_test_state * ptr;
	FIN_MT (lpw_sink_test_alloc (obtype))

	ptr = (lpw_sink_test_state *)Vos_Alloc_Object_MT (VOS_THREAD_INDEX_COMMA obtype);
	if (ptr != OPC_NIL)
		ptr->_op_current_block = init_block;
	FRET ((VosT_Address)ptr)
	}



void
lpw_sink_test_svar (void * gen_ptr, const char * var_name, void ** var_p_ptr)
	{

	FIN_MT (lpw_sink_test_svar (gen_ptr, var_name, var_p_ptr))

	*var_p_ptr = (void *)OPC_NIL;

	FOUT
	}

