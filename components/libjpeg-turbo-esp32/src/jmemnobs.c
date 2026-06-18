/*
 * jmemnobs.c
 *
 * This file was part of the Independent JPEG Group's software:
 * Copyright (C) 1992-1996, Thomas G. Lane.
 * libjpeg-turbo Modifications:
 * Copyright (C) 2017-2018, 2024, D. R. Commander.
 * For conditions of distribution and use, see the accompanying README.ijg
 * file.
 *
 * ESP32 modification: Route JPEG allocation pools to PSRAM via
 * heap_caps_malloc. This is required for artwork decoding on displays where
 * LVGL and networking leave very little internal SRAM available.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jmemsys.h"            /* import the system-dependent declarations */

#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#endif


/*
 * Memory allocation and freeing are controlled by the regular library
 * routines malloc() and free().
 */

GLOBAL(void *)
jpeg_get_small(j_common_ptr cinfo, size_t sizeofobject)
{
#ifdef ESP_PLATFORM
  void *p = heap_caps_malloc(sizeofobject, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (p == NULL)
    p = (void *)MALLOC(sizeofobject);
  return p;
#else
  return (void *)MALLOC(sizeofobject);
#endif
}

GLOBAL(void)
jpeg_free_small(j_common_ptr cinfo, void *object, size_t sizeofobject)
{
  free(object);
}


/*
 * "Large" objects are routed to PSRAM on ESP32 to avoid exhausting
 * the small internal SRAM heap.  Progressive JPEG decode requires
 * large coefficient buffers that can easily exceed 1MB.
 */

GLOBAL(void *)
jpeg_get_large(j_common_ptr cinfo, size_t sizeofobject)
{
#ifdef ESP_PLATFORM
  void *p = heap_caps_malloc(sizeofobject, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (p == NULL)
    p = (void *)MALLOC(sizeofobject);
  return p;
#else
  return (void *)MALLOC(sizeofobject);
#endif
}

GLOBAL(void)
jpeg_free_large(j_common_ptr cinfo, void *object, size_t sizeofobject)
{
#ifdef ESP_PLATFORM
  heap_caps_free(object);
#else
  free(object);
#endif
}


/*
 * This routine computes the total memory space available for allocation.
 */

GLOBAL(size_t)
jpeg_mem_available(j_common_ptr cinfo, size_t min_bytes_needed,
                   size_t max_bytes_needed, size_t already_allocated)
{
  if (cinfo->mem->max_memory_to_use) {
    if ((size_t)cinfo->mem->max_memory_to_use > already_allocated)
      return cinfo->mem->max_memory_to_use - already_allocated;
    else
      return 0;
  } else {
    /* Here we always say, "we got all you want bud!" */
    return max_bytes_needed;
  }
}


/*
 * Backing store (temporary file) management.
 * Since jpeg_mem_available always promised the moon,
 * this should never be called and we can just error out.
 */

GLOBAL(void)
jpeg_open_backing_store(j_common_ptr cinfo, backing_store_ptr info,
                        long total_bytes_needed)
{
  ERREXIT(cinfo, JERR_NO_BACKING_STORE);
}


/*
 * These routines take care of any system-dependent initialization and
 * cleanup required.  Here, there isn't any.
 */

GLOBAL(long)
jpeg_mem_init(j_common_ptr cinfo)
{
  return 0;                     /* just set max_memory_to_use to 0 */
}

GLOBAL(void)
jpeg_mem_term(j_common_ptr cinfo)
{
  /* no work */
}
