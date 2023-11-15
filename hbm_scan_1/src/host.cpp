/*
*
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



#include "xcl2.hpp"
#include <algorithm>
#include <iostream>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <fstream>

#define NUM_KERNEL 16

// Number of HBM PCs required
#define MAX_HBM_PC_COUNT 32
#define PC_NAME(n) n | XCL_MEM_TOPOLOGY
const int pc[MAX_HBM_PC_COUNT] = {
    PC_NAME(0),  PC_NAME(1),  PC_NAME(2),  PC_NAME(3),  PC_NAME(4),  PC_NAME(5),  PC_NAME(6),  PC_NAME(7),
    PC_NAME(8),  PC_NAME(9),  PC_NAME(10), PC_NAME(11), PC_NAME(12), PC_NAME(13), PC_NAME(14), PC_NAME(15),
    PC_NAME(16), PC_NAME(17), PC_NAME(18), PC_NAME(19), PC_NAME(20), PC_NAME(21), PC_NAME(22), PC_NAME(23),
    PC_NAME(24), PC_NAME(25), PC_NAME(26), PC_NAME(27), PC_NAME(28), PC_NAME(29), PC_NAME(30), PC_NAME(31)};

// Function for verifying results
bool verify(std::vector<uint16_t, aligned_allocator<uint16_t> >& source_sw_results, //std::vector<bool>& source_sw_results,
            std::vector<uint16_t, aligned_allocator<uint16_t> >& source_hw_results, //std::vector<bool>& source_hw_results,
            unsigned int size) {
    bool check = true;
    for (size_t i = 0; i < size; i++) {
        if (source_hw_results[i] != source_sw_results[i]) {
            std::cout << "Error: Result mismatch" << std::endl;
            std::cout << "i = " << i << " CPU result = " << source_sw_results[i]
                      << " Device result = " << source_hw_results[i] << std::endl;
            check = false;
            break;
        }
    }
    return check;
}


int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <XCLBIN> \n", argv[0]);
        return -1;
    }
    
    //unsigned int dataSize = 67108849;
    unsigned int dataSize = 600000000;
    // unsigned int dataSize = 335544320; //32 * 1024 * 1024
    //unsigned int dataSize = 1024 * 1024 * 100; //599862170; //TODO: check and reduce size for first iteration
    
    // reducing the test data capacity to run faster in emulation mode
    if (xcl::is_emulation()) {
        dataSize = 128;
    }
    std::cout << "dataSize=" << dataSize << std::endl;
    unsigned int vector_size = dataSize/NUM_KERNEL;

    std::string binaryFile = argv[1];
    cl_int err;
    cl::Context context;
    cl::CommandQueue q;
    //cl::Kernel kernel_scan;
    std::string krnl_name = "krnl_scan";
    std::vector<cl::Kernel> krnls(NUM_KERNEL);
   
    // The get_xil_devices will return vector of Xilinx Devices
    auto devices = xcl::get_xil_devices();  //Do we need a device or devices??

    // read_binary_file() command will find the OpenCL binary file created using
    // the
    // V++ compiler load into OpenCL Binary and return pointer to file buffer.
    auto fileBuf = xcl::read_binary_file(binaryFile);

    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
    bool valid_device = false;
    for (unsigned int i = 0; i < devices.size(); i++) {
        auto device = devices[i];
        // Creating Context and Command Queue for selected Device
        OCL_CHECK(err, context = cl::Context(device, nullptr, nullptr, nullptr, &err));
        OCL_CHECK(err, q = cl::CommandQueue(context, device, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE, &err));

        std::cout << "Trying to program device[" << i << "]: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
        cl::Program program(context, {device}, bins, nullptr, &err);
        if (err != CL_SUCCESS) {
            std::cout << "Failed to program device[" << i << "] with xclbin file!\n";
        } else {
            std::cout << "Device[" << i << "]: program successful!\n";
            
	    for (int i = 0; i < NUM_KERNEL; i++) {
                std::string cu_id = std::to_string(i + 1);
                std::string krnl_name_full = krnl_name + ":{" + "krnl_scan_" + cu_id + "}";

                printf("Creating a kernel [%s] for CU(%d)\n", krnl_name_full.c_str(), i + 1);

                // Here Kernel object is created by specifying kernel name along with
                // compute unit.
                // For such case, this kernel object can only access the specific
                // Compute unit

                OCL_CHECK(err, krnls[i] = cl::Kernel(program, krnl_name_full.c_str(), &err));
            } 

	    //OCL_CHECK(err, kernel_scan = cl::Kernel(program, "krnl_scan", &err));
            valid_device = true;
            break; // we break because we found a valid device
        }
    }
    if (!valid_device) {
        std::cout << "Failed to program any device found, exit!\n";
        exit(EXIT_FAILURE);
    }


    std::vector<uint16_t, aligned_allocator<uint16_t> > l_shipdate[NUM_KERNEL];
    std::vector<uint16_t, aligned_allocator<uint16_t> > source_sw_results[NUM_KERNEL];
    std::vector<uint16_t, aligned_allocator<uint16_t> > source_hw_results[NUM_KERNEL];

    for (int i = 0; i < NUM_KERNEL; i++) {
        source_hw_results[i].resize(vector_size);
        source_sw_results[i].resize(vector_size);
        l_shipdate[i].resize(vector_size);
        
    }

    // Create the test data
    //for (int i = 0; i < NUM_KERNEL; i++) {
//	    std::generate(d_year[i].begin(), d_year[i].end(), std::rand);
//	    std::generate(lo_discount[i].begin(), lo_discount[i].end(), std::rand);
//	    std::generate(lo_quantity[i].begin(), lo_quantity[i].end(), std::rand);
  //  }

    //Reading the inputs from txt files
    uint16_t shipdate;
    std::vector<uint16_t> shipdate_row;
    std::ifstream f_l_shipdate("l_shipdate.txt");
 
    if(!f_l_shipdate.is_open()){
        std::cerr << "Error: unable to open l_shipdate.txt.txt" << std::endl;
    }   
    
    while(f_l_shipdate >> shipdate){
        shipdate_row.push_back(shipdate);
    }
    for (int i = 0; i < NUM_KERNEL; i++){
	    for (int j = 0; j < vector_size; j++){
	  	    l_shipdate[i][j] = shipdate_row[vector_size*i + j];
	    }
	    std::cout << "Completed reading for kernel " << i << std::endl;
    }
    std::cout << "Completed reading data from l_shipdate.txt file.\n";

    f_l_shipdate.close();

    for (size_t j = 0; j < NUM_KERNEL; j++){
	    for (size_t i = 0; i < vector_size; i++) {
            if(l_shipdate[j][i] == 288) {
                source_sw_results[j][i] = 1;
            }
            else {
                source_sw_results[j][i] = 0;
            }
	    }
    }
    std::cout << "Generated software results.\n";

    // Initializing output vectors to zero
    for (size_t i = 0; i < NUM_KERNEL; i++) {
        std::fill(source_hw_results[i].begin(), source_hw_results[i].end(), 0);
    }

    // Copying lines of code from the run_krnl 
    
    // For Allocating Buffer to specific Global Memory PC, user has to use
    // cl_mem_ext_ptr_t and provide the PCs
    //cl_mem_ext_ptr_t inBufExt1, inBufExt2, inBufExt3, outBufExt;
    
    std::vector<cl_mem_ext_ptr_t> inBufExt1(NUM_KERNEL);
    std::vector<cl_mem_ext_ptr_t> outBufExt(NUM_KERNEL);

    std::vector<cl::Buffer> buffer_input1(NUM_KERNEL);
    std::vector<cl::Buffer> buffer_output(NUM_KERNEL);

    for (int i = 0; i < NUM_KERNEL; i++) {
        inBufExt1[i].obj = l_shipdate[i].data();
        inBufExt1[i].param = 0;
        inBufExt1[i].flags = pc[i * 2];
        std::cout << "Assigning PC for inBufExt1 " << i << std::endl;

        outBufExt[i].obj = source_hw_results[i].data();
        outBufExt[i].param = 0;
        outBufExt[i].flags = pc[(i * 2) + 1];
        std::cout << "Assigning PC for outBufExt " << i << std::endl;
    }

    // These commands will allocate memory on the FPGA. The cl::Buffer objects can
    // be used to reference the memory locations on the device.
    // Creating Buffers
    for (int i = 0; i < NUM_KERNEL; i++) {
        OCL_CHECK(err, buffer_input1[i] = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR,
                                                sizeof(uint16_t) * vector_size, &inBufExt1[i], &err));
        OCL_CHECK(err, buffer_output[i] = cl::Buffer(context, CL_MEM_WRITE_ONLY | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR,
                                      		sizeof(uint16_t) * vector_size, &outBufExt[i], &err));
    }
    std::cout << "Allocated memoory on the FPGA creating buffers" << std::endl;

    double kernel_time_in_sec = 0, host_write_time_in_sec = 0, host_read_time_in_sec =0, result = 0;

    std::chrono::duration<double> host_write_time(0);
    auto host_write_start = std::chrono::high_resolution_clock::now();
    // Copy input data to Device Global Memory
    for (int i = 0; i < NUM_KERNEL; i++) {
        OCL_CHECK(err,
                  err = q.enqueueMigrateMemObjects({buffer_input1[i]}, 0 /* 0 means from host*/));
    }
    q.finish();
    auto host_write_end = std::chrono::high_resolution_clock::now();
    std::cout << "Copied input data to Device Global Memory" << std::endl;
    host_write_time = std::chrono::duration<double>(host_write_end - host_write_start);
    host_write_time_in_sec = host_write_time.count();
    std::cout << "host_write_time.count() = " << host_write_time_in_sec << " s" << std::endl;    

    std::chrono::duration<double> kernel_time(0);
    std::cout << "Starting the kernels now" << std::endl;
    auto kernel_start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_KERNEL; i++) {
        // Setting the k_scan Arguments
        OCL_CHECK(err, err = krnls[i].setArg(0, buffer_input1[i]));
        OCL_CHECK(err, err = krnls[i].setArg(1, buffer_output[i]));
        OCL_CHECK(err, err = krnls[i].setArg(2, vector_size));      

        // Invoking the kernel
        OCL_CHECK(err, err = q.enqueueTask(krnls[i]));
    }
    q.finish();
    auto kernel_end = std::chrono::high_resolution_clock::now();
    std::cout << "Finished executing the kernels" << std::endl;

    kernel_time = std::chrono::duration<double>(kernel_end - kernel_start);

    kernel_time_in_sec = kernel_time.count();
    std::cout << "kernel_time.count() = " << kernel_time_in_sec << " s" << std::endl;
    std::cout << "kernel_time.count/NUM_KERNEL = " << kernel_time_in_sec / NUM_KERNEL << std::endl;

    // Copy Result from Device Global Memory to Host Local Memory
    std::chrono::duration<double> host_read_time(0);
    auto host_read_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_KERNEL; i++) {
    	OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_output[i]}, CL_MIGRATE_MEM_OBJECT_HOST));
    }
    q.finish();
    auto host_read_end = std::chrono::high_resolution_clock::now();
    std::cout << "Copied the results from Device Global Memory to Host" << std::endl;
    host_read_time = std::chrono::duration<double>(host_read_end - host_read_start);
    host_read_time_in_sec = host_read_time.count();
    std::cout << "host_read_time.count() = " << host_read_time_in_sec << " s" <<std::endl;    

    bool match = true;
    
    for (int i = 0; i < NUM_KERNEL; i++) {
    	match = verify(source_sw_results[i], source_hw_results[i], vector_size);
    }
    std::cout << "Finished matching the software and hardware results" << std::endl;

    result = 1 * dataSize * sizeof(uint16_t) + dataSize * sizeof(uint16_t); //sizeof(bool);
    std::cout << "dataSize*2 = " << result << std::endl;
    result /= (1000 * 1000 * 1000); // to GB
    result /= kernel_time_in_sec;   // to GBps

    std::cout << "THROUGHPUT = " << result << " GB/s " << std::endl;

    std::cout << (match ? "TEST PASSED" : "TEST FAILED") << std::endl;
    return (match ? EXIT_SUCCESS : EXIT_FAILURE);
}