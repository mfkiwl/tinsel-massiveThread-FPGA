# This file controls the parameters for the circuit generator

import sys

#==============================================================================
# Configurable parameters
#==============================================================================

# Intialise parameter map
p = {}

# The Altera device family being targetted
p["DeviceFamily"] = "'Stratix V'"

# Log of max number of cores
# (used to determine width of globally unique core id)
p["LogMaxCores"] = 8

# The number of hardware threads per core
p["LogThreadsPerCore"] = 4

# The number of 32-bit instructions that fit in a core's instruction memory
p["LogInstrsPerCore"] = 9

# Log of number of multi-threaded cores per tile
p["LogCoresPerTile"] = 2

# Log of number of tiles per tree
p["LogTilesPerTree"] = 0

# Log of number of 32-bit words in a single memory transfer
p["LogWordsPerBeat"] = 3

# Log of number of beats in a cache line
p["LogBeatsPerLine"] = 0

# Log of number of sets per thread in set-associative data cache
p["DCacheLogSetsPerThread"] = 5

# Log of number of ways per set in set-associative data cache
p["DCacheLogNumWays"] = 2

# Max number of outstanding DRAM requests permitted
p["DRAMLogMaxInFlight"] = 4

# DRAM latency in cycles (simulation only)
p["DRAMLatency"] = 20

# If set to 1, reduces logic usage, but each DRAM port is limited to
# half throughput (one response every other cycle).  This may be
# acceptable for various reasons, most notably when using a multi-port
# front-end to DRAM, e.g. on a Cyclone V board.
p["DRAMPortHalfThroughput"] = True

# DRAM byte-address width
p["DRAMAddrWidth"] = 30

#==============================================================================
# Derived Parameters
#==============================================================================

# (These should not be modified.)

# Log of number of 32-bit words per data cache line
p["LogWordsPerLine"] = p["LogWordsPerBeat"]+p["LogBeatsPerLine"]

# Log of number of bytes per data cache line
p["LogBytesPerLine"] = 2+p["LogWordsPerLine"]

# Number of 32-bit words per data cache line
p["WordsPerLine"] = 2**p["LogWordsPerLine"]

# Data cache line size in bits
p["LineSize"] = p["WordsPerLine"] * 32

# Number of beats per cache line
p["BeatsPerLine"] = 2**p["LogBeatsPerLine"]

# Number of 32-bit words in a memory transfer
p["WordsPerBeat"] = 2**p["LogWordsPerBeat"]

# Log of number of bytes per data transfer
p["LogBytesPerBeat"] = 2+p["LogWordsPerBeat"]

# Number of data bytes in a memory transfer
p["BytesPerBeat"] = 2**p["LogBytesPerBeat"]

# Memory transfer bus width in bits
p["BusWidth"] = p["WordsPerBeat"] * 32

# Longest possible burst transfer is 2^BurstWidth-1
p["BurstWidth"] = p["LogBeatsPerLine"]+1

# Log of size of data cache writeback buffer
p["LogDCacheWriteBufferSize"] = (p["LogBeatsPerLine"]
                                   if p["LogBeatsPerLine"] < 2 else 2)

# Cores per tile
p["CoresPerTile"] = 2**p["LogCoresPerTile"]

#==============================================================================
# Main 
#==============================================================================

if len(sys.argv) > 1:
  mode = sys.argv[1]
else:
  print "Usage: config.py <cpp|bash>"
  sys.exit(-1)

if mode == "cpp":
  for var in p:
    if isinstance(p[var], bool):
      if p[var]: print("-D " + var),
    else:
      print("-D " + var + "=" + str(p[var])),
elif mode == "bash":
  for var in p:
    print("export " + var + "=" + str(p[var]))