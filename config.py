# This file controls the parameters for the circuit generator

import sys

#==============================================================================
# Prelude
#==============================================================================

def quoted(s): return "'\"" + s + "\"'"

# Intialise parameter map
p = {}

#==============================================================================
# DE5 config
#==============================================================================

# The Altera device family being targetted
p["DeviceFamily"] = quoted("Stratix V")

# FPGA board being targetted
p["TargetBoard"] = quoted("DE5")

# Log of max number of cores
# (used to determine width of globally unique core id)
p["LogMaxCores"] = 8

# The number of hardware threads per core
p["LogThreadsPerCore"] = 4

# The number of 32-bit instructions that fit in a core's instruction memory
p["LogInstrsPerCore"] = 9

# Log of number of multi-threaded cores sharing a DCache
p["LogCoresPerDCache"] = 2

# Log of number of caches per DRAM port
p["LogDCachesPerDRAM"] = 3

# Log of number of 32-bit words in a single memory transfer
p["LogWordsPerBeat"] = 3

# Log of number of beats in a cache line
p["LogBeatsPerLine"] = 0

# Log of number of sets per thread in set-associative data cache
p["DCacheLogSetsPerThread"] = 3

# Log of number of ways per set in set-associative data cache
p["DCacheLogNumWays"] = 2

# Max number of outstanding DRAM requests permitted
p["DRAMLogMaxInFlight"] = 4

# DRAM latency in cycles (simulation only)
p["DRAMLatency"] = 20

# DRAM byte-address width
p["DRAMAddrWidth"] = 30

# Size of packet payload
p["LogWordsPerMsg"] = 3

# Space available per thread in mailbox scratchpad
p["LogMsgsPerThread"] = 4

# Number of cores sharing a mailbox
p["LogCoresPerMailbox"] = 2

# Enable mailbox (message-passing between threads)
p["MailboxEnabled"] = True

# Use array of set instead of array of queue in mailbox client helper
#p["MailboxClientUseSet"] = True

# Number of mailboxes per ring
p["LogRingSize"] = 3;

#==============================================================================
# Derived Parameters
#==============================================================================

# (These should not be modified.)

# Max number of threads
p["LogMaxThreads"] = p["LogMaxCores"]+p["LogThreadsPerCore"]

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

# Data cache beat width in bits
p["BeatWidth"] = p["WordsPerBeat"] * 32

# Longest possible burst transfer is 2^BurstWidth-1
p["BurstWidth"] = p["LogBeatsPerLine"]+1

# Log of size of data cache writeback buffer
p["LogDCacheWriteBufferSize"] = (p["LogBeatsPerLine"]
                                   if p["LogBeatsPerLine"] < 2 else 2)

# Cores per DCache
p["CoresPerDCache"] = 2**p["LogCoresPerDCache"]

# Caches per DRAM
p["DCachesPerDRAM"] = 2**p["LogDCachesPerDRAM"]

# Number of bytes per message
p["LogBytesPerMsg"] = p["LogWordsPerMsg"] + 2

# Number of words per message
p["WordsPerMsg"] = 2**p["LogWordsPerMsg"]

# Number of cores sharing a mailbox
p["CoresPerMailbox"] = 2 ** p["LogCoresPerMailbox"]

# Number of threads sharing a mailbox
p["LogThreadsPerMailbox"] = p["LogCoresPerMailbox"]+p["LogThreadsPerCore"]

# Size of memory-mapped region for mailbox scratchpad in bytes
p["LogScratchpadBytes"] = 1+p["LogWordsPerMsg"]+2+p["LogMsgsPerThread"]

# Number of mailboxes per ring
p["RingSize"] = 2 ** p["LogRingSize"];

#==============================================================================
# Main 
#==============================================================================

if len(sys.argv) > 1:
  mode = sys.argv[1]
else:
  print "Usage: config.py <defs|envs|cpp>"
  sys.exit(-1)

if mode == "defs":
  for var in p:
    if isinstance(p[var], bool):
      if p[var]: print("-D " + var),
    else:
      print("-D " + var + "=" + str(p[var])),
elif mode == "envs":
  for var in p:
    print("export " + var + "=" + str(p[var]))
elif mode == "cpp":
  for var in p:
    print("#define " + var + " " + str(p[var]))
