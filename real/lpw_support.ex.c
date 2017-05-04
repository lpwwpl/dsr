#include "lpw_support.h"
#include <opnet.h>
int number_of_lpw_nodes=0;				// the number of lpw nodes
sFifo* ptr_fifo_test_addresses=OPC_NIL; // fifo structure used for checking and validation purpose
sFifo* ptr_fifo_addresses=OPC_NIL;		// fifo structure used to find the mac_address or the lpw_address of a node from its objid
Objid* nodes_objid_table;				// table used to find the objid of a node from its lpw_address
int validated=0;

void lpw_support_start() {
// if the initialization was not already done by another lpw nodes
if ((ptr_fifo_test_addresses==OPC_NIL) && (ptr_fifo_addresses==OPC_NIL))
	{
	// initialize the package global variables
	number_of_lpw_nodes=0;
	validated=0;
	ptr_fifo_test_addresses=fifo_new();
	ptr_fifo_addresses=fifo_new();
	// if the memory allocation failled during this initialisation the simulation is stopped
	if ((ptr_fifo_test_addresses==OPC_NIL)||(ptr_fifo_addresses==OPC_NIL))
		{
		op_sim_end("Lpw support initialization failure","","","");
		}
	}
}

int lpw_support_declare_node_addresses(Objid node_objid, int lpw_address, int mac_address) {
Objid* objidPtr=OPC_NIL;
sAddresses* addresses;

// test if the initialization of the lpw_support was done
if ((ptr_fifo_test_addresses==OPC_NIL)||(ptr_fifo_addresses==OPC_NIL))
	{
	op_sim_end("The lpw_support_start function was not call before using the package","","","");
	return(-1);
	}

// if every addresses has been already validated then the addresses declaration phase is finished
if (validated)
	{
	// and the simulation is stopped since no new node can be registered
	op_sim_end("No new lpw address can be added after having validated the whole lpw address","","","");
	return(-1);
	}

// address validy control 
if (lpw_address==LPW_SUPPORT_AUTOMATIC_ASSIGNATION)
	{
	// check if the futur node automatic address is already used
	objidPtr=fifo_multiplex_read(ptr_fifo_test_addresses,number_of_lpw_nodes);
	}
else
	{
	// check if the node lpw_address if already used
	objidPtr=fifo_multiplex_read(ptr_fifo_test_addresses,lpw_address);
	}
// if the lpw_address is "free" 
if (objidPtr==OPC_NIL)
	{
	objidPtr=(int*)op_prg_mem_alloc(sizeof(Objid));
	addresses=(sAddresses*)op_prg_mem_alloc(sizeof(sAddresses));
	if (lpw_address==LPW_SUPPORT_AUTOMATIC_ASSIGNATION)
		{
		// assign an automatic lpw address to the node
		lpw_address=number_of_lpw_nodes;
		*objidPtr=node_objid;
		}
	else
		{
		// assign the given address to the node
		*objidPtr=node_objid;
		}
	// valid the new lpw address associated with the mac address and node_objid by storing their values in memory
	addresses->lpw_address=lpw_address;
	addresses->mac_address=mac_address;
	fifo_multiplex_insert(ptr_fifo_addresses,addresses,node_objid);
	// store also the lpw address in another fifo for checking and validation purpose
	fifo_multiplex_insert(ptr_fifo_test_addresses,objidPtr,lpw_address);
	}
// if the lpw_address is busy => error
else
	{
	if (lpw_address==LPW_SUPPORT_AUTOMATIC_ASSIGNATION)
		{
		op_sim_end("Multiple uses of the same lpw address due to the use of both manual and automatic address assignations","","","");
		}
	else
		{
		printf("try to use the invalid address %d", lpw_address);
		op_sim_end("Multiple uses of the same lpw address","","","");
		}
	return(-1);
	}

// one more lpw node in the network
number_of_lpw_nodes++;
return(lpw_address);
}
	
int lpw_support_validate_addresses()
{
Objid* objidPtr;
int lpw_address=0;
// if the lpw addresses were not validated before
if (!validated)
	{
	// create the table of the nodes objid
	nodes_objid_table=(Objid*)op_prg_mem_alloc(number_of_lpw_nodes*sizeof(Objid));
	// check that every addresses are valids and fill in the objid table (indexed by the lpw_address)
	objidPtr=fifo_multiplex_extract(ptr_fifo_test_addresses,lpw_address);
	while (objidPtr!=OPC_NIL)
		{
		nodes_objid_table[lpw_address]=*objidPtr;
		op_prg_mem_free(objidPtr);
		lpw_address++;
		objidPtr=fifo_multiplex_extract(ptr_fifo_test_addresses,lpw_address);
		}
	// and destroy the fifo used only for lpw addresses checking and validation
	fifo_destroy(ptr_fifo_test_addresses);
	ptr_fifo_test_addresses=OPC_NIL;
	// if at least one address is invalid the simulation is stopped and an error message displayed
	if (lpw_address!=number_of_lpw_nodes)
		{
		op_sim_end("Lpw addresses do not follow the rule: every lpw addresses must be in the range [0,number of nodes-1]","","","");
		return(-1);
		}
	// otherwise the centralized structures containing every lpw addresses, mac addresses and node objid are validated
	else
		{
		printf("Lpw Addresses validated: the centralized structure containing every lpw and mac addresses can be used\n");
		printf("There are %d node(s) using lpw in the network\n",number_of_lpw_nodes);
		validated=1;
		return(0);
		}
	}
return(0);
}

int lpw_support_number_of_nodes()
{
// if every address has been validated
if (validated)
	{
	// return the number of node using the lpw process model
	return number_of_lpw_nodes;
	}
// otherwise the addresses declaration phase is not finised 
else
	{
	// and the simulation is stopped since other nodes can still declare their addresses
	op_sim_end("All the lpw address must be declare and validated before calling this function","","","");
	}
}

Objid lpw_support_get_node_objid(int lpw_address)
{
// if every address was validated
if (validated)
	{
	// if the lpw address is known
	if (lpw_address<number_of_lpw_nodes)
		{
		// return the objid of the node using this lpw address
		return(nodes_objid_table[lpw_address]);
		}
	// otherwise the node lpw addresss is unknown by the lpw support package
	else
		{
		// thus the simulation is stopped
		op_sim_end("This lpw address is unknown by the lpw support","","","");
		return(-1);
		}
	}
// otherwise if the addresses was not validated before
else
	{
	// the simulation is stopped since other nodes can still declare their addresses
	op_sim_end("All the lpw address must be declare and validated before calling this function","","","");
	return(-1);
	}
}

int lpw_support_get_mac_from_lpw_address(int lpw_address)
{
sAddresses* addresses;
// if every address was validated
if (validated)
	{
	// if the lpw address is known
	if (lpw_address<number_of_lpw_nodes)
		{
		// read and return the mac address of the node using this lpw address
		addresses=fifo_multiplex_read(ptr_fifo_addresses,nodes_objid_table[lpw_address]);
		if (addresses!=OPC_NIL)
			{
			return(addresses->mac_address);
			}
		else
			{
			op_sim_end("lpw support amazing error: a declared node a diseapered from the memory !!","","","");
			return(-1);
			}
		}
	// otherwise the node lpw addresss is unknown by the lpw support package
	else
		{
		// thus the simulation is stopped
		op_sim_end("This lpw address is unknown by the lpw support","","","");
		return(-1);
		}
	}
// otherwise if the addresses was not validated before
else
	{
	// the simulation is stopped since other nodes can still declare their addresses
	op_sim_end("All the lpw address must be declare and validated before calling this function","","","");
	return(-1);
	}
}

int lpw_support_get_mac_address(Objid node_objid)
{
sAddresses* addresses;
// if every address was validated
if (validated)
	{
	// read the mac address of the node identified by the given objid
	addresses=fifo_multiplex_read(ptr_fifo_addresses,node_objid);
	// if this value was found
	if (addresses!=OPC_NIL)
		{
		// this wanted mac address is returned
		return(addresses->mac_address);
		}
	// otherwise the Objid is unknown by the lpw support package
	else
		{
		// thus the simulation is stopped
		op_sim_end("This node objid is unknown by the lpw support","","","");
		return(-1);
		}
	}
// otherwise if the addresses was not validated before
else
	{
	// the simulation is stopped since other nodes can still declare their addresses
	op_sim_end("All the lpw address must be declare and validated before calling this function","","","");
	return(-1);
	}
}

int lpw_support_get_lpw_address(Objid node_objid)
{
sAddresses* addresses;
// if every address was validated
if (validated)
	{
	// read the lpw address of the node identified by the given objid
	addresses=fifo_multiplex_read(ptr_fifo_addresses,node_objid);
	// if this value was found
	if (addresses!=OPC_NIL)
		{
		// this wanted lpw address is returned
		return(addresses->lpw_address);
		}
	// otherwise the Objid is unknown by the lpw support package
	else
		{
		// thus the simulation is stopped
		op_sim_end("This node objid is unknown by the lpw support","","","");
		return(-1);
		}
	}
// otherwise if the addresses was not validated before
else
	{
	// the simulation is stopped since other nodes can still declare their addresses
	op_sim_end("All the lpw address must be declare and validated before calling this function","","","");
	return(-1);
	}
}



void lpw_support_end()
{
// if the lpw addresses checking and validation structure was not destroyed yet (ex: error during pre init phase)
if (ptr_fifo_test_addresses!=OPC_NIL)
	{
	// free the memory used by this dynamic fifo structure
	fifo_destroy(ptr_fifo_test_addresses);
	ptr_fifo_test_addresses=OPC_NIL;
	}

// if the closure was not already done by another lpw nodes
if ((ptr_fifo_addresses!=OPC_NIL)||(nodes_objid_table!=OPC_NIL))
	{
	// free the memory used by the dynamic addresses structure and reintialize the global variables
	if (ptr_fifo_addresses!=OPC_NIL) fifo_destroy(ptr_fifo_addresses);
	if (nodes_objid_table!=OPC_NIL) op_prg_mem_free(nodes_objid_table);
	ptr_fifo_addresses=OPC_NIL;
	validated=0;
	number_of_lpw_nodes=0;
	}
}




