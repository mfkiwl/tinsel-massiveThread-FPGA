// SPDX-License-Identifier: BSD-2-Clause
#ifndef _PGRAPH_H_
#define _PGRAPH_H_

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <HostLink.h>
#include <config.h>
#include <POLite.h>
#include <POLite/Seq.h>
#include <POLite/Graph.h>
#include <POLite/Placer.h>
#include <POLite/ProgRouters.h>
#include <type_traits>

// Nodes of a POETS graph are devices
typedef NodeId PDeviceId;

// POETS graph
template <typename DeviceType,
          typename S, typename E, typename M> class PGraph {
 private:
  // Align address to 2^n byte boundary
  inline uint32_t align(uint32_t n, uint32_t addr) {
    if ((addr & (1<<n)-1) == 0) return addr;
    return ((addr >> n) + 1) << n;
  }

  // Align address to 32-bit word boundary
  uint32_t wordAlign(uint32_t addr) { return align(2, addr); }

  // Align address to cache-line boundary
  uint32_t cacheAlign(uint32_t addr) {
    return align(TinselLogBytesPerLine, addr);
  }

  // Helper function
  inline uint32_t min(uint32_t x, uint32_t y) { return x < y ? x : y; }

  // Number of FPGA boards available
  uint32_t meshLenX;
  uint32_t meshLenY;

  // Number of FPGA boards to use
  uint32_t numBoardsX;
  uint32_t numBoardsY;

  // Out table (sender-side edge tables)
  // Sequence of destinations for every (device, pin) pair
  Seq<PRoutingDest>*** outTable;

  // Key table (sender-side key tables)
  // Global routing key for every (device, pin) pair
  uint32_t** keyTable;

  // In table (receiver-side edge tables)
  // Sequence of incoming edges for every thread
  Seq<PInEdge<E>>** inTable;

  // Mesh of per-board programmable routers
  ProgRouterMesh* routingTables;

  // Generic constructor
  void constructor(uint32_t lenX, uint32_t lenY) {
    meshLenX = lenX;
    meshLenY = lenY;
    char* str = getenv("POLITE_BOARDS_X");
    int nx = str ? atoi(str) : meshLenX;
    str = getenv("POLITE_BOARDS_Y");
    int ny = str ? atoi(str) : meshLenY;
    setNumBoards(nx, ny);
    numDevices = 0;
    devices = NULL;
    toDeviceAddr = NULL;
    numDevicesOnThread = NULL;
    fromDeviceAddr = NULL;
    vertexMem = NULL;
    vertexMemSize = NULL;
    vertexMemBase = NULL;
    inEdgeMem = NULL;
    inEdgeMemSize = NULL;
    inEdgeMemBase = NULL;
    mapVerticesToDRAM = false;
    mapInEdgesToDRAM = true;
    outTable = NULL;
    keyTable = NULL;
    inTable = NULL;
    routingTables = NULL;
    chatty = 0;
    str = getenv("POLITE_CHATTY");
    if (str != NULL) {
      chatty = !strcmp(str, "0") ? 0 : 1;
    }
  }

 public:
  // Number of devices
  uint32_t numDevices;

  // Graph containing device ids and connections
  Graph graph;

  // Edge labels: has same structure as graph.outgoing
  Seq<Seq<E>*> edgeLabels;

  // Mapping from device id to device state
  // (Not valid until the mapper is called)
  PState<S>** devices;

  // Mapping from thread id to number of devices on that thread
  // (Not valid until the mapper is called)
  uint32_t* numDevicesOnThread;

  // Mapping from device id to device address and back
  // (Not valid until the mapper is called)
  PDeviceAddr* toDeviceAddr;  // Device id -> device address
  PDeviceId** fromDeviceAddr; // Device address -> device id

  // Each thread's vertex mem and thread mem regions
  // (Not valid until the mapper is called)
  uint8_t** vertexMem;      uint8_t** threadMem;
  uint32_t* vertexMemSize;  uint32_t* threadMemSize;
  uint32_t* vertexMemBase;  uint32_t* threadMemBase;

  // Each thread's in-edge tables
  // (Not valid until the mapper is called)
  uint8_t** inEdgeMem;
  uint32_t* inEdgeMemSize;
  uint32_t* inEdgeMemBase;

  // Where to map the various regions
  // (If false, map to SRAM instead)
  bool mapVerticesToDRAM;
  bool mapInEdgesToDRAM;

  // Allow mapper to print useful information to stdout
  uint32_t chatty;

  // Setter for number of boards to use
  void setNumBoards(uint32_t x, uint32_t y) {
    if (x > meshLenX || y > meshLenY) {
      printf("Mapper: %d x %d boards requested, %d x %d available\n",
        numBoardsX, numBoardsY, meshLenX, meshLenY);
      exit(EXIT_FAILURE);
    }
    numBoardsX = x;
    numBoardsY = y;
  }

  // Create new device
  inline PDeviceId newDevice() {
    edgeLabels.append(new SmallSeq<E>);
    numDevices++;
    return graph.newNode();
  }

  // Add a connection between devices
  inline void addEdge(PDeviceId from, PinId pin, PDeviceId to) {
    if (pin >= POLITE_NUM_PINS) {
      printf("addEdge: pin exceeds POLITE_NUM_PINS\n");
      exit(EXIT_FAILURE);
    }
    graph.addEdge(from, pin, to);
    E edge;
    edgeLabels.elems[from]->append(edge);
  }

  // Add labelled edge using given output pin
  void addLabelledEdge(E edge, PDeviceId x, PinId pin, PDeviceId y) {
    graph.addEdge(x, pin, y);
    edgeLabels.elems[x]->append(edge);
  }

  // Allocate SRAM and DRAM partitions
  void allocatePartitions() {
    // Decide a maximum partition size that is reasonable
    // SRAM: Partition size minus 2048 bytes for the stack
    uint32_t maxSRAMSize = (1<<TinselLogBytesPerSRAMPartition) - 2048;
    // DRAM: Partition size minus 65536 bytes for the stack
    uint32_t maxDRAMSize = (1<<TinselLogBytesPerDRAMPartition) - 65536;
    // Allocate partition sizes and bases
    vertexMem = (uint8_t**) calloc(TinselMaxThreads, sizeof(uint8_t*));
    vertexMemSize = (uint32_t*) calloc(TinselMaxThreads, sizeof(uint32_t));
    vertexMemBase = (uint32_t*) calloc(TinselMaxThreads, sizeof(uint32_t));
    threadMem = (uint8_t**) calloc(TinselMaxThreads, sizeof(uint8_t*));
    threadMemSize = (uint32_t*) calloc(TinselMaxThreads, sizeof(uint32_t));
    threadMemBase = (uint32_t*) calloc(TinselMaxThreads, sizeof(uint32_t));
    inEdgeMem = (uint8_t**) calloc(TinselMaxThreads, sizeof(uint8_t*));
    inEdgeMemSize = (uint32_t*) calloc(TinselMaxThreads, sizeof(uint32_t));
    inEdgeMemBase = (uint32_t*) calloc(TinselMaxThreads, sizeof(uint32_t));
    // Compute partition sizes for each thread
    for (uint32_t threadId = 0; threadId < TinselMaxThreads; threadId++) {
      // This variable is used to count the size of the *initialised*
      // partition.  The total partition size is larger as it includes
      // uninitialised portions.
      uint32_t sizeVMem = 0;
      uint32_t sizeEIMem = 0;
      uint32_t sizeTMem = 0;
      // Add space for thread structure (always stored in SRAM)
      sizeTMem = cacheAlign(sizeof(PThread<DeviceType, S, E, M>));
      // Add space for devices
      uint32_t numDevs = numDevicesOnThread[threadId];
      for (uint32_t devNum = 0; devNum < numDevs; devNum++) {
        // Add space for device
        sizeVMem = sizeVMem + sizeof(PState<S>);
      }
      // Add space for incoming edge table
      if (inTable[threadId]) {
        sizeEIMem = inTable[threadId]->numElems * sizeof(PInEdge<E>);
        sizeEIMem = wordAlign(sizeEIMem);
      }
      // The total partition size including uninitialised portions
      uint32_t totalSizeVMem =
        sizeVMem + wordAlign(sizeof(PLocalDeviceId) * numDevs);
      // Check that total size is reasonable
      uint32_t totalSizeSRAM = sizeTMem;
      uint32_t totalSizeDRAM = 0;
      if (mapVerticesToDRAM) totalSizeDRAM += totalSizeVMem;
                        else totalSizeSRAM += totalSizeVMem;
      if (mapInEdgesToDRAM)  totalSizeDRAM += sizeEIMem;
                        else totalSizeSRAM += sizeEIMem;
      if (totalSizeDRAM > maxDRAMSize) {
        printf("Error: max DRAM partition size exceeded\n");
        exit(EXIT_FAILURE);
      }
      if (totalSizeSRAM > maxSRAMSize) {
        printf("Error: max SRAM partition size exceeded\n");
        exit(EXIT_FAILURE);
      }
      // Allocate space for the initialised portion of the partition
      assert((sizeVMem%4) == 0);
      assert((sizeTMem%4) == 0);
      assert((sizeEIMem%4) == 0);
      vertexMem[threadId] = (uint8_t*) calloc(sizeVMem, 1);
      vertexMemSize[threadId] = sizeVMem;
      threadMem[threadId] = (uint8_t*) calloc(sizeTMem, 1);
      threadMemSize[threadId] = sizeTMem;
      inEdgeMem[threadId] = (uint8_t*) calloc(sizeEIMem, 1);
      inEdgeMemSize[threadId] = sizeEIMem;
      // Tinsel address of base of partition
      uint32_t partId = threadId & (TinselThreadsPerDRAM-1);
      uint32_t sramBase = (1 << TinselLogBytesPerSRAM) +
          (partId << TinselLogBytesPerSRAMPartition);
      uint32_t dramBase = TinselBytesPerDRAM -
          ((partId+1) << TinselLogBytesPerDRAMPartition);
      // Use partition-interleaved region for DRAM
      dramBase |= 0x80000000;
      threadMemBase[threadId] = sramBase;
      sramBase += threadMemSize[threadId];
      // Determine base addresses of each region
      if (mapVerticesToDRAM) {
        vertexMemBase[threadId] = dramBase;
        dramBase += totalSizeVMem;
      }
      else {
        vertexMemBase[threadId] = sramBase;
        sramBase += totalSizeVMem;
      }
      if (mapInEdgesToDRAM) {
        inEdgeMemBase[threadId] = dramBase;
        dramBase += sizeEIMem;
      }
      else {
        inEdgeMemBase[threadId] = sramBase;
        sramBase += sizeEIMem;
      }
    }
  }

  // Initialise partitions
  void initialisePartitions() {
    for (uint32_t threadId = 0; threadId < TinselMaxThreads; threadId++) {
      // Next pointers for each partition
      uint32_t nextVMem = 0;
      // Pointer to thread structure
      PThread<DeviceType, S, E, M>* thread =
        (PThread<DeviceType, S, E, M>*) &threadMem[threadId][0];
      // Set number of devices on thread
      thread->numDevices = numDevicesOnThread[threadId];
      // Set number of devices in graph
      thread->numVertices = numDevices;
      // Set tinsel address of array of device states
      thread->devices = vertexMemBase[threadId];
      // Set tinsel address of base of in-edge table
      thread->inTableBase = inEdgeMemBase[threadId];
      // Add space for each device on thread
      uint32_t numDevs = numDevicesOnThread[threadId];
      for (uint32_t devNum = 0; devNum < numDevs; devNum++) {
        PState<S>* dev = (PState<S>*) &vertexMem[threadId][nextVMem];
        PDeviceId id = fromDeviceAddr[threadId][devNum];
        devices[id] = dev;
        // Add space for device
        nextVMem = nextVMem + sizeof(PState<S>);
      }
      // Initialise each device and the thread's out edges
      for (uint32_t devNum = 0; devNum < numDevs; devNum++) {
        PDeviceId id = fromDeviceAddr[threadId][devNum];
        PState<S>* dev = devices[id];
        // Initialise
        for (uint32_t p = 0; p < POLITE_NUM_PINS; p++) {
          dev->pin[p] = keyTable[id][p];
        }
      }
      // Intialise thread's in edges
      PInEdge<E>* inEdgeArray = (PInEdge<E>*) inEdgeMem[threadId];
      Seq<PInEdge<E>>* edges = inTable[threadId];
      if (edges)
        for (uint32_t i = 0; i < edges->numElems; i++) {
          inEdgeArray[i] = edges->elems[i];
        }
      // At this point, check that next pointers line up with heap sizes
      if (nextVMem != vertexMemSize[threadId]) {
        printf("Error: vertex mem size does not match pre-computed size\n");
        exit(EXIT_FAILURE);
      }
      // Set tinsel address of senders array
      thread->senders = vertexMemBase[threadId] + nextVMem;
    }
  }

  // Allocate mapping structures
  void allocateMapping() {
    devices = (PState<S>**) calloc(numDevices, sizeof(PState<S>*));
    toDeviceAddr = (PDeviceAddr*) calloc(numDevices, sizeof(PDeviceAddr));
    fromDeviceAddr = (PDeviceId**) calloc(TinselMaxThreads, sizeof(PDeviceId*));
    numDevicesOnThread = (uint32_t*) calloc(TinselMaxThreads, sizeof(uint32_t));
  }

  // Allocate thread edge input and output tables
  // (Only valid after mapper is called)
  void allocateInOutTables() {
    // Receiver-side tables
    inTable = (Seq<PInEdge<E>>**)
      calloc(TinselMaxThreads,sizeof(Seq<PInEdge<E>>*));
    for (uint32_t t = 0; t < TinselMaxThreads; t++) {
      if (numDevicesOnThread[t] != 0)
        inTable[t] = new SmallSeq<PInEdge<E>>;
    }

    // Sender-side tables
    outTable = (Seq<PRoutingDest>***)
      calloc(numDevices, sizeof(Seq<PRoutingDest>**));
    for (uint32_t d = 0; d < numDevices; d++) {
      outTable[d] = (Seq<PRoutingDest>**)
        calloc(POLITE_NUM_PINS, sizeof(Seq<PRoutingDest>*));
      for (uint32_t p = 0; p < POLITE_NUM_PINS; p++)
        outTable[d][p] = new SmallSeq<PRoutingDest>;
    }

    keyTable = new uint32_t* [numDevices];
    for (uint32_t d = 0; d < numDevices; d++)
      keyTable[d] = new uint32_t [POLITE_NUM_PINS];
  }

  // Compute thread edge input and output tables
  // (Only valid after mapper is called)
  void computeInOutTables() {
    // For each device
    for (uint32_t d = 0; d < numDevices; d++) {
      // For each pin
      for (uint32_t p = 0; p < POLITE_NUM_PINS; p++) {
        Seq<PDeviceId>* dests = graph.outgoing->elems[d];
        Seq<E>* edges = edgeLabels.elems[d];
        for (uint32_t i = 0; i < dests->numElems; i++) {
          PDeviceId destId = dests->elems[i];
          // Destination thread id
          uint32_t threadId = getThreadId(toDeviceAddr[destId]);
          // Thread-local device id
          uint32_t devId = getLocalDeviceId(toDeviceAddr[destId]);
          // Add edge to thread's input table
          uint32_t edgeId = inTable[threadId]->numElems;
          if (i < inTable[threadId]->numElems) {
            PInEdge<E> edge;
            edge.edge = edges->elems[i];
            inTable[threadId]->append(edge);
          }
          // Add output table entry
          PRoutingDest rdest;
          rdest.kind = PRDestKindURM1;
          rdest.mbox = threadId >> TinselLogThreadsPerMailbox;
          rdest.urm1.key = devId | (edgeId << 16);
          rdest.urm1.threadId = threadId &
            ((1<<TinselLogThreadsPerMailbox) - 1);
          outTable[d][p]->append(rdest);
        }
      }
    }
  }

  // Release all structures
  void releaseAll() {
    if (devices != NULL) {
      free(devices);
      free(toDeviceAddr);
      free(numDevicesOnThread);
      for (uint32_t t = 0; t < TinselMaxThreads; t++)
        if (fromDeviceAddr[t] != NULL) free(fromDeviceAddr[t]);
      free(fromDeviceAddr);
      for (uint32_t t = 0; t < TinselMaxThreads; t++)
        if (vertexMem[t] != NULL) free(vertexMem[t]);
      free(vertexMem);
      free(vertexMemSize);
      free(vertexMemBase);
      for (uint32_t t = 0; t < TinselMaxThreads; t++)
        if (threadMem[t] != NULL) free(threadMem[t]);
      free(threadMem);
      free(threadMemSize);
      free(threadMemBase);
      for (uint32_t t = 0; t < TinselMaxThreads; t++)
        if (inEdgeMem[t] != NULL) free(inEdgeMem[t]);
      free(inEdgeMem);
      free(inEdgeMemSize);
      free(inEdgeMemBase);
    }
    if (inTable != NULL) {
      for (uint32_t t = 0; t < TinselMaxThreads; t++)
        if (inTable[t] != NULL) delete inTable[t];
      free(inTable);
      inTable = NULL;
    }
    if (outTable != NULL) {
      for (uint32_t d = 0; d < numDevices; d++) {
        if (outTable[d] == NULL) continue;
        for (uint32_t p = 0; p < POLITE_NUM_PINS; p++)
          delete outTable[d][p];
        free(outTable[d]);
      }
      free(outTable);
      outTable = NULL;
    }
    if (keyTable != NULL) {
      for (uint32_t d = 0; d < numDevices; d++) delete [] keyTable[d];
      delete [] keyTable;
      keyTable = NULL;
    }
    if (routingTables != NULL) delete routingTables;
  }

  // Implement mapping to tinsel threads
  void map() {
    // Let's measure some times
    struct timeval placementStart, placementFinish;
    struct timeval routingStart, routingFinish;
    struct timeval initStart, initFinish;

    // Release all mapping and heap structures
    releaseAll();

    // Reallocate mapping structures
    allocateMapping();

    // Start placement timer
    gettimeofday(&placementStart, NULL);

    // Partition into subgraphs, one per board
    Placer boards(&graph, numBoardsX, numBoardsY);

    // Place subgraphs onto 2D mesh
    const uint32_t placerEffort = 8;
    boards.place(placerEffort);

    // For each board
    for (uint32_t boardY = 0; boardY < numBoardsY; boardY++) {
      for (uint32_t boardX = 0; boardX < numBoardsX; boardX++) {
        // Partition into subgraphs, one per mailbox
        PartitionId b = boards.mapping[boardY][boardX];
        Placer boxes(&boards.subgraphs[b], 
                 TinselMailboxMeshXLen, TinselMailboxMeshYLen);
        boxes.place(placerEffort);

        // For each mailbox
        for (uint32_t boxX = 0; boxX < TinselMailboxMeshXLen; boxX++) {
          for (uint32_t boxY = 0; boxY < TinselMailboxMeshYLen; boxY++) {
            // Partition into subgraphs, one per thread
            uint32_t numThreads = 1<<TinselLogThreadsPerMailbox;
            PartitionId t = boxes.mapping[boxY][boxX];
            Placer threads(&boxes.subgraphs[t], numThreads, 1);

            // For each thread
            for (uint32_t threadNum = 0; threadNum < numThreads; threadNum++) {
              // Determine tinsel thread id
              uint32_t threadId = boardY;
              threadId = (threadId << TinselMeshXBits) | boardX;
              threadId = (threadId << TinselMailboxMeshYBits) | boxY;
              threadId = (threadId << TinselMailboxMeshXBits) | boxX;
              threadId = (threadId << (TinselLogCoresPerMailbox +
                            TinselLogThreadsPerCore)) | threadNum;

              // Get subgraph
              Graph* g = &threads.subgraphs[threadNum];

              // Populate fromDeviceAddr mapping
              uint32_t numDevs = g->incoming->numElems;
              numDevicesOnThread[threadId] = numDevs;
              fromDeviceAddr[threadId] = (PDeviceId*)
                malloc(sizeof(PDeviceId) * numDevs);
              for (uint32_t devNum = 0; devNum < numDevs; devNum++)
                fromDeviceAddr[threadId][devNum] = g->labels->elems[devNum];
  
              // Populate toDeviceAddr mapping
              assert(numDevs < maxLocalDeviceId());
              for (uint32_t devNum = 0; devNum < numDevs; devNum++) {
                PDeviceAddr devAddr =
                  makeDeviceAddr(threadId, devNum);
                toDeviceAddr[g->labels->elems[devNum]] = devAddr;
              }
            }
          }
        }
      }
    }

    // Stop placement timer and start In/Out table timer
    gettimeofday(&placementFinish, NULL);
    gettimeofday(&routingStart, NULL);

    // Compute send and receive side routing tables
    allocateInOutTables();
    computeInOutTables();

    // Compute per-board programmable routing tables
    routingTables = new ProgRouterMesh(numBoardsX, numBoardsY);
    for (uint32_t d = 0; d < numDevices; d++) {
      uint32_t src = getThreadId(toDeviceAddr[d]) >>
        TinselLogThreadsPerMailbox;
      for (uint32_t p = 0; p < POLITE_NUM_PINS; p++)
        keyTable[d][p] = routingTables->addDestsFromBoard(src, outTable[d][p]);
   }

    // Stop routing timer and start init timer
    gettimeofday(&routingFinish, NULL);
    gettimeofday(&initStart, NULL);

    // Reallocate and initialise heap structures
    allocatePartitions();
    initialisePartitions();

    // Display times, if chatty
    gettimeofday(&initFinish, NULL);
    if (chatty > 0) {
      struct timeval diff;

      timersub(&placementFinish, &placementStart, &diff);
      double duration = (double) diff.tv_sec +
        (double) diff.tv_usec / 1000000.0;
      printf("POLite mapper profile:\n");
      printf("  Partitioning and placement: %lfs\n", duration);

      timersub(&routingFinish, &routingStart, &diff);
      duration = (double) diff.tv_sec + (double) diff.tv_usec / 1000000.0;
      printf("  In/Out table construction: %lfs\n", duration);

      timersub(&initFinish, &initStart, &diff);
      duration = (double) diff.tv_sec + (double) diff.tv_usec / 1000000.0;
      printf("  Thread state initialisation: %lfs\n", duration);
    }
  }

  // Constructor
  PGraph() {
    char* str = getenv("HOSTLINK_BOXES_X");
    int x = str ? atoi(str) : 1;
    x = x * TinselMeshXLenWithinBox;
    str = getenv("HOSTLINK_BOXES_Y");
    int y = str ? atoi(str) : 1;
    y = y * TinselMeshYLenWithinBox;
    constructor(x, y);
  }
  PGraph(uint32_t numBoxesX, uint32_t numBoxesY) {
    int x = numBoxesX * TinselMeshXLenWithinBox; 
    int y = numBoxesY * TinselMeshYLenWithinBox;
    constructor(x, y);
  }

  // Deconstructor
  ~PGraph() {
    releaseAll();
    for (uint32_t i = 0; i < edgeLabels.numElems; i++)
      delete edgeLabels.elems[i];
  }

  // Write partition to tinsel machine
  void writeRAM(HostLink* hostLink,
         uint8_t** heap, uint32_t* heapSize, uint32_t* heapBase) {
    // Number of bytes written by each thread
    uint32_t* writeCount = (uint32_t*)
      calloc(TinselMaxThreads, sizeof(uint32_t));

    // Number of threads completed by each core
    uint32_t*** threadCount = (uint32_t***)
      calloc(meshLenX, sizeof(uint32_t**));
    for (uint32_t x = 0; x < meshLenX; x++) {
      threadCount[x] = (uint32_t**)
        calloc(meshLenY, sizeof(uint32_t*));
      for (uint32_t y = 0; y < meshLenY; y++)
        threadCount[x][y] = (uint32_t*)
          calloc(TinselCoresPerBoard, sizeof(uint32_t));
    }

    // Initialise write addresses
    for (int x = 0; x < meshLenX; x++)
      for (int y = 0; y < meshLenY; y++)
        for (int c = 0; c < TinselCoresPerBoard; c++)
          hostLink->setAddr(x, y, c, heapBase[hostLink->toAddr(x, y, c, 0)]);

    // Write heaps
    uint32_t done = false;
    while (! done) {
      done = true;
      for (int x = 0; x < meshLenX; x++) {
        for (int y = 0; y < meshLenY; y++) {
          for (int c = 0; c < TinselCoresPerBoard; c++) {
            uint32_t t = threadCount[x][y][c];
            if (t < TinselThreadsPerCore) {
              done = false;
              uint32_t threadId = hostLink->toAddr(x, y, c, t);
              uint32_t written = writeCount[threadId];
              if (written == heapSize[threadId]) {
                threadCount[x][y][c] = t+1;
                if ((t+1) < TinselThreadsPerCore)
                  hostLink->setAddr(x, y, c,
                    heapBase[hostLink->toAddr(x, y, c, t+1)]);
              } else {
                uint32_t send = min((heapSize[threadId] - written)>>2, 15);
                hostLink->store(x, y, c, send,
                  (uint32_t*) &heap[threadId][written]);
                writeCount[threadId] = written + send * sizeof(uint32_t);
              }
            }
          }
        }
      }
    }

    // Release memory
    free(writeCount);
    for (uint32_t x = 0; x < meshLenX; x++) {
      for (uint32_t y = 0; y < meshLenY; y++)
        free(threadCount[x][y]);
      free(threadCount[x]);
    }
    free(threadCount);
  }

  // Write graph to tinsel machine
  void write(HostLink* hostLink) { 
    // Start timer
    struct timeval start, finish;
    gettimeofday(&start, NULL);

    bool useSendBufferOld = hostLink->useSendBuffer;
    hostLink->useSendBuffer = true;
    writeRAM(hostLink, vertexMem, vertexMemSize, vertexMemBase);
    writeRAM(hostLink, threadMem, threadMemSize, threadMemBase);
    writeRAM(hostLink, inEdgeMem, inEdgeMemSize, inEdgeMemBase);
    routingTables->write(hostLink);
    hostLink->flush();
    hostLink->useSendBuffer = useSendBufferOld;

    // Display time if chatty
    gettimeofday(&finish, NULL);
    if (chatty > 0) {
      struct timeval diff;
      timersub(&finish, &start, &diff);
      double duration = (double) diff.tv_sec +
        (double) diff.tv_usec / 1000000.0;
      printf("POLite graph upload time: %lfs\n", duration);
    }
  }

  // Determine fan-in of given device
  uint32_t fanIn(PDeviceId id) {
    return graph.fanIn(id);
  }

  // Determine fan-out of given device
  uint32_t fanOut(PDeviceId id) {
    return graph.fanOut(id);
  }

};

// Read performance stats and store in file
inline void politeSaveStats(HostLink* hostLink, const char* filename) {
  #ifdef POLITE_DUMP_STATS
  // Open file for performance counters
  FILE* statsFile = fopen(filename, "wt");
  if (statsFile == NULL) {
    printf("Error creating stats file\n");
    exit(EXIT_FAILURE);
  }
  uint32_t meshLenX = hostLink->meshXLen;
  uint32_t meshLenY = hostLink->meshYLen;
  // Number of caches
  uint32_t numLines = meshLenX * meshLenY *
                        TinselDCachesPerDRAM * TinselDRAMsPerBoard;
  // Add on number of cores
  numLines += meshLenX * meshLenY * TinselCoresPerBoard;
  // Add on number of threads
  #ifdef POLITE_COUNT_MSGS
  numLines += meshLenX * meshLenY * TinselThreadsPerBoard;
  #endif
  hostLink->dumpStdOut(statsFile, numLines);
  fclose(statsFile);
  #endif
}

#endif