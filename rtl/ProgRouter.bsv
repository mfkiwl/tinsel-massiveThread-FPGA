// SPDX-License-Identifier: BSD-2-Clause
// Functions, data types, and modules for programmable routers
package ProgRouter;

// =============================================================================
// Routing keys and beats
// =============================================================================

// A routing record is either 40 bits or 80 bits in size (aligned on a
// 40-bit or 80-bit boundary respectively). Multiple records are
// packed into a 256-bit DRAM beat (aligned on a 256-bit boundary).
// The most significant 16 bits of the beat contain a count of the
// number of records in the beat (in the range 1 to 6 inclusive). The
// remaining 240 bits contain records. The first record lies in the
// least-significant bits of the beat. The size portion of the routing
// key contains the number of contiguous DRAM beats holding all
// records for the key.

// 256-bit routing beat
typedef struct {
  // Number of 40-bit record chunks present
  Bit#(16) size;
  // The 40-bit record chunks
  Vector#(6, Bit#(40)) chunks;
} RoutingBeat deriving (Bits);

// Routing beat, tagged with the beat number in the DRAM burst
typedef struct {
  // Beat
  RoutingBeat beat;
  // Beat number
  Bit#(`BeatBurstWidth) beatNum;
} NumberedRoutingBeat deriving (Bits);

// 32-bit routing key
typedef struct {
  // Which off-chip RAM?
  Bit#(`LogDRAMsPerBoard) ram
  // Pointer to array of routing beats containing routing records
  Bit#(`LogBeatsPerDRAM) ptr;
  // Number of beats in the array
  Bit#(`LogRoutingEntryLen) numBeats;
} RoutingKey deriving (Bits);

// Extract routing key from an address
function RoutingKey getRoutingKey(NetAddr addr) =
  unpack(getRoutingKeyRaw(addr));

// =============================================================================
// Types of routing record
// =============================================================================

typedef enum {
  URM1 = 3'd0, // 40-bit Unicast Router-to-Mailbox
  URM2 = 3'd1, // 80-bit Unicast Router-to-Mailbox
  RR   = 3'd2, // 40-bit Router-to-Router
  MRM  = 3'd3, // 80-bit Multicast Router-to-Mailbox
  IND  = 3'd4  // 40-bit Indirection
} RoutingRecordTag;

typedef enum {
  NORTH = 2'd0,
  SOUTH = 2'd1,
  EAST  = 2'd2,
  WEST  = 2'd3,
} RoutingDir;

// 40-bit Unicast Router-to-Mailbox (URM1) record
typedef struct {
  // Record type
  RoutingRecordTag tag;
  // Mailbox destination
  Bit#(4) mbox;
  // Mailbox-local thread identifier
  Bit#(6) thread;
  // Local key. The first word of the message
  // payload is overwritten with this.
  Bit#(27) localKey;
} URM1Record deriving (Bits);

// 80-bit Unicast Router-to-Mailbox (URM2) record
typedef struct {
  // Record type
  RoutingRecordTag tag;
  // Mailbox destination
  Bit#(4) mbox;
  // Mailbox-local thread identifier
  Bit#(6) thread;
  // Currently unused
  Bit#(3) unused;
  // Local key. The first two words of the message
  // payload is overwritten with this.
  Bit#(64) localKey;
} URM2Record deriving (Bits);

// 40-bit Router-to-Router (RR) record
typedef struct {
  // Record type
  RoutingRecordTag tag;
  // Direction (N, S, E, or W)
  RoutingDir dir;
  // Currently unused
  Bit#(3) unused;
  // New 32-bit routing key that will replace the one in the
  // current message for the next hop of the message's journey
  Bit#(32) newKey;
} RRRecord deriving (Bits);

// 80-bit Multicast Router-to-Mailbox (MRM) record
typedef struct {
  // Record type
  RoutingRecordTag tag;
  // Mailbox destination
  Bit#(4) mbox;
  // Currently unused
  Bit#(9) unused;
  // Mailbox-local destination mask
  Bit#(64) destMask;
} MRMRecord deriving (Bits);

// 40-bit Indirection (IND) record:
typedef struct {
  // Record type
  RoutingRecordTag tag;
  // Currently unused
  Bit#(5) unused;
  // New 32-bit routing key for new set of records on current router
  Bit#(32) newKey;
} INDRecord deriving (Bits);

// It is sometimes convenient (though redundant) to record a routing
// decision for a flit internally within the programmable router
typedef struct {
  // Normal flit
  Flit flit;
  // Routing decision for flit
  RoutingDecision decision;
} RoutedFlit deriving (Bits);

// Routing decision
typedef enum {
  RouteNorth,
  RouteSouth,
  RouteEast,
  RouteWest,
  RouteNoC,
  RouteLoop
} RoutingDecision deriving (Bits, Eq);

// =============================================================================
// Design
// =============================================================================

// In the following diagram N/S/E/W are the inter-FPGA links and
// L0..L3 are links at one edge of the NoC.  Depending on the NoC
// dimensions, there may be more or less than four links on a single
// NoC edge, but the diagram assumes four.

//
//               N     S     E     W     L0..L3/Loop   Input flits
//               |     |     |     |     |       |
//             +---+ +---+ +---+ +---+ +---+     |
//             | F | | F | | F | | F | | F |     |     Fetchers
//             +---+ +---+ +---+ +---+ +---+     |
//               |     |     |     |     |       |
//             +---------------------------+     |
//             |          Crossbar         |     |     Routing
//             +---------------------------+     |
//               |     |     |     |     |       |
//              N/L0  S/L1  E/L2  W/L3   Ind-----+     Output queues
//               |     |     |     |
//             +---------------------------+
//             |          Expander         |           Final expansion
//             +---------------------------+
//               |  |  |  |  |  |  |  |
//               N  S  E  W  L0 L1 L2 L3               Output flits
//

// The core functionality is implemented in the fetchers, which:
//   (1) extract routing keys from incoming flits;
//   (2) lookup the keys in RAM;
//   (3) interpret the resulting routing records; and
//   (4) emit the interpreted flits.

// The key property of these fetchers is that they act entirely
// indepdedently of each other: each one can make progress even if
// another is blocked.  Unfortunately, this leads to a duplicated
// logic resources, but is necessary to avoid deadlock.

// Note that, as the routers are fully programmable, it is possible
// for the programmer to introduce deadlock using an ill-defined
// routing scheme, e.g. where a flit arrives in on (say) link N and
// requires a flit to be sent back along the same direction N.
// However, the hardware does guarantee deadlock-freedom if the
// routing scheme is based on dimension-ordered routing.

// After the fetchers have interpreted the flits, they are fed to a
// fair crossbar which organises them by destination into output
// queues.  To reduce logic, we allow each inter-board link to share
// an output queue with a local link, as this does not compromise
// forward progress.  Finally the queues are expanded to provide an
// output stream for each possible destination.

// =============================================================================
// Fetcher
// =============================================================================

// Flit address in a fetcher's flit buffer
typedef Bit#(`FetcherLogFlitBufferSize) FetcherFlitBufferAddr;

// Message address in a fetcher's flit buffer
typedef Bit#(`FetcherLogMsgsPerFlitBuffer) FetcherFlitBufferMsgAddr;

// This structure contains information about an in-flight memory
// request from a fetcher.  When a fetcher issues a memory load
// request, this info is packed into the unused data field of the
// request.  When the memory subsystem responds, it passes back the
// same info in an extra field inside the memory response structure.
// Maintaining info about an inflight request inside the request
// itself provides an easy way to handle out-of-order responses from
// memory.
typedef struct {
  // Message address in the fetcher's flit buffer
  FetcherFlitBufferMsgAddr msgAddr;
  // How many beats in the burst?
  Bit#(`BeatBurstWidth) burst;
  // Is this the final burst of routing records for the current key?
  Bool finalBurst;
} InflightFetcherReqInfo deriving (Bits);

// Fetcher interface
interface Fetcher;
  // Incoming and outgoing flits
  interface In#(Flit) flitIn;
  interface BOut#(RoutedFlit) flitOut;
  // Off-chip RAM connections
  Vector#(`DRAMsPerBoard, BOut#(DRAMReq)) ramReqs;
  Vector#(`DRAMsPerBoard, In#(DRAMResp)) ramResps;
endinterface

// Fetcher module
module mkFetcher#(BoardId boardId) (Fetcher);

  // Flit input port
  InPort#(Flit)) flitInPort <- mkInPort;

  // RAM request queues
  Vector#(`DRAMsPerBoard, Queue1#(DRAMReq)) ramReqQueue <-
    replicateM(mkUGShiftQueue(QueueOptFmax));

  // Flit buffer
  BlockRamOpts flitBufferOpts =
    BlockRamOpts {
      readDuringWrite: DontCare,
      style: "AUTO",
      registerDataOut: False,
      initFile: Invalid
    };
  BlockRam#(FetcherFlitBufferAddr, Flit) flitBuffer <- mkBlockRam;

  // Beat buffer
  SizedQueue#(`LogProgRouterBeatBufferSize, NumberedRoutingBeat))
    beatBuffer <- replicateM(mkUGSizedQueue);

  // Track length of beat buffer, so that we don't overfetch
  Count#(TAdd#(`LogProgRouterBeatBufferSize, 1)) beatBufferLen <-
      mkCount(2 ** `LogProgRouterBeatBufferSize);

  // For flits whose destinations are *not* routing keys
  Queue1#(RoutedFlit) flitBypassQueue <- mkUGShiftQueue(QueueOptFmax);

  // For flits whose destinations are routing keys
  Queue1#(RoutedFlit) flitProcessedQueue <- mkUGShiftQueue(QueueOptFmax);

  // Final output queue for flits
  Queue1#(RoutedFlit) flitOutQueue <- mkUGShiftQueue(QueueOptFmax);

  // Stage 1: consume input message
  // ------------------------------

  // Consumer state
  // State 0: pass through flits that don't contain routing keys
  // State 1: buffer flits that do contain routing keys
  // State 2: fetch routing beats
  Reg#(Bit#(2)) consumeState <- mkReg(0);

  // Count number of flits of message consumed so far
  Reg#(Bit#(`LogFlitsPerMsg)) consumeFlitCount <- mkReg(0);

  // Flit slot allocator
  Vector#(`FetcherMsgsPerFlitBuffer, SetReset) flitBufferUsedSlots <-
    mkSetReset(False);

  // Chosen message slot
  Reg#(FetcherFlitBufferMsgAddr) chosenReg <- mkRegU;

  // Routing key of message consumed
  Reg#(RoutingKey) consumeKey <- mkRegU;

  // Maintain count of routing beats fetched so far
  Reg#(Bit#(`LogRoutingEntryLen)) fetchBeatCount <- mkReg(0);

  // State 0: pass through flits that don't contain routing keys
  rule consumeMessage0 (consumeState == 0);
    Flit flit = flitInPort.value;
    // Find unused message slot
    Bool found = False;
    FetcherFlitBufferMsgAddr chosen = ?;
    for (Integer i = 0; i < `FetcherMsgsPerFlitBuffer; i=i+1) 
      if (flitBufferUsedSlots[i].value == 0) begin
        found = True;
        chosen = fromInteger(i);
      end
    chosenReg <= chosen;
    // Initialise counters for subsequent states
    flitCount <= 0;
    fetchBeatCount<= 0;
    // Consume flit
    if (flitInPort.canGet) begin
      if (flit.dest.addr.isKey) begin
        if (found) begin
          consumeState <= 1;
        end
      end else if (flitBypassQueue.notFull) begin
        flitInPort.get;
        // Make routing decision
        RoutingDecision decision = RouteLocal;
        MailboxNetAddr = flit.dest.addr;
        if (a.addr.host.valid)
          decision = addr.host.value == 0 ? RouteWest : RouteEast;
        else if (addr.board.x < boardId.x) decision = RouteWest;
        else if (addr.board.x > boardId.x) decision = RouteEast;
        else if (addr.board.y < boardId.y) decision = RouteSouth;
        else if (addr.board.y > boardId.y) decision = RouteNorth;
        // Insert into bypass queue
        flitBypassQueue.enq(RoutedFlit { decision: decision, flit: flit});
      end
    end
  endrule

  // State 1: buffer flits that do contain routing keys
  rule consumeMessage1 (consumeState == 1);
    Flit flit = flitInPort.value;
    if (flitInPort.canGet) begin
      flitInPort.get;
      consumeKey <= getRoutingKey(flit.dest.addr);
      // Write to flit buffer
      flitBuffer.write({chosenReg, consumeFlitCount}, flit);
      consumeFlitCount <= consumeFlitCount + 1;
      // On final flit, move to fetch state
      if (! flit.notFinalFlit) begin
        consumeState <= 2;
        // Claim chosen slot
        flitBufferUsedSlots[chosenReg].set;
      end
    end
  endrule

  // State 2: fetch routing beats
  rule consumeMessage2 (consumeState == 2);
    // Have we finished fetching beats?
    Bool finished = fetchBeatCount + `ProgRouterMaxBurst >= consumeKey.len;
    // Prepare inflight RAM request info
    // (to handle out of order resps from the RAMs)
    InflightFetcherReqInfo info;
    info.msgAddr = chosenReg;
    info.burst = min(consumeKey.len - fetchBeatCount, `ProgRouterMaxBurst);
    info.finalBurst = finished;
    // Prepare RAM request
    DRAMReq req;
    req.isStore = False;
    req.id = fromInteger(`DCachesPerDRAM + myId);
    req.addr = {1'b0, consumeKey.ptr + fetchBeatCount};
    req.data = {?, pack(info)};
    req.burst = info.burst;
    // Don't overfetch (beat buffer has finite size)
    if (ramReqQueue[consumeKey.ram].notFull &&
          beatBufferLen.available >= zeroExtend(req.burst)) begin
        ramReqQueue[consumeKey.ram].enq(req);
        fetchBeatCount <= fetchBeatCount + req.burst;
        beatBufferLen.incBy(zeroExtend(req.burst));
        if (finished) consumeState <= 0;
      end
    end
  endrule

  // Stage 2: interpret routing beats
  // --------------------------------

  // Merge responses from each RAM
  staticAssert(`DRAMsPerBoard == 2,
    "Fetcher: need to generalise number of RAMs used")
  MergeUnit#(NumberedRoutingBeat) ramRespMerger <- mkMergeUnitFair;

  // Convert a RAM response to a numbered routing beat
  function NumberedRoutingBeat fromDRAMResp(DRAMResp resp) =
    NumberedRoutingBeat {
      beat: unpack(resp.data)
    , beatNum: resp.beat
    };

  // Create RAM response input interfaces for this module
  In#(DRAMResp) respA <- onIn(fromDRAMResp, ramRespMerger.inA);
  In#(DRAMResp) respB <- onIn(fromDRAMResp, ramRespMerger.inB);
  Vector#(`DRAMsPerBoard, In#(DRAMResp)) ramResps = vector(respA, respB);

  // Connect the merger to the beat buffer
  connectToQueue(ramRespMerger.out, beatBuffer);

  // Count number of flits of message emitted so far
  Reg#(Bit#(`LogFlitsPerMsg)) emitFlitCount <- mkReg(0);

  // Count number of records processed so far in current beat
  Reg#(Bit#(3)) recordCount <- mkReg(0);

  // (Shift) register holding current routing beat
  Reg#(NumberedRoutingBeat) beatReg <- mkRegU;

  // Interpreter state
  // 0: register the routing beat and fetch first flit
  // 1: interpret flits
  Reg#(Bit#(1)) interpreterState <- mkReg(0);

  // State 0: register the routing beat and fetch first flit
  rule interpreter0 (interpreterState == 0);
    let beat = beatBuffer.dataOut.beat;
    InflightFetcherReqInfo info = unpack(truncate(beat.info));
    // Consume beat
    if (beatBuffer.canDeq && beatBuffer.canPeek) begin
      beatReg <= beatBuffer.dataOut;
      beatBuffer.deq;
      interpreterState <= 1;
    end
    // Load first flit
    flitBuffer.load({info.msgAddr, 0});
    emitFlitCount <= 0;
    recordCount <= 0;
  endrule

  // State 1: interpret flits
  rule interpreter1 (interpreterState == 1);
    // Extract details of registered routing beat
    let beat = beatReg.beat;
    let beatNum = beatReg.beatNum;
    InflightFetcherReqInfo info = unpack(truncate(beat.info));
    // Extract tag from next record
    RoutingRecordTag tag = beat.chunks[5].tag;
    // Is this the first flit of a message?
    Bool firstFlit = emitFlitCount == 0;
    // Modify flit by interpreting routing key
    RoutingDecision decision = ?;
    Flit flit = flitBuffer.dataOut;
    case (tag)
      // 40-bit Unicast Router-to-Mailbox
      URM1: begin
        URM1Record rec = unpack(beat.chunks[5]);
        flit.dest.addr.isKey = False;
        flit.dest.addr.mbox = rec.mbox;
        Vector#(`ThreadsPerMailbox, Bool) threadMask = newVector;
        for (Integer j = 0; j < `ThreadsPerMailbox; j=j+1)
          threadMask = rec.thread == fromInteger(j);
        flit.dest.threads = pack(threadMask);
        // Replace first word of message with local key
        if (firstFlit)
          flit.payload = {truncateLSB(flit.payload), 5'b0, rec.localKey};
        decision = RouteLocal;
      end
      // 80-bit Unicast Router-to-Mailbox
      URM2: begin
        URM2Record rec = unpack({beat.chunks[5], beat.chunks[4]});
        flit.dest.addr.isKey = False;
        flit.dest.addr.mbox = rec.mbox;
        Vector#(`ThreadsPerMailbox, Bool) threadMask = newVector;
        for (Integer j = 0; j < `ThreadsPerMailbox; j=j+1)
          threadMask = rec.thread == fromInteger(j);
        flit.dest.threads = pack(threadMask);
        // Replace first two words of message with local key
        if (firstFlit)
          flit.payload = {truncateLSB(flit.payload), rec.localKey};
        decision = RouteLocal;
      end
      // 40-bit Router-to-Router
      RR: begin
        RRRecord rec = unpack(beat.chunks[5]);
        case (rec.dir)
          NORTH: begin
            decision = RouteNorth;
            flit.dest.board = BoardId {x: boardId.x, y: boardId.y+1};
          end
          SOUTH: begin
            decision = RouteSouth;
            flit.dest.board = BoardId {x: boardId.x, y: boardId.y-1};
          end
          EAST: begin
            decision = RouteEast;
            flit.dest.board = BoardId {x: boardId.x+1, y: boardId.y};
          end
          WEST: begin
            decision = RouteWest;
            flit.dest.board = BoardId {x: boardId.x-1, y: boardId.y};
          end
        endcase
        flit.dest.threads = {?, rec.newKey};
      end
      // 80-bit Multicast Router-to-Mailbox
      MRM: begin
        MRMRecord rec = unpack({beat.chunks[5], beat.chunks[4]});
        flit.dest.addr.isKey = False;
        flit.dest.addr.mbox = rec.mbox;
        flit.dest.threads = rec.destMask;
        decision = RouteLocal;
      end
      // 40-bit Indirection
      IND: begin
        INDRecord rec = unpack(beat.chunks[5]);
        flit.dest.threads = {?, rec.newKey};
        decision = RouteLoop;
      end
    end
    // Is output queue ready for new flit?
    Bool emit = flitProcessedQueue.notFull;
    Bool newFlitCount = emitFlitCount;
    // Consume routing record
    if (emit) begin
      flitProcessedQueue.enq(RoutedFlit { decision: decision, flit: flit });
      // Move to next record
      recordCount <= recordCount + 1;
      // Shift beat to point to next record
      RoutingBeat newBeat = beat;
      Bool doubleChunk = unpack(pack(tag)[0]);
      if (doubleChunk) begin
        for (Integer i = 5; i > 2; i=i-2) begin
          newBeat.chunks[i] = beat.chunks[i-2];
          newBeat.chunks[i-1] = beat.chunks[i-3];
        end
      end else begin
        for (Integer i = 5; i > 0; i=i-1)
          newBeat.chunks[i] = beat.chunks[i-1];
      end
      beatReg <= NumberedRoutingBeat { beatNum: beatNum, beat: newBeat };
      // Is this the final record in the beat?
      if ((recordCount+1) == beat.size) begin
        interpreterState <= 0;
        // Have we finished with this message yet?
        if (info.finalBurst && info.burst == (beatNum+1)) begin
          // Reclaim message slot in flit buffer
          flitBufferUsedSlots[info.msgAddr].clear;
        end
      end
      // Is this the final flit in the message?
      if (flit.notFinalFlit)
        newFlitCount = emitFlitCount + 1;
      else
        newFlitCount = 0;
    end
    // Issue flit load request
    flitBuffer.load({info.msgAddr, newFlitCount});
    emitFlitCount <= newFlitCount;
  endrule

  // Stage 3: merge output queues
  // ----------------------------

  // We want to merge messages, not flits
  // Are we in the middle of consuming a message?
  Reg#(Bool) mergeInProgress <- mkReg(False);
  Reg#(Bool) prevFromBypass <- mkReg(False);

  rule merge (flitOutQueue.notFull);
    // Favour the bypass queue
    Bool chooseBypass = mergeInProgress ? prevFromBypass :
      flitBypassQueue.canDeq;
    if (chooseBypass) begin
      if (flitBypassQueue.canDeq) begin
        flitBypassQueue.deq;
        flitOutQueue.enq(flitBypassQueue.dataOut);
        mergeInProgress <= flitBypassQueue.dataOut.flit.notFinalFlit;
        prevFromBypass = True;
      end
    end else if (flitProcessedQueue.canDeq) begin
      flitProcessedQueue.deq;
      flitOutQueue.enq(flitProcessedQueue.dataOut);
      mergeInProgress <= flitProcessedQueue.dataOut.flit.notFinalFlit;
      prevFromBypass = False;
    end
  endrule;

  // Interfaces
  // -----------

  interface flitIn = flitInPort.in;
  interface flitOut = queueToBOut(flitOutQueue);
  interface ramReqs = map(queueToBOut, ramReqQueue);
  interface ramResps = ramResps;

endmodule

// =============================================================================
// Programmable router
// =============================================================================

interface ProgRouter;
  // Incoming and outgoing flits
  interface Vector#(`FetchersPerProgRouter, In#(Flit) flitIn);
  interface Vector#(`FetchersPerProgRouter, Out#(Flit) flitOut);

  // Interface to off-chip memory
  interface Vector#(`DRAMsPerBoard,
    Vector#(`FetchersPerProgRouter, BOut#(DRAMReq))) ramReqs;
  interface Vector#(`DRAMsPerBoard,
    Vector#(`FetchersPerProgRouter, In#(DRAMResp))) ramResps;
endinterface

/*
module mkProgRouter (ProgRouter);

  // Flit input ports
  Vector#(`FetchersPerProgRouter, InPort#(Flit)) flitInPort <-
    replicateM(mkInPort);

endmodule
*/

endpackage
