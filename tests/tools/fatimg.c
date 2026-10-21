/*
 * fatimg - small host tool to create and inspect FAT disk images with the
 * same FatFs version and configuration as the VCU firmware (plus f_mkfs).
 *
 * Used by the SD logging tests in the host simulation:
 *   fatimg mkfs <image> <size_mb>          create a formatted image
 *   fatimg put  <image> <name> <text>      create a file containing text
 *   fatimg ls   <image>                    list files in the root directory
 *   fatimg cat  <image> <name>             print a file
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "diskio.h"
#include "ff.h"

static FILE *img;

DSTATUS disk_status(BYTE pdrv)
{
    return (pdrv == 0 && img) ? 0 : STA_NOINIT;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    return disk_status(pdrv);
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    (void)pdrv;
    if (fseek(img, (long)sector * 512, SEEK_SET) != 0 ||
        fread(buff, 512, count, img) != count)
        return RES_ERROR;
    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    (void)pdrv;
    if (fseek(img, (long)sector * 512, SEEK_SET) != 0 ||
        fwrite(buff, 512, count, img) != count)
        return RES_ERROR;
    return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    (void)pdrv;
    switch (cmd)
    {
    case CTRL_SYNC: fflush(img); return RES_OK;
    case GET_SECTOR_COUNT:
        fseek(img, 0, SEEK_END);
        *(LBA_t *)buff = (LBA_t)(ftell(img) / 512);
        return RES_OK;
    case GET_SECTOR_SIZE: *(WORD *)buff = 512; return RES_OK;
    case GET_BLOCK_SIZE: *(DWORD *)buff = 1; return RES_OK;
    default: return RES_PARERR;
    }
}

static int fail(const char *what, FRESULT r)
{
    fprintf(stderr, "fatimg: %s failed (%d)\n", what, (int)r);
    return 1;
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "usage: fatimg mkfs|put|ls|cat <image> ...\n");
        return 2;
    }
    const char *cmd = argv[1];
    FATFS fs;
    FRESULT r;

    if (strcmp(cmd, "mkfs") == 0 && argc == 4)
    {
        long size = atol(argv[3]) * 1024L * 1024L;
        img = fopen(argv[2], "w+b");
        if (!img || fseek(img, size - 1, SEEK_SET) != 0 || fputc(0, img) == EOF)
        {
            perror("fatimg");
            return 1;
        }
        static BYTE work[4096];
        MKFS_PARM opt = { FM_FAT | FM_SFD, 0, 0, 0, 0 };
        r = f_mkfs("", &opt, work, sizeof(work));
        fclose(img);
        return r == FR_OK ? 0 : fail("f_mkfs", r);
    }

    img = fopen(argv[2], "r+b");
    if (!img)
    {
        perror("fatimg");
        return 1;
    }
    r = f_mount(&fs, "", 1);
    if (r != FR_OK)
        return fail("f_mount", r);

    if (strcmp(cmd, "put") == 0 && argc == 5)
    {
        FIL f;
        UINT bw;
        r = f_open(&f, argv[3], FA_WRITE | FA_CREATE_ALWAYS);
        if (r != FR_OK)
            return fail("f_open", r);
        r = f_write(&f, argv[4], (UINT)strlen(argv[4]), &bw);
        f_close(&f);
        return r == FR_OK ? 0 : fail("f_write", r);
    }
    if (strcmp(cmd, "ls") == 0)
    {
        DIR d;
        FILINFO fi;
        r = f_opendir(&d, "/");
        if (r != FR_OK)
            return fail("f_opendir", r);
        while (f_readdir(&d, &fi) == FR_OK && fi.fname[0])
            printf("%s %lu\n", fi.fname, (unsigned long)fi.fsize);
        f_closedir(&d);
        return 0;
    }
    if (strcmp(cmd, "cat") == 0 && argc == 4)
    {
        FIL f;
        char buf[512];
        UINT br;
        r = f_open(&f, argv[3], FA_READ);
        if (r != FR_OK)
            return fail("f_open", r);
        while (f_read(&f, buf, sizeof(buf), &br) == FR_OK && br > 0)
            fwrite(buf, 1, br, stdout);
        f_close(&f);
        return 0;
    }
    fprintf(stderr, "fatimg: bad command\n");
    return 2;
}
