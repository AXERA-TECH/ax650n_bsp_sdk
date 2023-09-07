/*
 * Copyright (c) 2019 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc.  They may be adapted and modified by bona fide
 * purchasers for internal use, but neither the original nor any adapted
 * or modified version may be disclosed or distributed to third parties
 * in any manner, medium, or form, in whole or in part, without the prior
 * written consent of Cadence Design Systems Inc.  This software and its
 * derivatives are to be executed solely on products incorporating a Cadence
 * Design Systems processor.
 */

#ifndef __TMUTILS_H__
#define __TMUTILS_H__


/*******************************************************
*   X V M E M    S T R U C T U R E S
*******************************************************/
typedef enum
{
  /*! No Error */
  XVMEM_OK                      = 0,
  /*! User provided pool for the heap2 memory manager
   * is not aligned to the required block size. */
  XVMEM_ERROR_POOL_NOT_ALIGNED  = -1,
  /*! User provided pool does not have the required size. */
  XVMEM_ERROR_POOL_SIZE         = -2,
  /*! User provided pool is NULL. */
  XVMEM_ERROR_POOL_NULL         = -3,
  /*! The block size for heap2 memory manager is not a power of 2. */
  XVMEM_ERROR_BLOCK_SIZE        = -4,
  /*! Unsupported memory manager type. */
  XVMEM_ERROR_UNKNOWN_HEAP_TYPE = -5,
  /*! Failed to allocate the requested buffer. */
  XVMEM_ERROR_ALLOC_FAILED      = -6,
  /*! Alignment of the request buffer has to be a power of 2. */
  XVMEM_ERROR_ILLEGAL_ALIGN     = -7,
  /*! Requested allocation from an uninitialized memory manager object. */
  XVMEM_ERROR_UNINITIALIZED     = -8,
  /*! Encountered an internal error */
  XVMEM_ERROR_INTERNAL          = -100
} xvmem_status_t;

#define XVMEM_HEAP3_DEFAULT_NUM_BLOCKS      (32)
#define XVMEM_HEAP3_MIN_ALLOC_SIZE          (4)
#define XVMEM_HEAP3_BLOCK_STRUCT_SIZE_LOG2  (4)

#define XVMEM_INITIALIZED                   (0x1234abcd)

/* Struct to maintain list of free/allocated blocks for type 3 heap. */
typedef struct xvmem_block_struct
{
  /* next block in the list */
  struct xvmem_block_struct *_next_block;                // next block in the list
  int32_t                   _block_size;                 // size of the block
  void                      *_buffer;                    // ptr to the buffer
  void                      *_aligned_buffer;            // aligned ptr to buffer
} xvmem_block_t;

/* Type 3 heap manager */
typedef struct
{
  uint32_t      _initialized;
  void          *_buffer;                                       // user provided pool for allocation
  int32_t       _buffer_size;                                   // num bytes in pool
  int32_t       _free_bytes;                                    // free bytes in the heap
  int32_t       _allocated_bytes;                               // allocated bytes in the heap
  int32_t       _unused_bytes;                                  // unused bytes in the heap
  xvmem_block_t _free_list_head;                                // sentinel for the free list
  xvmem_block_t _free_list_tail;                                // sentinel for the free list
  xvmem_block_t _alloc_list_head;                               // sentinel for the allocated list
  xvmem_block_t *_blocks;                                       // list of blocks
  uint32_t      _num_blocks;                                    // number of blocks
  uint32_t      *_block_free_bitvec;                            // bitvec to mark free/alloc blocks
  uint16_t      _header_size;                                   // size of head in bytes
  uint16_t      _has_header;                                    // is the block info part of buffer
} xvmem_mgr_t;

/*******************************************************
*   I N T E R N A L    F U N C T I O N S
*******************************************************/
xvmem_status_t xvmem_init(xvmem_mgr_t *mgr, void *buf, int32_t size, uint32_t num_blocks, void *header);
void *xvmem_alloc(xvmem_mgr_t *mgr, size_t size, uint32_t align, xvmem_status_t *err_code);
void xvmem_free(xvmem_mgr_t *mgr, void const *p);

void copyBufferEdgeDataH(uint8_t * __restrict srcPtr, uint8_t * __restrict dstPtr, int32_t widthBytes, int32_t pixWidth, int32_t height, int32_t pitchBytes, uint8_t paddingType, uint32_t paddingVal);
void copyBufferEdgeDataV(uint8_t const* __restrict srcPtr, uint8_t * __restrict dstPtr, int32_t width, int32_t pixWidth, int32_t height, int32_t pitchBytes, uint8_t paddingType, uint32_t paddingVal);



#endif

