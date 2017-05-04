#ifndef LPW_SUPPORT
#define LPW_SUPPORT

///////////////////////////////////////////////////////////////////////////
///////////////////////////////// INCLUDE /////////////////////////////
////
///////////////////////////////////////////////////////////////////////////

#include <opnet.h>
#include "fifo.h"

///////////////////////////////////////////////////////////////////////////
///////////////////////////// TYPE DEFINITION /////////////////////////////
///////////////////////////////////////////////////////////////////////////

// constant definition
#define LPW_SUPPORT_USE_THE_MAC_ADDRESS -1
#define LPW_SUPPORT_AUTOMATIC_ASSIGNATION -2

// structure used to store the addresses of a node
typedef struct
	{
	int lpw_address;	// the lpw address of the node
	int mac_address;	// the mac address of the node	
	} sAddresses;

///////////////////////////////////////////////////////////////////////////
///////////////////////////// FUNCTIONS HEADER ////////////////////////////
///////////////////////////////////////////////////////////////////////////

// to start the lpw support package
  extern void lpw_support_start();

// to declare a new node using the lpw model to the lpw support package
  extern int lpw_support_declare_node_addresses(Objid node_objid, int lpw_address, int mac_address);

// to validate (check the validity) all the lpw addresses used in the network
  extern int lpw_support_validate_addresses();

// to know the number of nodes using the lpw model in the network
  extern int lpw_support_number_of_nodes();

// to get the objid of a node from its lpw address
  extern int lpw_support_get_node_objid(int lpw_address);

// to get the mac address of a node from its lpw address
  extern int lpw_support_get_mac_from_lpw_address(int lpw_address);

// to get the mac address of a node from its objid
  extern int lpw_support_get_mac_address(Objid node_objid);

// to get the lpw address of a node from its objid
  extern int lpw_support_get_lpw_address(Objid node_objid);

// to close the lpw support
  extern void lpw_support_end();

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


#endif
