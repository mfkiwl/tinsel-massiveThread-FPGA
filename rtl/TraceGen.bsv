package TraceGen;

// ============================================================================
// Imports
// ============================================================================

import Mem     :: *;
import FIFOF   :: *;
import DCache  :: *;
import DRAM    :: *;
import Vector  :: *;
import RegFile :: *;
import Assert  :: *;

// Interface to C functions
import "BDPI" function ActionValue#(Bit#(32)) getUInt32();

// ============================================================================
// Constants
// ============================================================================

// Operator encoding
Bit#(32) opEND   = 0;  // End of operation stream
Bit#(32) opDELAY = 1;  // Delay for some number of cycles
Bit#(32) opLW    = 2;  // Load word
Bit#(32) opSW    = 3;  // Store word

// ============================================================================
// Types
// ============================================================================

// TraceGen request
// (Fields read straight from stdin are all 32 bits)
typedef struct {
  Bit#(32) op;
  Bit#(32) threadId;
  Bit#(32) addr;
  Bit#(32) data;
  Bit#(32) delay;
} TraceGenReq deriving (Bits);

// ============================================================================
// Implementation
// ============================================================================

module traceGen ();
  // State
  // -----

  // DRAM instance
  Mem dram <- mkDRAM;

  // Data cache instance (id 0) connected directly to DRAM instance
  DCache dcache <- mkDCache(0, dram);

  // Record in-flight requests (max of one outstanding request per thread)
  RegFile#(DCacheClientId, TraceGenReq) inFlight <-
    mkRegFile(minBound, maxBound);
  Vector#(TExp#(SizeOf#(DCacheClientId)), Reg#(Bool)) inFlightValid <-
    replicateM(mkReg(False));

  // Raw requests from stdin
  FIFOF#(TraceGenReq) rawReqs <- mkFIFOF;

  // Goes high when the opEND operation is encountered
  Reg#(Bool) allReqsGathered <- mkReg(False);

  // Countdown timer
  Reg#(Bit#(32)) countdown <- mkReg(0);

  // Constants
  // ---------

  Integer maxThreadId = 2**valueOf(SizeOf#(DCacheClientId))-1;

  // Rules
  // -----

  // Read raw requests from stdin and enqueue to rawReqs
  rule gatherRequests (! allReqsGathered);
    TraceGenReq req = ?;
    let op <- getUInt32();
    req.op = op;
    if (op == opEND)
      allReqsGathered <= True;
    else if (op == opDELAY) begin
      let delay <- getUInt32();
      req.delay = delay;
      rawReqs.enq(req);
    end else begin
      let threadId <- getUInt32();
      dynamicAssert(threadId <= fromInteger(maxThreadId),
                      "TraceGen.bsv: thread id too large");
      req.threadId = threadId;
      let addr <- getUInt32();
      req.addr = addr;
      if (op == opSW) begin
        let data <- getUInt32();
        req.data = data;
      end
      rawReqs.enq(req);
    end
  endrule

  // Both the request and responses handlers update the in-flight
  // valid bits, but they never both assign to the same bit
  (* mutually_exclusive = "issueRequests, receiveResponses" *)

  // Issue requests to data cache
  rule issueRequests (countdown == 0);
    TraceGenReq rawReq = rawReqs.first;
    if (rawReq.op == opDELAY) begin
      rawReqs.deq;
      countdown <= rawReq.delay;
    end else begin
      DCacheClientId id = truncate(rawReq.threadId);
      if (!inFlightValid[id] && dcache.canPut) begin
        rawReqs.deq;
        DCacheReq req = ?;
        req.cmd.isLoad  = rawReq.op == opLW ? True : False;
        req.cmd.isStore = rawReq.op == opSW ? True : False;
        req.addr = rawReq.addr;
        req.data = rawReq.data;
        req.byteEn = -1;
        dcache.putReq(req);
        inFlightValid[id] <= True;
        inFlight.upd(id, rawReq);
      end
    end
  endrule

  // Receive responses from data cache
  rule receiveResponses (dcache.canGet);
    DCacheResp resp <- dcache.getResp;
    DCacheClientId id = resp.id;
    TraceGenReq req = inFlight.sub(id);
    dynamicAssert(inFlightValid[id],
                    "TraceGen.bsv: response has no associated request");
    if (req.op == opLW)
      $display("%d: M[%d] == %d", id, req.addr, resp.data);
    else if (req.op == opSW)
      $display("%d: M[%d] := %d", id, req.data);
    inFlightValid[id] <= False;
  endrule

  // Delay for some number of cycles
  rule countdownTimer (countdown != 0);
    countdown <= countdown-1;
  endrule

  // Termination condition
  rule terminate ( allReqsGathered
                && !any(id, readVReg(inFlightValid))
                && !rawReqs.notEmpty );
    $finish(0);
  endrule
endmodule

endpackage