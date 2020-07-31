// SPDX-License-Identifier: BSD-2-Clause
#include <stdint.h>
#include "model.h"
/***************************************************
 * Edit values between here ->
 * ************************************************/

// Model

// HMM States Labels
const uint8_t hmm_labels[NOOFSTATES][NOOFOBS] = { 
    { 0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0 },
    { 0,1,0,0,0,1,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    { 0,0,0,0,0,0,0,0,1,0,0,1,0,0,1,0,0,1,0,1,0,0,0,0 },
    { 0,0,0,1,0,0,1,1,1,1,0,0,0,1,1,0,0,1,0,0,1,0,0,0 },
    { 0,0,0,0,0,0,1,0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,0,1 },
    { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0 },
    { 1,0,0,0,0,0,0,1,0,0,0,1,0,0,0,1,0,1,0,0,0,0,0,0 },
    { 0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,0,0,0,1,0 },
    { 0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,1,0,0,0,0,0,0 },
    { 0,1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0,1,0,1,0,0,1,0 },
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0 },
    { 1,0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0 },
    { 0,0,1,1,0,0,1,0,0,1,0,0,0,0,0,0,1,1,0,1,0,1,0,0 },
    { 0,0,0,0,0,0,1,0,0,0,0,1,0,0,1,0,1,0,0,1,0,1,0,0 },
    { 1,1,0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    { 0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,1,1,0 },
    { 0,0,0,0,0,0,1,0,1,0,0,0,0,1,1,0,0,0,1,1,0,0,0,0 },
    { 0,1,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1 },
    { 0,0,0,1,1,1,1,0,0,0,0,0,0,1,0,0,0,0,1,1,0,1,0,0 },
    { 0,0,0,0,1,1,1,0,0,0,1,0,1,0,1,0,0,0,0,0,1,0,1,0 },
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1,0,0,0,1,0 },
    { 0,0,0,0,0,1,0,1,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0 },
    { 0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,1,0,0,0,1,0,0,0,0 },
    { 0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0 },
    { 0,0,1,0,1,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0 },
    { 0,0,0,0,1,0,0,0,1,1,0,0,0,0,0,0,1,0,0,0,0,0,1,1 },
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,1,0,0 },
    { 0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,1,0,1,0,0,0 },
    { 0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0 },
    { 0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0 },
    { 0,1,0,0,1,0,0,0,0,0,1,1,0,1,1,0,1,0,1,0,0,0,0,0 },
    { 0,0,0,1,0,0,0,0,1,1,0,0,1,0,0,0,0,0,0,1,0,0,0,0 },
    { 0,0,0,0,0,1,1,0,0,1,0,1,0,1,0,0,0,1,0,0,1,0,1,0 },
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,1,1,1 },
    { 0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0 },
    { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0 },
    { 1,0,0,0,0,0,1,1,0,0,1,0,0,0,1,0,1,1,0,0,1,0,0,1 },
    { 0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,1,0,0,1,1,1 },
    { 0,0,0,0,1,1,0,0,0,0,0,1,0,1,1,1,1,0,1,0,0,1,0,0 },
    { 1,0,1,1,1,0,1,0,0,0,0,0,0,0,1,1,0,0,0,0,0,1,0,0 },
    { 0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,1,1,0,0,0,1,0 },
    { 0,1,0,0,0,0,1,0,0,0,0,1,1,0,0,1,0,0,0,0,0,1,0,0 },
    { 0,0,0,0,1,0,1,1,0,0,0,0,0,1,1,1,0,0,0,0,1,0,0,0 },
    { 0,0,0,0,1,1,0,1,0,0,1,0,0,0,0,1,0,1,0,1,0,0,0,0 },
    { 0,1,1,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0 },
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,1,0,0,0,0,0,0,0 },
    { 0,0,0,0,0,0,1,1,0,0,0,1,1,0,0,1,0,0,1,0,0,0,1,1 },
    { 0,0,0,0,0,1,0,0,0,0,0,0,1,0,0,1,0,1,1,0,0,0,0,0 },
    { 1,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0 },
    { 0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,0,0,0,1,0,0,0 },
    { 0,1,0,0,0,1,1,0,0,1,1,0,0,0,0,1,0,0,1,0,0,0,0,0 },
    { 1,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,1,0,0 },
    { 0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,1,0,0,0,0,0,0 },
    { 0,0,0,0,1,0,0,0,0,0,0,0,0,1,0,1,0,1,0,0,0,0,1,0 },
    { 1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0 },
    { 0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1 },
    { 0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1,0,1,0,0,1,0 },
    { 0,0,1,0,0,0,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,1,0 },
    { 0,0,0,0,0,0,1,1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    { 1,1,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    { 0,0,0,1,0,0,1,1,0,0,0,0,1,0,0,1,0,0,1,1,0,0,0,0 },
    { 0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0 },
    { 1,0,0,0,1,1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0,1,0,1 },
    { 0,1,0,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    { 1,0,0,0,0,0,0,0,0,1,0,1,0,1,0,0,1,0,0,1,1,1,0,0 },
    { 1,0,0,0,0,0,0,0,0,0,1,0,0,0,1,1,1,0,0,0,0,0,0,0 },
    { 0,0,0,1,0,0,1,0,0,0,0,0,1,1,0,0,0,1,0,0,1,0,0,0 }
};

// Target Markers 
const uint32_t observation[NOOFTARGMARK][2] = { 
    { 0, 0 },
    { 10, 0 },
    { 20, 0 },
    { 24, 0 }
};

// dm -> Genetic Distances 
const float dm[NOOFOBS-1] = {
0.00002776575491,
0.00002690985924,
0.00003359387536,
0.00002343341916,
0.00001740272650,
0.00002932712902,
0.00000263127713,
0.00003006697192,
0.00003750852002,
0.00003208623393,
0.00000163991174,
0.00003037015269,
0.00000415398870,
0.00000032030073,
0.00001472361748,
0.00000236136434,
0.00000610464863,
0.00003326050971,
0.00002812795832,
0.00000361174801,
0.00003356920575,
0.00001108695991,
0.00004428544523
};

/***************************************************
 * <- And here
 * ************************************************/

