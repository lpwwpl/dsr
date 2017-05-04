///////////////////////////////////////////////////////////////////////////
///////////////// OPNET'S COMPLEX_INTRPT LIBRARY HEADER FILE //////////////
///////////////////////////////////////////////////////////////////////////
/// 
///	Contains:	The complex_intrpt library source code, which provides the 
///  			equivalent of a multiple_ici (one intrpt = one ici) service
///
///	Company:	National Institute of Standards and Technology
/// Written by:	Xavier Pallot
///	Date: 		10/20/00
///
///////////////////////////////////////////////////////////////////////////
///	Description:	This library provides the multi-ici service for Opnet.
///					With it you can associate with any intrpt a pointer,
///					wich means any kind of information. You have not the
///					OPNET limitation: one outgoing ici per process.
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

#ifndef COMPLEX_INTRPT
#define COMPLEX_INTRPT

///////////////////////////////////////////////////////////////////////////
///////////////////////////////// INCLUDE /////////////////////////////
////
///////////////////////////////////////////////////////////////////////////

#include "fifo.h"
#include <opnet.h>

///////////////////////////////////////////////////////////////////////////
///////////////////////////// TYPE DEFINITION /////////////////////////////
///////////////////////////////////////////////////////////////////////////

// definition of the structure used to store the information associated with one complex interruption
typedef struct
	{
	int intrpt_code;
		// the code that the user has associated with his interruption
	void* intrpt_data;
		// a pointer to the data (of any type) that the user has associated with his interruption
	} sComplexIntrptObject
;
	
// definition of the structure used to manage the complex_intrpt library
typedef struct 
	{
	int intrpt_id_number;
		// the id that will be attributed and that will identifie the next complex interruption
	sFifo* intrpt_data_fifo;	// the queue (fifo) in which all the information about every scheduled complex interruption are stored
	} sComplexIntrpt;


///////////////////////////////////////////////////////////////////////////
//////////////////////////// FUNCTION HEADERS /////////////////////////////
///////////////////////////////////////////////////////////////////////////

// to create a dynamic management structure allowing the use of the complex interruptions
 extern sComplexIntrpt* complex_intrpt_new()
;

// to inititialize a management structure allowing the use if the complex interruptions
 extern int complex_intrpt_init(sComplexIntrpt* complex_intrpt);

// to schedule a self complex interruption
extern Evhandle complex_intrpt_schedule_self(sComplexIntrpt * complex_intrpt, double time, int intrpt_code, void* intrpt_data)
;

// to schedule a remote complex interruption
 extern Evhandle complex_intrpt_schedule_remote(sComplexIntrpt * complex_intrpt, double time, int intrpt_code, void* intrpt_data, Objid process_objid);

// to get the code of the current complex interruption
 extern int complex_intrpt_code(sComplexIntrpt * complex_intrpt)
;

// to get the data associated with the current complex interruption
 extern void* complex_intrpt_data(sComplexIntrpt * complex_intrpt)
;

// to cancel a previously scheduled complex interruption
 extern int complex_intrpt_cancel(sComplexIntrpt * complex_intrpt, Evhandle evt)
;

// to close the current interruption
 extern void complex_intrpt_close(sComplexIntrpt * complex_intrpt)
;

// to the dynamic structure allowing the management of the complex interruptions
 extern void complex_intrpt_destroy(sComplexIntrpt* complex_intrpt);

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

#endif
