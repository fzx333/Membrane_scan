/**
* Copyright (C) 2019-2021 Xilinx, Inc
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/

#include <stdint.h>

//#define VDATA_SIZE 16
#define VDATA_SIZE 32 //599862170 // TODO: reduce this, this is the number of elements in chunk array ( does it leads to some optimization? )

// TRIPCOUNT indentifier
const unsigned int c_dt_size = VDATA_SIZE;

typedef struct v_datatype { uint16_t data[VDATA_SIZE]; } v_dt;
typedef struct bit_datatype { uint16_t data[VDATA_SIZE]; } bit_dt; //TODO alignment issues with bool , use uint8_t

extern "C" {
void krnl_scan(const v_dt* in1,        // Read-Only Vector 1
                const v_dt* in2,
                const v_dt* in3,
                const v_dt* in4,
                const v_dt* in5,
                bit_dt* out_r,            // Output Bitmap
                const unsigned int size // Size in integer
               ) {

#pragma HLS INTERFACE m_axi port = in1 offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = in2 offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = in3 offset = slave bundle = gmem2
#pragma HLS INTERFACE m_axi port = in4 offset = slave bundle = gmem3
#pragma HLS INTERFACE m_axi port = in5 offset = slave bundle = gmem4
#pragma HLS INTERFACE m_axi port = out_r offset = slave bundle = gmem0

#pragma HLS INTERFACE s_axilite port = in1
#pragma HLS INTERFACE s_axilite port = in2
#pragma HLS INTERFACE s_axilite port = in3
#pragma HLS INTERFACE s_axilite port = in4
#pragma HLS INTERFACE s_axilite port = in5
#pragma HLS INTERFACE s_axilite port = out_r
#pragma HLS INTERFACE s_axilite port = size
#pragma HLS INTERFACE s_axilite port = return

	unsigned int vSize = ((size - 1) / VDATA_SIZE) + 1; //check this

	//to avoid the conditional memory acceses for in2 and in3 
	bit_dt temp1;
    bit_dt temp2;
    bit_dt temp3;
    bit_dt temp4;
    bit_dt temp5;

// Auto-pipeline is going to apply pipeline to this loop
scan1:
    for (int i = 0; i < vSize; i++) {
    scan2:
        for (int k = 0; k < VDATA_SIZE; k++) {
#pragma HLS unroll    
#pragma HLS LOOP_TRIPCOUNT min = c_dt_size max = c_dt_size
            //out_r[i].data[k] = (in1[i].data[k] == 1993 && 1 <= in2[i].data[k] &&  in2[i].data[k] <= 3 && in3[i].data[k] < 25)?1:0;
	        temp1.data[k] = (in1[i].data[k] == 7) ? 1 : 0;
            temp2.data[k] = (in2[i].data[k] == 105) ? 1 : 0;
            temp3.data[k] = (in3[i].data[k] == 105) ? 1 : 0;
            temp4.data[k] = (in4[i].data[k] == 15) ? 1 : 0;
            temp5.data[k] = (in5[i].data[k] == 15) ? 1 : 0;
            out_r[i].data[k] = temp1.data[k];
        }
    }
}
}
