#include <atad.h>
//#include <dev9.h>

#include <loadcore.h>
#include <mcman.h>
#include <thbase.h>
#include <thsemap.h>

#include <ps2ip.h>
#include <stdio.h>
#include <sysclib.h>
#include "lwNBD/nbd_server.h"

#define MODNAME "hdldsvr"
IRX_ID(MODNAME, 1, 1);

struct irx_export_table _exp_hdldsvr;
void tcp_server_thread(void *args);
static int tcp_server_tid;



/****************************************************************************************/

int hdd_fake_init(struct nbd_context *ctx)
{
    ctx->export_size = 1073741824;
    return 0;
}

/* return ascii formated buffer with offset so 2 by 16 bytes columns for hex dumpers
 * slow too because still not one block read operation */
int hdd_fake_read1(void *buffer, uint64_t offset, uint32_t length)
{
    register uint32_t i = 0;
    register uint32_t len = length * 512;
    uint64_t *pbuf = buffer;

    while (i < len) {
        *pbuf = offset + i;
        i += sizeof(uint64_t);
        pbuf++;
    }
    return 0;
}

/* return buffer with offset so 2 by 16 bytes columns for hex dumpers
 * slow too because still not one block read operation */
//int hdd_fake_read2(void *buffer, uint64_t offset, uint32_t length)
//{
//    register uint32_t i = 0;
//    register uint32_t len = length * 512;
//
//    static int count = 0;
//    for (i = 0; i < len; i++) {
//        if (i % sizeof(uint64_t) == 0)
//            sprintf(buffer + i, "%08llx", offset + i);
//    }
//    return 0;
//}

/* return buffer with offset in the first bytes of the bloc.
 * make only first int per block */
int hdd_fake_read_fast(void *buffer, uint64_t offset, uint32_t length)
{
    register uint32_t i = 0;
    register uint32_t len = length * 512;
    uint64_t *pbuf = buffer;

    while (i < len) {
        *pbuf = offset + i;
        i += 512;
        pbuf += 512;
    }
    return 0;
}

int hdd_fake_write(void *buffer, uint64_t offset, uint32_t length)
{
    return 0;
}

struct nbd_context hdd_fake =
    {
        .export_name = "hdd",
        .export_desc = "Fake hdd",
        .blocksize = 512,
        .export_init = hdd_fake_init,
        .read = hdd_fake_read_fast,
        .write = hdd_fake_write,
};

/****************************************************************************************/

int hdd_atad_init(struct nbd_context *ctx)
{
    ata_devinfo_t *dev_info = ata_get_devinfo(0);
    if (dev_info != NULL && dev_info->exists) {
        ctx->export_size = (uint64_t)dev_info->total_sectors * ctx->blocksize;
        return 0;
    }
    return 1;
}

int hdd_atad_read(void *buffer, uint64_t offset, uint32_t length)
{
    return ata_device_sector_io(0, buffer, (uint32_t)offset, length, ATA_DIR_READ);
}

int hdd_atad_write(void *buffer, uint64_t offset, uint32_t length)
{
    return ata_device_sector_io(0, buffer, (uint32_t)offset, length, ATA_DIR_WRITE);
}

int hdd_atad_flush(void)
{
    return ata_device_flush_cache(0);
}

struct nbd_context hdd_atad =
    {
        .export_name = "hdd",
        .export_desc = "PlayStation 2 HDD via ATAD",
        .blocksize = 512,
        .export_init = hdd_atad_init,
        .read = hdd_atad_read,
        .flush = hdd_atad_flush,
};


/****************************************************************************************/

//int hdd_dev9_init(struct nbd_context *ctx)
//{
//    ata_devinfo_t *dev_info = ata_get_devinfo(0);
//    if (dev_info != NULL && dev_info->exists) {
//        ctx->export_size = (uint64_t)dev_info->total_sectors * ctx->blocksize;
//        return 0;
//    }
//    return 1;
//}
//
//int hdd_dev9_read(void *buffer, uint64_t offset,uint32_t length)
//{
//	// int dev9DmaTransfer(int ctrl, void *buf, int bcr, int dir)
////    return dev9DmaTransfer(0, buffer, ,
//	return 0;
//
//}
//
//int hdd_dev9_write(void *buffer, uint64_t offset,uint32_t length)
//{
//    return 0;
//}
//
//int hdd_dev9_flush(void)
//{
//	return (0);
//}
//
//struct nbd_context hdd_dev9 =
//    {
//        .export_name = "hdd",
//        .export_desc = "PlayStation 2 HDD via DEV9",
//        .blocksize = 512,
//        .export_init = hdd_dev9_init,
//        .read = hdd_dev9_read,
//        .write = hdd_dev9_write,
//		.flush = hdd_dev9_flush,
//};

/****************************************************************************************/

int mc_mcman_init(struct nbd_context *ctx)
{
    int cardsize;
    u8 flags;
    u16 blocksize;
    s16 pagesize;

    if (McGetCardSpec(0, 0, &pagesize, &blocksize, &cardsize, &flags) != 0)
        ;
    return 1;

    ctx->blocksize = (uint16_t)pagesize;
    ctx->export_size = (uint64_t)cardsize;
    return 0;
}

int mc_mcman_read(void *buffer, uint64_t offset, uint32_t length)
{
    return McReadPage(0, 0, offset, buffer);
}

int mc_mcman_write(void *buffer, uint64_t offset, uint32_t length)
{
    return 0;
    //    return McWritePage(int port, int slot, int page, void *pagebuf, void *eccbuf);
}

struct nbd_context mc_mcman =
    {
        .export_name = "memorycard",
        .export_desc = "PlayStation 2 Memory Card via MCMAN",
        .export_init = mc_mcman_init,
        .read = mc_mcman_read,
        .write = mc_mcman_write,
};

/****************************************************************************************/

int _start(int argc, char **argv)
{
    iop_thread_t thread_param;

    // register exports
    RegisterLibraryEntries(&_exp_hdldsvr);
    thread_param.attr = TH_C;
    thread_param.thread = (void *)tcp_server_thread;
    thread_param.priority = 0x10;
    thread_param.stacksize = 0x1000;
    thread_param.option = 0;
    tcp_server_tid = CreateThread(&thread_param);
    StartThread(tcp_server_tid, 0);
    return MODULE_RESIDENT_END;
}

int _shutdown(void)
{
    DeleteThread(tcp_server_tid);
    return 0;
}

void tcp_server_thread(void *args)
{
	//TODO : platform specific block device detection then nbd_context initialization go here

    struct nbd_context *ctx = &hdd_atad;
    //TODO : many export in a loop
    if (ctx->export_init(ctx) != 0)
        return;
    nbd_init(ctx);
}
