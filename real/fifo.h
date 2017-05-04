#ifndef FIFO
#define FIFO

///////////////////////////////////////////////////////////////////////////
///////////////////////////////// INCLUDE /////////////////////////////
////
///////////////////////////////////////////////////////////////////////////

#include <opnet.h>

///////////////////////////////////////////////////////////////////////////
///////////////////////////// TYPE DEFINITION /////////////////////////////
///////////////////////////////////////////////////////////////////////////

struct sObject;

// structure definition of the objects which are contained in the "FIFO"
typedef struct
	{
	void* data;
				// pointer to data of any types
	int fifoNumber;
 		// FIFO number for multiplexage
	struct sObject* next;	
// pointer to the next Object of the "FIFO"	
	} sObject;

// structure definition of the "FIFO"
typedef struct 
	{
	int nbrObjects;
			// number of objects contained in the "FIFO"
	sObject* firstObject;
 	// pointer to the first object of the "FIFO"
	sObject* lastObject;
	// pointer to the last object of the "FIFO"
	} sFifo;

///////////////////////////////////////////////////////////////////////////
///////////////////////////// FUNCTIONS HEADER ////////////////////////////
///////////////////////////////////////////////////////////////////////////

/// to init a fifo
  extern void fifo_init(sFifo *fifo);

/// to create and init a new dynamic fifo
  extern sFifo* fifo_new();

/// to add a data pointer at the end of a fifo
  extern int fifo_insert(sFifo* fifo, void* data)
;

/// to extract a data pointer from the beginning of the fifo
  extern void* fifo_extract(sFifo *fifo)
;

/// to read a data pointer located at the beginning of the fifo
  
extern void* fifo_read(sFifo *fifo)
;

/// to add a data pointer in a fifo using the multeplexing service
  
extern int fifo_multiplex_insert(sFifo* fifo,void* data,int fifo_number)
;

/// to extract a data pointer from a fifo using the multiplexing service
  
extern void* fifo_multiplex_extract(sFifo* fifo,int fifo_number)
;

/// to read a data pointer in a fifo using the multiplexing service
  
extern void* fifo_multiplex_read(sFifo* fifo,int fifo_number)
;

/// to extract a data pointer and its fifo number from the beginning of a fifo
 
/// <=> to extract the oldest data pointer (and its subqueue number) from the fifo, regardless the subqueue in which it is stored
  
extern void* fifo_multiplex_extract_first(sFifo *fifo, int *fifo_number)
;

/// to read a data pointer and its fifo number located at the beginning of a fifo
 
/// <=> to read the oldest data pointer (and its subqueue number) from the fifo, regardless the subqueue in which it is stored
  
extern void* fifo_multiplex_read_first(sFifo *fifo, int *fifo_number)
;

/// to destroy a dynamic fifo
  extern void fifo_destroy(sFifo* fifo);

/// to print the fifo on the screen
  
extern void fifo_print(sFifo fifo);

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

#endif
