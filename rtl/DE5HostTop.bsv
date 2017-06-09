// This module implements a "host board", i.e. an FPGA board that acts
// as an interface between a host PC (connected over PCIe) and an FPGA
// cluster (connected via a 10G link).
//
// The basic idea is that messages received from the host PC over PCIe
// are inserted onto the cluster's 10G network.  Likewise, messages
// from the cluster's network are sent to the host PC over PCIe.
//
// The format of the data stream in the PC->FPGA direction is:
//
//   1. DA: Destination address (4 bytes)
//   2. NM: Number of messages that follow minus one (4 bytes)
//   3. FM: Number of flit payloads per message minus one (1 byte)
//   4. Padding (7 bytes)
//   5. NM*FM flit payloads (NM*FM*BytesPerFlit bytes)
//   6. Goto step 1
//
// The format of the data stream in the FPGA->PC direction is simply
// raw flit payloads.
//
// This module assumes that BytesPerFlit is 16.  This restriction
// should be removed in future, if necessary.

package DE5HostTop;

// ============================================================================
// Imports
// ============================================================================

import Globals    :: *;
import DRAM       :: *;
import Interface  :: *;
import Queue      :: *;
import Vector     :: *;
import Mailbox    :: *;
import Network    :: *;
import Mac        :: *;
import PCIeStream :: *;
import Pipe       :: *;
import ConfigReg  :: *;

// ============================================================================
// Interface
// ============================================================================

`ifdef SIMULATE

typedef Empty DE5HostTop;

`else

interface DE5HostTop;
  // Interface to the PCIe BAR
  interface PCIeBAR controlBAR;
  // Interface to host PCIe bus
  // (Use for DMA to/from host memory)
  interface PCIeHostBus pcieHostBus;
  // Connection to FPGA cluster
  interface AvalonMac mac;
endinterface

`endif

// ============================================================================
// Implementation
// ============================================================================

module de5HostTop (DE5HostTop);

  // Ports
  OutPort#(Bit#(128)) toPCIe <- mkOutPort;
  InPort#(Bit#(128)) fromPCIe <- mkInPort;
  OutPort#(Flit) toLink <- mkOutPort;
  InPort#(Flit) fromLink <- mkInPort;

  // Create off-board link
  BoardLink link <- mkBoardLink(northPipe);

  // Connect ports to off-board link
  connectUsing(mkUGQueue, toLink.out, link.flitIn);
  connectUsing(mkUGQueue, link.flitOut, fromLink.in);

  // Create PCIeStream instance
  PCIeStream pcie <- mkPCIeStream;

  // Connect ports to PCIeStream
  connectUsing(mkUGQueue, toPCIe.out, pcie.streamIn);
  connectDirect(pcie.streamOut, fromPCIe.in);

  // Connect PCIe stream to 10G link
  // -------------------------------

  Reg#(Bit#(32)) fromPCIeDA    <- mkConfigRegU;
  Reg#(Bit#(32)) fromPCIeNM    <- mkConfigRegU;
  Reg#(Bit#(8))  fromPCIeFM    <- mkConfigRegU;
  Reg#(Bit#(1))  fromPCIeState <- mkConfigReg(0);

  Reg#(Bit#(32)) messageCount  <- mkConfigReg(0);
  Reg#(Bit#(8))  flitCount     <- mkConfigReg(0);

  rule pcieToLink0 (fromPCIeState == 0);
    if (fromPCIe.canGet) begin
      Bit#(128) data = fromPCIe.value;
      fromPCIeDA <= data[31:0];
      fromPCIeNM <= data[63:32];
      fromPCIeFM <= data[71:64];
      fromPCIeState <= 1;
      fromPCIe.get;
    end
  endrule

  rule pcieToLink1 (fromPCIeState == 1);
    if (fromPCIe.canGet && toLink.canPut) begin
      Flit flit;
      flit.dest = unpack(truncate(fromPCIeDA));
      flit.payload = fromPCIe.value;
      flit.notFinalFlit = True;
      if (flitCount == fromPCIeFM) begin
        flitCount <= 0;
        flit.notFinalFlit = False;
        if (messageCount == fromPCIeNM) begin
          messageCount <= 0;
          fromPCIeState <= 0;
        end else
          messageCount <= messageCount+1;
      end
      toLink.put(flit);
      fromPCIe.get;
    end
  endrule

  // Connect 10G link to PCIe stream
  rule linkToPCIe (fromLink.canGet && toPCIe.canPut);
    toPCIe.put(fromLink.value.payload);
    fromLink.get;
  endrule

  `ifndef SIMULATE
  interface controlBAR = pcie.external.controlBAR;
  interface pcieHostBus = pcie.external.hostBus;
  interface mac = link.avalonMac;
  `endif

endmodule

endpackage