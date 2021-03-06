MIL_3_Tfile_Hdr_ 100A 94B modeler 40 49AD1CB9 49AD1CBA 1 lpw Administrator 0 0 none none 0 0 none 8CE3C6E8 1601 0 0 0 0 0 0 8f3 1                                                                                                                                                                                                                                                                                                                                                                                                             ����             0x0             NONE                                 (   <              
   SRC   
       
      1The dsr address of the request packet source node   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             1                x   <              
   DEST   
       
      ?The dsr address of the request packet destination (target) node   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             2                �   <              
   Seg_Left   
       
      EThe number of hops on which the request packet can be still forwarded   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             3                  <              
   
Size_Route   
       
      AThe actual size of the route that the request packet has followed   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             4               h   <              
   
Seq_number   
       
      )The sequence number of the request packet   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             5                       pk_field     �   <              
   Type   
       
      FThe type of the packet (that is REQUEST_PACKET_TYPE for a data packet)   
          integer             signed, big endian          
   8   
       
   7   
       
   set   
          NONE             *����             6                       pk_field        <              
   Creation_Time   
       
      FThe creation time of the request packet (for the life time management)   
       
   floating point   
          
big endian          
   16   
       ����             set             NONE             *����             7                       pk_field      (   Z              
   Node_0   
       
      pThe first node dsr address of the request packet route (that is the dsr address of the request packet generator)   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             8                x   Z              
   Node_1   
       
      MThe second node dsr address of the route that the request packet has followed   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             9                �   Z              
   Node_2   
       
      LThe third node dsr address of the route that the request packet has followed   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             10                  Z              
   Node_3   
       
      MThe fourth node dsr address of the route that the request packet has followed   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             11               h   Z              
   Node_4   
       
      LThe fifth node dsr address of the route that the request packet has followed   
       
   integer   
          signed, big endian          
   8   
       ����             set             NONE             *����             12               �   Z              
   Node_5   
       
      LThe sixth node dsr address of the route that the request packet has followed   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             13                  Z              
   Node_6   
       
      NThe seventh node dsr address of the route that the request packet has followed   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             14               X   Z              
   Node_7   
       
      NThe seventh node dsr address of the route that the request packet has followed   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             15                (   �              
   	TR_source   
       
      OThe objid of the last packet transmitor: used to compute the transmission range   
          integer             signed, big endian          
   16   
       ����             set             NONE             *����             16             