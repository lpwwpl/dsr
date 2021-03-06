MIL_3_Tfile_Hdr_ 100A 94B modeler 40 49AD1CCC 49AD1CCC 1 lpw Administrator 0 0 none none 0 0 none 666EA8A3 1933 0 0 0 0 0 0 8f3 1                                                                                                                                                                                                                                                                                                                                                                                                             ����             0x0             NONE                                 2   <              
   SRC   
       
      YThe dsr address of the request packet (to which this packet reply) final destination node   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             1                �   <              
   DEST   
       
      VThe dsr address of the reply packet destination node (that is the request source node)   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             2                �   <              
   RELAY   
       
      3The dsr address of the reply packet next relay node   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             3               "   <              
   Seg_Left   
       
      ^The number of hops on which the reply packet must be forwarded before reaching its destination   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             4               r   <              
   
Size_Route   
       
      7The size of the route that the reply packet must follow   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             5               �   <              
   
Seq_number   
       
      DThe sequence number of the request packet to which this packet reply   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             6                       pk_field        <              
   Type   
       
      DThe type of the packet (that is REPLY_PACKET_TYPE for a data packet)   
          integer             signed, big endian          
   8   
       
   11   
       
   set   
          NONE             *����             7                       pk_field      2   Z              
   Node_0   
       
      �The first node dsr address of the request packet route (that is the dsr address of the request packet generator) to which this packet reply   9Thus it is the dsr address of the reply destination node.   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             8                �   Z              
   Node_1   
       
      hThe second node dsr address of the route that the request packet to which this packet reply has followed   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             9                �   Z              
   Node_2   
       
      gThe third node dsr address of the route that the request packet to which this packet reply has followed   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             10               "   Z              
   Node_3   
       
      hThe fourth node dsr address of the route that the request packet to which this packet reply has followed   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             11               r   Z              
   Node_4   
       
      gThe fifth node dsr address of the route that the request packet to which this packet reply has followed   
       
   integer   
          signed, big endian          
   8   
       ����             set             NONE             *����             12               �   Z              
   Node_5   
       
      gThe sixth node dsr address of the route that the request packet to which this packet reply has followed   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             13                  Z              
   Node_6   
       
      iThe seventh node dsr address of the route that the request packet to which this packet reply has followed   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             14               b   Z              
   Node_7   
       
      hThe eighth node dsr address of the route that the request packet to which this packet reply has followed   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             15                2   �              
   	TR_source   
       
      OThe objid of the last packet transmitor: used to compute the transmission range   
          integer             signed, big endian          
   16   
       ����             set             NONE             *����             16                       pk_field      �   �              
   Reply_From_Target   
       
      ]Flag equal to 1 if the reply packet was built by the request final destination (target)) node   x	 equal to 0 if the reply packet was built by an intermediate node that has compute the total route with its route cache   
       
   integer   
          signed, big endian          
   1   
       ����             set             NONE             *����             17             