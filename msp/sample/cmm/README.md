说明：
CMM接口参考用例

执行：
/opt/bin/sample_cmm


测试项描述：
ax_mem_cmm_test_001：测试 MemAlloc/MemFree

ax_mem_cmm_test_002：测试 AX_SYS_MemAllocCached/AX_SYS_MemFree

ax_mem_cmm_test_003：测试 AX_SYS_GetBlockInfoByVirt/AX_SYS_GetBlockInfoByPhy是否支持non-cached VirtAddr

ax_mem_cmm_test_004：测试 AX_SYS_Mmap/AX_SYS_Munmap

ax_mem_cmm_test_005：测试 AX_SYS_MmapCache/AX_SYS_MflushCache/AX_SYS_Munmap

ax_mem_cmm_test_006：测试 AX_SYS_MmapCache/AX_SYS_MinvalidateCache/AX_SYS_Munmap

ax_mem_cmm_test_007：测试 AX_SYS_MflushCache。

ax_mem_cmm_test_008：测试 AX_SYS_MinvalidateCache。

ax_mem_cmm_test_009：测试 AX_SYS_MflushCache是否支持对偏移地址操作case 1。

ax_mem_cmm_test_010：测试 AX_SYS_MflushCache是否支持对偏移地址操作case 2。

ax_mem_cmm_test_011：测试 AX_SYS_MflushCache是否支持对偏移地址操作case 3。

ax_mem_cmm_test_012：本case期望返回failure!!! 测试 AX_SYS_MfushCache是否支持对偏移地址操作case 4。

ax_mem_cmm_test_013：测试 AX_SYS_MmapCache/AX_SYS_MflushCache/AX_SYS_Munmap是否支持偏移地址操作。

ax_mem_cmm_test_014：本case期望返回failure!!! 测试 AX_SYS_MmapCache/AX_SYS_MflushCache/AX_SYS_Munmap是否支持偏移地址操作。

ax_mem_cmm_test_015：测试 AX_SYS_MmapCache/AX_SYS_MflushCache/AX_SYS_Munmap 非托管模式

ax_mem_cmm_test_016：测试 AX_SYS_MmapCache/AX_SYS_MinvalidateCache/AX_SYS_Munmap 非托管模式

ax_mem_cmm_test_017：测试 AX_SYS_GetBlockInfoByVirt/AX_SYS_GetBlockInfoByPhy是否支持cache VirtAddr

ax_mem_cmm_test_018：测试 AX_SYS_GetBlockInfoByVirt/AX_SYS_GetBlockInfoByPhy是否支持AX_SYS_Mmap出的VirtAddr

ax_mem_cmm_test_019：测试 AX_SYS_GetBlockInfoByVirt/AX_SYS_GetBlockInfoByPhy是否支持AX_SYS_MmapCache出的VirtAddr

ax_mem_cmm_test_020：测试 AX_SYS_Mmap/AX_SYS_MmapCache对缓存池中单个block做映射。

ax_mem_cmm_test_021：测试 AX_SYS_MmapFast接口多次调用返回的虚拟地址是否一致。

ax_mem_cmm_test_022：测试 AX_SYS_MmapCacheFast接口多次调用返回的虚拟地址是否一致。

ax_mem_cmm_test_023：测试 AX_SYS_MmapCacheFast/AX_SYS_MflushCache/AX_SYS_Munmap

ax_mem_cmm_test_024：测试 AX_SYS_MmapCacheFast/AX_SYS_MinvalidateCache/AX_SYS_Munmap

备注：
ax_mem_cmm_test_012和ax_mem_cmm_test_014是故意构造的特殊案例，我们期望其返回的结果就是fail，
目的是为了反向验证flush接口是否真正有效。有时候单次运行不一定能测出fail，可多测试几次。