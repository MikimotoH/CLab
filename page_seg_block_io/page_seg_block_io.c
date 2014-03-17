#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <syslog.h>

#define OPTIONAL

struct pi_opt
{
    uint32_t scsi_skcq;
};
typedef struct pi_opt pi_opt_t;
typedef uint32_t vol_id_t;
typedef uint64_t LBA_t;
typedef int      status_t;
//A Type, Length, Value tuple
struct tlv
{
    uint8_t  type;
    uint16_t length;
    uint8_t  value[];
};
typedef void bplm_callback_fn_t(status_t status, void *client_data);
struct picoral_dma;
typedef int (*bplm_rw_fn_t)(vol_id_t, LBA_t, uint32_t, struct picoral_dma *, struct tlv *, bplm_callback_fn_t, void *);
typedef uint64_t bus_addr_t;

#define OPT_TERMINATED    0    // operation is terminated
#define OPT_WAITING       1    // operation is waiting for callback from BPLM

/* ==========================================================================*/
#define BAD_ENDING(skcq,msg, ...) \
    ({\
        LOGINF(msg, ## __VA_ARGS__);\
        opt->scsi_skcq.u32_value = skcq;\
        OPT_TERMINATED;\
    })
#define GOOD_ENDING(MSG, ...)\
    ({\
         PI_HOST_LOG(PI_LOG_DEBUG, opt->host, msg, ## __VA_ARGS__);\
         opt->scsi_skcq.u32_value = SCSI_SKCQ_GOOD;\
         OPT_TERMINATED;\
    )}

static int32_t
opt_rw(pi_opt_t *opt,
        uint64_t start_block,
        uint32_t num_blocks,
        bplm_rw_fn_t bplm_fn,
        uint8_t   *dma_data,
        bus_addr_t dma_addr)
{
    int ret = 0;
    const uint32_t block_size = 512;
    const uint32_t bytes_per_page = 1024*1024;
    const uint32_t blocks_per_page= 1024*1024/block_size;//blocks per page

    uint64_t end_block = start_block + num_blocks;
    // xfer_blocks in CDB is number of blocks.
    if (end_lba > opt->volume->num_block || end_lba < start_lba)
        return BAD_ENDING(SCSI_SKCQ_LBA_OUT_OF_RANGE, "Last LBA (%lu) is "
                "over maximum LBA of the volume.\n",
                start_lba + xfer_blocks );


    {
        uint64_t b_pos = start_block;
        bus_addr_t dma_addr_cur = dma_addr;
        uint8_t *dma_data_cur = dma_data;

        while( b_pos < end_block )
        {
            uint64_t page_index   = b_pos / blocks_per_page;
            uint64_t block_offset = b_pos % blocks_per_page;
            uint32_t b_seg = MIN(end_block, (page_index+1)*blocks_per_page) - b_pos;
            uint32_t bytes_xfer = b_seg*block_size;
            bool is_last = ( end_block < ((page_index+1)*blocks_per_page));
            ret = bplm_rw_submit(
                    volume_id,
                    b_pos,
                    block_size,
                    bytes_xfer,
                    dma_data_cur,
                    dma_addr_cur,
                    bplm_fn,
                    opt,
                    is_last );
            if(is_last)
                LOGINF("Last BPLM api of the r/w command is called. (Last transfer size = %u, return %d)\n", bytes_xfer, ret );
            if( ret != 0 )
                break;

            if (dma_data_cur)
                dma_addr_cur += bytes_xfer, dma_data_cur += bytes_xfer;
            
            b_pos += b_seg;
        }
    }


    if( ret != 0 )
        LOGERR( "BPLM return error (%d).\n", ret );

    return OPT_WAITING;
}

