# HBM scan 1

This is the implementation for scan of 1 input columns with HBM.

Command I used to run the code:

```
nohup make run TARGET=hw PLATFORM=/opt/xilinx/platforms/xilinx_u280_gen3x16_xdma_1_202211_1/xilinx_u280_gen3x16_xdma_1_202211_1.xpfm & disown
```

Commands I used to clean all the run-time files:

```
make cleanall PLATFORM=/opt/xilinx/platforms/xilinx_u280_gen3x16_xdma_1_202211_1/xilinx_u280_gen3x16_xdma_1_202211_1.xpfm
rm -rf .ipcache/ .run/ xrt.run_summary nohup.out
```
