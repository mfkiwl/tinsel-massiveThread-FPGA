// Copyright (c) Matthew Naylor

package Socket;

// Access named sockets on the file system in simulation.

import Vector :: *;

// Socket identifier
typedef Bit#(32) SocketId;

// Socket ids used in tinsel
SocketId uartSocket  = 0;
SocketId northSocket = 1;
SocketId southSocket = 2;
SocketId eastSocket  = 3;
SocketId westSocket  = 4;
SocketId pcieSocket  = 5;

`ifdef SIMULATE

// Imports from C
// --------------

import "BDPI" function ActionValue#(Bit#(32)) socketGet8(SocketId id);

import "BDPI" function ActionValue#(Bool) socketPut8(SocketId id, Bit#(8) b);

import "BDPI" function ActionValue#(Bit#(n))
  socketGetN(SocketId id, Bit#(32) nbytes);

import "BDPI" function ActionValue#(Bool)
  socketPutN(SocketId id, Bit#(32) nbytes, Bit#(n) b);

// Wrappers
// --------

function ActionValue#(Maybe#(Vector#(n, Bit#(8)))) socketGet(SocketId id) =
  actionvalue
    Bit#(TMul#(TAdd#(n, 1), 8)) tmp <-
      socketGetN(id, fromInteger(valueOf(n)));
    Vector#(TAdd#(n, 1), Bit#(8)) res = unpack(tmp);
    if (res[valueOf(n)] == 0)
      return Valid(init(res));
    else
      return Invalid;
  endactionvalue;
  
function ActionValue#(Bool) socketPut(SocketId id, Vector#(n, Bit#(8)) data) =
  socketPutN(id, fromInteger(valueOf(n)), pack(data));

`endif
endpackage