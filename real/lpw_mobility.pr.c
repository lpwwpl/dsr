/* Process model C form file: lpw_mobility.pr.c */
/* Portions of this file copyright 1992-2003 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
const char lpw_mobility_pr_c [] = "MIL_3_Tfile_Hdr_ 100A 30A op_runsim 7 49AD4724 49AD4724 1 lpw Administrator 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 8f3 1                                                                                                                                                                                                                                                                                                                                                                                                       ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>



/* Header Block */

///////////////////////////////////////////////////////////////
// BILLARD MOBILITY HEADER BLOCK
//
// Declaration of every constant, type, library, global 
// variables... used by the billard nobility process
///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
//////////////////////// INCLUDE ////////////////////////////// 
///////////////////////////////////////////////////////////////

#include <math.h>

///////////////////////////////////////////////////////////////
///////////////// CONSTANTS DEFINITION ////////////////////////
///////////////////////////////////////////////////////////////

// constant PI for trigonometry
#define PI	 3.141592654

///////////////////////////////////////////////////////////////
///////////// TRANSITION MACROS DEFINITION ////////////////////
///////////////////////////////////////////////////////////////

#define MOVE ( op_intrpt_type () == OPC_INTRPT_SELF ) 

///////////////////////////////////////////////////////////////
////////////// GLOBAL VARIABLES DECLARATION ///////////////////
///////////////////////////////////////////////////////////////

// variable that define the size of the grid on which the nodes are moving
double XMAX; 
double XMIN;
double YMIN;
double YMAX;

// variable which define the node mobilty (speed = STEP_DIST/ POS_TIMER)
double POS_TIMER;
double STEP_DIST;

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
	Distribution *	         		one;
	Objid	                  		my_node_id;
	Objid	                  		my_process_id;
	double	                 		angle;
	Objid	                  		my_net_id;
	} lpw_mobility_state;

#define pr_state_ptr            		((lpw_mobility_state*) (OP_SIM_CONTEXT_PTR->mod_state_ptr))
#define one                     		pr_state_ptr->one
#define my_node_id              		pr_state_ptr->my_node_id
#define my_process_id           		pr_state_ptr->my_process_id
#define angle                   		pr_state_ptr->angle
#define my_net_id               		pr_state_ptr->my_net_id

/* These macro definitions will define a local variable called	*/
/* "op_sv_ptr" in each function containing a FIN statement.	*/
/* This variable points to the state variable data structure,	*/
/* and can be used from a C debugger to display their values.	*/
#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE
#if defined (OPD_PARALLEL)
#  define FIN_PREAMBLE_DEC	lpw_mobility_state *op_sv_ptr; OpT_Sim_Context * tcontext_ptr;
#  define FIN_PREAMBLE_CODE	\
		if (VosS_Mt_Perform_Lock) \
			VOS_THREAD_SPECIFIC_DATA_GET (VosI_Globals.simi_mt_context_data_key, tcontext_ptr, SimT_Context *); \
		else \
			tcontext_ptr = VosI_Globals.simi_sequential_context_ptr; \
		op_sv_ptr = ((lpw_mobility_state *)(tcontext_ptr->mod_state_ptr));
#else
#  define FIN_PREAMBLE_DEC	lpw_mobility_state *op_sv_ptr;
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
	void lpw_mobility (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Obtype lpw_mobility_init (int * init_block_ptr);
	VosT_Address lpw_mobility_alloc (VOS_THREAD_INDEX_ARG_COMMA VosT_Obtype, int);
	void lpw_mobility_diag (OP_SIM_CONTEXT_ARG_OPT);
	void lpw_mobility_terminate (OP_SIM_CONTEXT_ARG_OPT);
	void lpw_mobility_svar (void *, const char *, void **);


	VosT_Fun_Status Vos_Define_Object (VosT_Obtype * obst_ptr, const char * name, unsigned int size, unsigned int init_obs, unsigned int inc_obs);
	VosT_Address Vos_Alloc_Object_MT (VOS_THREAD_INDEX_ARG_COMMA VosT_Obtype ob_hndl);
	VosT_Fun_Status Vos_Poolmem_Dealloc_MT (VOS_THREAD_INDEX_ARG_COMMA VosT_Address ob_ptr);
#if defined (__cplusplus)
} /* end of 'extern "C"' */
#endif




/* Process model interrupt handling procedure */


void
lpw_mobility (OP_SIM_CONTEXT_ARG_OPT)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	FIN_MT (lpw_mobility ());
	if (1)
		{
		Evhandle evh;
		double	altitude,latitude,longitude,
				x_pos,y_pos,z_pos,
				x,y,z,rand, D, prev_angle;
		
				
		Compcode comp_code;
		


		FSM_ENTER ("lpw_mobility")

		FSM_BLOCK_SWITCH
			{
			/*---------------------------------------------------------*/
			/** state (init) enter executives **/
			FSM_STATE_ENTER_FORCED_NOLABEL (0, "init", "lpw_mobility [init enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_mobility [init enter execs], state0_enter_exec)
				{
				///////////////////////////////////////////////////////////////
				// BILLARD MOBILITY INIT STATE
				//
				// Initialize the billard mobility process model (variable,...)
				///////////////////////////////////////////////////////////////
				
				// init every id relative to the current process
				my_node_id=op_topo_parent (op_id_self());
				my_process_id=op_id_self();
				my_net_id=op_topo_parent(my_node_id);
				
				// read the simulation attributes
				op_ima_sim_attr_get(OPC_IMA_DOUBLE, "mobil_POS_TIMER", &POS_TIMER);
				op_ima_sim_attr_get(OPC_IMA_DOUBLE, "mobil_STEP_DIST", &STEP_DIST);
				op_ima_sim_attr_get(OPC_IMA_DOUBLE, "mobil_XMIN", &XMIN);
				op_ima_sim_attr_get(OPC_IMA_DOUBLE, "mobil_XMAX", &XMAX);
				op_ima_sim_attr_get(OPC_IMA_DOUBLE, "mobil_YMIN", &YMIN);
				op_ima_sim_attr_get(OPC_IMA_DOUBLE, "mobil_YMAX", &YMAX);
				
				// initialize the distributions for the random movements
				one=op_dist_load ("uniform",-1,1);
				
				// initilaze the first random direction followed by the node containing the current mobility process
				angle=op_dist_outcome (one)*PI;
				
				// schedule the first "mouvement" interuption
				op_intrpt_schedule_self(op_sim_time()+POS_TIMER,0);
				}

				FSM_PROFILE_SECTION_OUT (lpw_mobility [init enter execs], state0_enter_exec)

			/** state (init) exit executives **/
			FSM_STATE_EXIT_FORCED (0, "init", "lpw_mobility [init exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_mobility [init exit execs], state0_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_mobility [init exit execs], state0_exit_exec)


			/** state (init) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "init", "idle")
				/*---------------------------------------------------------*/



			/** state (idle) enter executives **/
			FSM_STATE_ENTER_UNFORCED (1, "idle", state1_enter_exec, "lpw_mobility [idle enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_mobility [idle enter execs], state1_enter_exec)
				{
				}

				FSM_PROFILE_SECTION_OUT (lpw_mobility [idle enter execs], state1_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (3,"lpw_mobility")


			/** state (idle) exit executives **/
			FSM_STATE_EXIT_UNFORCED (1, "idle", "lpw_mobility [idle exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_mobility [idle exit execs], state1_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_mobility [idle exit execs], state1_exit_exec)


			/** state (idle) transition processing **/
			FSM_PROFILE_SECTION_IN (lpw_mobility [idle trans conditions], state1_trans_conds)
			FSM_INIT_COND (MOVE)
			FSM_DFLT_COND
			FSM_TEST_LOGIC ("idle")
			FSM_PROFILE_SECTION_OUT (lpw_mobility [idle trans conditions], state1_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 2, state2_enter_exec, ;, "MOVE", "", "idle", "position")
				FSM_CASE_TRANSIT (1, 1, state1_enter_exec, ;, "default", "", "idle", "idle")
				}
				/*---------------------------------------------------------*/



			/** state (position) enter executives **/
			FSM_STATE_ENTER_FORCED (2, "position", state2_enter_exec, "lpw_mobility [position enter execs]")
				FSM_PROFILE_SECTION_IN (lpw_mobility [position enter execs], state2_enter_exec)
				{
				///////////////////////////////////////////////////////////////
				// BILLARD MOBILITY POSITION STATE
				//
				// Update the position of the node containing the current mobility 
				// process
				///////////////////////////////////////////////////////////////
				
				
				// Get the actual position of the node containing the current process
				op_ima_obj_attr_get(my_node_id,"x position",&x_pos);
				op_ima_obj_attr_get(my_node_id,"y position",&y_pos);
				
				// calcul the new x and y position according to the STEP_DIST and the current direction (angle)
				x=x_pos+STEP_DIST*cos(angle);
				y=y_pos+STEP_DIST*sin(angle);
				
				// if the new x position is outside the network grid boundaries
				if ((x<XMIN)||(XMAX<x))
					{
					// if the new y position is also outside the network grid boundaries
					if ((y<YMIN)||(YMAX<y))
						{
						// come back to the previous position (on x and y)
						x=x_pos-STEP_DIST*cos(angle);
						y=y_pos-STEP_DIST*sin(angle);
						// and calculate a random new direction
						angle=op_dist_outcome(one)*PI;	
						}
					// if new y position are still in the network boundaries 
					else 
						{
						// come back to the previous position (only on x)
						x=x_pos-STEP_DIST*cos(angle);
						// and calculate a random new direction
						angle=op_dist_outcome(one)*PI;
						}
					}
				
				// if only the new y position is outside (x and y outside handle before) the network boundaries 
				if ((y<YMIN)||(YMAX<y))
					{
					// come back to the previous position (only on y)
					y=y_pos-STEP_DIST*sin(angle);
					// and calculate a random new direction
					angle=op_dist_outcome(one)*PI;
					}
				
				printf(" my_id : %d previous pos (%f,%f), new position (%f,%f), direction %f\n",my_node_id,x_pos,y_pos,x,y,angle);
				
				// set the new position of the node
				op_ima_obj_attr_set(my_node_id, "x position",x);
				op_ima_obj_attr_set(my_node_id, "y position",y);
				
				// schedule the interuption for the next movement (position updating)
				op_intrpt_schedule_self(op_sim_time()+POS_TIMER,0);
				
				// breakpoint for debugging purpose
				if (1)
					{
					op_prg_odb_bkpt("position");
					}
				
				
				
				
				
				}

				FSM_PROFILE_SECTION_OUT (lpw_mobility [position enter execs], state2_enter_exec)

			/** state (position) exit executives **/
			FSM_STATE_EXIT_FORCED (2, "position", "lpw_mobility [position exit execs]")
				FSM_PROFILE_SECTION_IN (lpw_mobility [position exit execs], state2_exit_exec)
				{
				}
				FSM_PROFILE_SECTION_OUT (lpw_mobility [position exit execs], state2_exit_exec)


			/** state (position) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "position", "idle")
				/*---------------------------------------------------------*/



			}


		FSM_EXIT (0,"lpw_mobility")
		}
	}




void
lpw_mobility_diag (OP_SIM_CONTEXT_ARG_OPT)
	{
	/* No Diagnostic Block */
	}




void
lpw_mobility_terminate (OP_SIM_CONTEXT_ARG_OPT)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = __LINE__;
#endif

	FIN_MT (lpw_mobility_terminate ())

	Vos_Poolmem_Dealloc_MT (OP_SIM_CONTEXT_THREAD_INDEX_COMMA pr_state_ptr);

	FOUT
	}


/* Undefine shortcuts to state variables to avoid */
/* syntax error in direct access to fields of */
/* local variable prs_ptr in lpw_mobility_svar function. */
#undef one
#undef my_node_id
#undef my_process_id
#undef angle
#undef my_net_id

#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE

#define FIN_PREAMBLE_DEC
#define FIN_PREAMBLE_CODE

VosT_Obtype
lpw_mobility_init (int * init_block_ptr)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	VosT_Obtype obtype = OPC_NIL;
	FIN_MT (lpw_mobility_init (init_block_ptr))

	Vos_Define_Object (&obtype, "proc state vars (lpw_mobility)",
		sizeof (lpw_mobility_state), 0, 20);
	*init_block_ptr = 0;

	FRET (obtype)
	}

VosT_Address
lpw_mobility_alloc (VOS_THREAD_INDEX_ARG_COMMA VosT_Obtype obtype, int init_block)
	{

#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	lpw_mobility_state * ptr;
	FIN_MT (lpw_mobility_alloc (obtype))

	ptr = (lpw_mobility_state *)Vos_Alloc_Object_MT (VOS_THREAD_INDEX_COMMA obtype);
	if (ptr != OPC_NIL)
		ptr->_op_current_block = init_block;
	FRET ((VosT_Address)ptr)
	}



void
lpw_mobility_svar (void * gen_ptr, const char * var_name, void ** var_p_ptr)
	{
	lpw_mobility_state		*prs_ptr;

	FIN_MT (lpw_mobility_svar (gen_ptr, var_name, var_p_ptr))

	if (var_name == OPC_NIL)
		{
		*var_p_ptr = (void *)OPC_NIL;
		FOUT
		}
	prs_ptr = (lpw_mobility_state *)gen_ptr;

	if (strcmp ("one" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->one);
		FOUT
		}
	if (strcmp ("my_node_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_node_id);
		FOUT
		}
	if (strcmp ("my_process_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_process_id);
		FOUT
		}
	if (strcmp ("angle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->angle);
		FOUT
		}
	if (strcmp ("my_net_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_net_id);
		FOUT
		}
	*var_p_ptr = (void *)OPC_NIL;

	FOUT
	}

