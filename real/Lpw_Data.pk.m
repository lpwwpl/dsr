MIL_3_Tfile_Hdr_ 100A 94B modeler 40 49AD1C90 49AD1C90 1 lpw Administrator 0 0 none none 0 0 none CAA41C36 1787 0 0 0 0 0 0 8f3 1                                                                                                                                                                                                                                                                                                                                                                                                             ����             0x0             NONE                                 2   <              
   SRC   
       
      .The dsr address of the data packet source node   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             1                       pk_field      �   <              
   DEST   
       
      3The dsr address of the data packet destination node   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             2                       pk_field      �   <              
   RELAY   
       
      2The dsr address of the data packet next relay node   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             3                       pk_field     "   <              
   Seg_Left   
       
      ]The number of hops on which the data packet must be forwarded before reaching its destination   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             4                       pk_field     r   <              
   
Size_Route   
       
      6The size of the route that the data packet must follow   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             5                       pk_field     �   <              
   Type   
       
      CThe type of the packet (that is DATA_PACKET_TYPE for a data packet)   
          integer             signed, big endian          
   8   
       
   5   
          set             NONE             *����             6                2   Z              
   Node_0   
       
      gThe first node dsr address of the data packet route (that is the dsr address of the packet source node)   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             7                       pk_field      �   Z              
   Node_1   
       
      4The second node dsr address of the data packet route   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             8                       pk_field      �   Z              
   Node_2   
       
      3The third node dsr address of the data packet route   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             9                       pk_field     "   Z              
   Node_3   
       
      4The fourth node dsr address of the data packet route   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             10                       pk_field     r   Z              
   Node_4   
       
      3The fifth node dsr address of the data packet route   
       
   integer   
          signed, big endian          
   8   
       ����             set             NONE             *����             11                       pk_field     �   Z              
   Node_5   
       
      3The sixth node dsr address of the data packet route   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             12                       pk_field        Z              
   Node_6   
       
      5The seventh node dsr address of the data packet route   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             13                       pk_field     b   Z              
   Node_7   
       
      4The eighth node dsr address of the data packet route   
          integer             signed, big endian          
   8   
       ����             set             NONE             *����             14                       pk_field      2   �              
   data   
       
      The upper layer Data   
       
   packet   
          signed, big endian          
   	inherited   
       ����             unset             NONE             *����             15                       pk_field      2   �              
   	TR_source   
       
      OThe objid of the last packet transmitor: used to compute the transmission range   
       
   integer   
          signed, big endian          
   16   
       
   -1   
          set             NONE             *����             16                �   �              
   	packet_ID   
       
      ^The id of the data packet: used to ensure that a node processes only one time each data packet   
          integer             signed, big endian             32          
   0   
       
   set   
          NONE             *����             17                       pk_field   