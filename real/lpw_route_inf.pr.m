MIL_3_Tfile_Hdr_ 100A 94E modeler 9 49AD2B6C 49B0BFFD 62 lpw Administrator 0 0 none none 0 0 none 5EE20A02 2ACB 0 0 0 0 0 0 8f3 1                                                                                                                                                                                                                                                                                                                                                                                               ��g�      @   �   �  �  �  l  (�  (�  (�  (�  (�  (�  (�  `      Transmit    �������    ����             Yes          ����          ����         Yes      ����      No       ����              	   begsim intrpt         
   ����   
   doc file            	nd_module      endsim intrpt             ����      failure intrpts            disabled      intrpt interval         ԲI�%��}����      priority              ����      recovery intrpts            disabled      subqueue                     count    ���   
   ����   
      list   	���   
          
      super priority             ����             =* distribution for a random packet destination attribution */   Distribution *	\address_dist;       N/* the dsr address of the node containing the current dsr interface process */   int	\my_lpw_address;       4/* the objid of the current dsr interface process */   Objid	\my_objid;       H/* the objid of the node containing the current dsr interface process */   Objid	\my_node_objid;       K/* the objid of the network containing the current dsr interface process */   Objid	\my_network_objid;       �/* the user parameter which say if the node containing the current dsr interface process must transmit new data packets or not */   int	\TRANSMIT;              Packet* pk_ptr;   int destination_lpw_address;   Ici* iciptr;   Ici* ici_dest_ptr;   double packet_delay;   char* theData;   int number_of_nodes;   myudp* myudp_pk_ptr;   $   #include "lpw_support.h"   //#include "fifo.h"   #define FROM_SRC_STRM 1   #define TO_SINK_STRM 1   !#define FROM_ROUTING_LAYER_STRM 0   #define TO_ROUTING_LAYER_STRM 0       ^#define SRC_ARRIVAL (op_intrpt_type() == OPC_INTRPT_STRM && op_intrpt_strm() == FROM_SRC_STRM)   h#define RCV_ARRIVAL (op_intrpt_type() == OPC_INTRPT_STRM && op_intrpt_strm() == FROM_ROUTING_LAYER_STRM)       int number_of_nodes;   #define FENCE 0xCCDEADCC           typedef struct {   	int source;   
	int dest;   	Packet* data;   	int checksum;   
	int size;   	int fencehead;   	int fencefoot;   } myudp;       +/* encode and decode , now i don't work it.   Aint encodeAndDecode(char* source,char* dest,char* key,char type);   %int is_InputKeyRight(char *inputkey);   void InitS(unsigned char *s);   ,void InitT(unsigned char *t,char *inputkey);   /void Swap(unsigned char *s,int first,int last);   1void InitPofS(unsigned char *s,unsigned char *t);   -int Crypt(char *source,char *dest,char *key);   */   'void route2Application(Packet* pk_ptr);   <void application2Route(Packet* pk_ptr,int lpw_dest_address);       �   !void destroy_packet(Packet* pk) {   	op_pk_destroy(pk);   }       ,void route2Application(Packet* my_udp_ptr) {       	int checksum;   
	int size;   
	int* ptr;   	int i;   	int j;   	int fencehead;   	int fencefoot;   	Packet* pk_ptr;   	int theChecksum;   	   	checksum = 0;   	   -	ptr = (int*)op_prg_mem_alloc(4*sizeof(int));   -	pk_ptr = op_pk_create_fmt("Lpw_Upper_Data");       3	op_pk_nfd_get(my_udp_ptr,"checksum",&theChecksum);   (	op_pk_nfd_get(my_udp_ptr,"size",&size);   2	op_pk_nfd_get(my_udp_ptr,"fencehead",&fencehead);   2	op_pk_nfd_get(my_udp_ptr,"fencefoot",&fencefoot);   	   //  *(ptr) = checksum;   	*(ptr + 1) = size;   	*(ptr + 2) = fencehead;   	   "	printf("%d,%d\n",size,fencehead);   	   L	//caculate the checksum by counting the amount of "1" in size and fencehead   	for (i = 0; i < 2; i++) {	   		for (j = 0; j < 32; j++) {		   5			checksum = checksum + ((*(ptr + 1 + i) >> j) & 1);   		}   	}	   	   	//check the data    	if(checksum != theChecksum) {   :		printf("transmit data ererr.some data maybe changed\n");    	} else if(fencehead != FENCE) {   :		printf("transmit data ererr.some data maybe changed\n");    	} else if(fencefoot != FENCE) {   :		printf("transmit data ererr.some data maybe changed\n");   	}           )	op_pk_nfd_get(my_udp_ptr,"data",pk_ptr);   !	op_pk_send(pk_ptr,TO_SINK_STRM);   }       <void application2Route(Packet* pk_ptr,int lpw_dest_address){       	int totalSize;   	int size;	   	int fencehead;   	int fencefoot;   	int checksum;   	int i;   	int j;   	int* start;   	Packet* pk;       %	size = op_pk_total_size_get(pk_ptr);   5	start  = (int*)op_prg_mem_alloc(4*sizeof(int)+size);   	   	checksum = 0;	   	fencehead = 0xCCDEADCC;	   	fencefoot = 0xCCDEADCC;		   	*(start+2) = fencehead;   	*(start+1) = size;   "	printf("%d,%d\n",size,fencehead);   	   L	//caculate the checksum by counting the amount of "1" in size and fencehead   	for (i = 0; i < 2; i++) {	   		for (j = 0; j < 32; j++) {		   7			checksum = checksum + ((*(start + 1 + i) >> j) & 1);   		}   	}	   	   	*(start) = checksum;   .	*(int *)((char *)start+size+12) = fencefoot;	       	printf("%d\n",checksum);   	   %	pk=op_pk_create_fmt("APP_TO_ROUTE");   +	op_pk_nfd_set(pk,"source",my_lpw_address);   +	op_pk_nfd_set(pk,"dest",lpw_dest_address);   !	op_pk_nfd_set(pk,"data",pk_ptr);   '	op_pk_nfd_set(pk,"checksum",checksum);   	op_pk_nfd_set(pk,"size",size);   )	op_pk_nfd_set(pk,"fencehead",fencehead);   )	op_pk_nfd_set(pk,"fencefoot",fencefoot);   	   &	op_pk_send(pk,TO_ROUTING_LAYER_STRM);   }       /* Encode and Decode    	DSA algorithm   */   /*   Bint encodeAndDecode(char* source,char* dest,char* key,char type) {   	int size=0;   	if (type=='e'||type=='E'){       #		if((size=Crypt(source,dest,key)))   			return size;   		else   			return -1;   	}   	   	if (type=='d'||type=='D') {   #		if((size=Crypt(source,dest,key)))   			return size;   		else   			return -1;       }   	   	return -1;   }       &int is_InputKeyRight(char *inputkey) {       4	if(strlen(inputkey) <= 256 && strlen(inputkey) > 0)   		return 1;   	else   		return 0;   }       void InitS(unsigned char *s) {   	int i;   	for(i=0;i<256;i++)   			s[i]=i;   }       -void InitT(unsigned char *t,char *inputkey) {   	int i;   	for(i=0;i<256;i++) {   $		t[i]=inputkey[i%strlen(inputkey)];   	}   }       0void Swap(unsigned char *s,int first,int last) {   	unsigned char temp;   	temp=s[first];   	s[first]=s[last];   	s[last]=temp;   }       2void InitPofS(unsigned char *s,unsigned char *t) {   	int i;   		int j=0;   	   	for(i=0;i<256;i++) {   		j=(j+s[i]+t[i])%256;   		Swap(s,i,j);   	}   }       .int Crypt(char *source,char *dest,char *key) {   	unsigned char s[256]={0};   	unsigned char t[256]={0};   		int buf;       		int i=0;   		int j=0;   		int k=0;   
	int temp;   
	int size;   	int counter=0;   	int last=0;       	size=strlen(source);   
	InitS(s);   	InitT(t,key);   	InitPofS(s,t);   	buf=source[0];       	while(1) {   		if(buf=='\0'){break;}   		i=(i+1)%256;   		j=(j+s[i])%256;   		Swap(s,i,j);   		temp=(s[i]+s[j])%256;   		k=s[temp];   		dest[counter] = k^buf;   		counter++;   		buf=source[counter];   	}   	   	last=counter+1;   	dest[last] = '\0';       	return counter;   }   */                                  Z   �          
   pre   
       
      )op_intrpt_schedule_self(op_sim_time(),0);   
                         ����             pr_state         �   �          
   init   
       
   	   my_objid = op_id_self();   )my_node_objid = op_topo_parent(my_objid);   1my_network_objid = op_topo_parent(my_node_objid);       <my_lpw_address = lpw_support_get_lpw_address(my_node_objid);   0number_of_nodes = lpw_support_number_of_nodes();       3op_ima_obj_attr_get(my_objid,"Transmit",&TRANSMIT);   Aaddress_dist = op_dist_load("uniform_int",0,(number_of_nodes-1));   
                     
   ����   
          pr_state        �   �          
   idle   
                                       ����             pr_state        �  �          
   tx   
       
          "pk_ptr = op_pk_get(FROM_SRC_STRM);       if(TRANSMIT) {       "//	ici_dest_ptr = op_intrpt_ici();   O//	op_ici_attr_get(ici_dest_ptr,"Packet_Destination",&destination_lpw_address);   	   U	// the destination address of data packet is random(not including self node address)   a    // destination address maybe design in the application layer,not in transport layer's concern   >	destination_lpw_address = (int)op_dist_outcome(address_dist);   3	while(destination_lpw_address == my_lpw_address) {   ?		destination_lpw_address = (int)op_dist_outcome(address_dist);   	}   (	iciptr = op_ici_create("Lpw_Dest_Ici");   F	op_ici_attr_set(iciptr,"Packet_Destination",destination_lpw_address);   	op_ici_install(iciptr);   	   3	application2Route(pk_ptr,destination_lpw_address);   } else {   	op_pk_destroy(pk_ptr);   }           
                     
   ����   
          pr_state        �             
   rx   
       
      2myudp_pk_ptr = op_pk_get(FROM_ROUTING_LAYER_STRM);   printf("$$$$$$$$$$$$$$$\n");   C//packet_delay = (op_sim_time() - op_pk_creation_time_get(pk_ptr));   "//op_pk_send(pk_ptr,TO_SINK_STRM);    route2Application(myudp_pk_ptr);   
                     
   ����   
          pr_state                     p       x   �  h  ;  �  �          
   tr_2   
       
   SRC_ARRIVAL   
       ����          
    ����   
          ����                       pr_transition              �  @     �  t  �    �   �          
   tr_3   
       
����   
       ����          
    ����   
          ����                       pr_transition              .   �      �   �  r   �          
   tr_4   
       ����          ����          
    ����   
          ����                       pr_transition                �   �      h   �   �   �          
   tr_5   
       ����          ����          
    ����   
          ����                       pr_transition              m   �     t   �  e   ]  ~   '          
   tr_8   
       
   RCV_ARRIVAL   
       ����          
    ����   
          ����                       pr_transition      	        �   V     �   '  �   �  �   �          
   tr_9   
       ����          ����          
    ����   
          ����                       pr_transition      
        �   �     �   �     �  �   �  �   �  �   �          
   tr_10   
       
   default   
       ����          
    ����   
          ����                       pr_transition                                             