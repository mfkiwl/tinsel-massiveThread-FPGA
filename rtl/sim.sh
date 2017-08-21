#!/bin/bash

if [ ! -e "./de5Top" ]; then
  echo 'Simulator not find, try "make sim" first'
  exit
fi

UDSOCK="../hostlink/udsock"
if [ ! -e "$UDSOCK" ]; then
  echo 'Cannot find udsock tool'
  exit
fi

# Load config parameters
while read -r EXPORT; do
  eval $EXPORT
done <<< `python ../config.py envs`

MESH_MAX_X=$((2 ** $MeshXBits))
MESH_MAX_Y=$((2 ** $MeshYBits))
echo "Max mesh dimensions: $MESH_MAX_X x $MESH_MAX_Y"

MESH_X=$MeshXLen
MESH_Y=$MeshYLen
echo "Using mesh dimensions: $MESH_X x $MESH_Y"

HOST_X=0
HOST_Y=$(($MESH_Y))
echo "Connecting bridge board at location ($HOST_X, $HOST_Y)"

# Check dimensions
if [ $MESH_X -gt $MESH_MAX_X ] || [ $MESH_Y -gt $MESH_MAX_Y ] ; then
  echo "ERROR: max mesh dimensions exceeded"
  exit
fi

# Convert coords to board id
function fromCoords {
  echo $(($2 * $MESH_MAX_X + $1))
}

LAST_X=$(($MESH_X - 1))
LAST_Y=$(($MESH_Y - 1))

# Run one simulator per board
for X in $(seq 0 $LAST_X); do
  for Y in $(seq 0 $LAST_Y); do
    ID=$(fromCoords $X $Y)
    echo "Lauching simulator at position ($X, $Y) with board id $ID"
    BOARD_ID=$ID ./de5Top | grep -v Warning &
    PIDS="$PIDS $!"
  done
done

# Run bridge board
HOST_ID=-1
echo "Lauching bridge board simulator at position ($HOST_X, $HOST_Y)" \
     "with board id $HOST_ID"
BOARD_ID=$HOST_ID ./de5BridgeTop &
PIDS="$PIDS $!"

PIDS=""
# Create horizontal links
for Y in $(seq 0 $LAST_Y); do
  for X in $(seq 0 $LAST_X); do
    A=$(fromCoords $X $Y)
    B=$(fromCoords $(($X+1)) $Y)
    if [ $(($X+1)) -lt $MESH_X ]; then
      $UDSOCK join "@tinsel.b$A.3" "@tinsel.b$B.4" &
      PIDS="$PIDS $!"
    fi
  done
done

# Create vertical links
for X in $(seq 0 $LAST_X); do
  for Y in $(seq 0 $LAST_Y); do
    A=$(fromCoords $X $Y)
    B=$(fromCoords $X $(($Y+1)))
    if [ $(($Y+1)) -lt $MESH_Y ]; then
      $UDSOCK join "@tinsel.b$A.1" "@tinsel.b$B.2" &
      PIDS="$PIDS $!"
    fi
  done
done

# Connect bridge board to mesh
ENTRY_ID=$(fromCoords 0 $(($MESH_Y-1)))
$UDSOCK join "@tinsel.b$ENTRY_ID.1" "@tinsel.b$HOST_ID.1" &
PIDS="$PIDS $!"

# On CTRL-C, call quit()
trap quit INT
function quit() {
  echo
  echo "Caught CTRL-C. Exiting."
  for PID in "$PIDS"; do
    kill $PID 2> /dev/null
  done
}

wait