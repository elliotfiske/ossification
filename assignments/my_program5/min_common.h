/*
//  min_common.h
//  assignments
//
//  Created by Elliot Fiske on 12/4/15.
//  Copyright © 2015 Elliot Fiske. All rights reserved.
*/

#ifndef min_common_h
#define min_common_h

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <libgen.h>
#include <stdlib.h>

/** Reasonable size for one filename */
#define ARG_LENGTH 100
/** Max length for path */
#define MAX_PATH_LENGTH 32768

/** Only print if -v is around */
void d_printf(int vflag, const char *format, ...);

/********************************************************************/
/************************ FUN MINIX STRUCTS *************************/
/********************************************************************/

/* Magic number and offsets */
#define START_OF_SUPERBLOCK 1024
#define MINIX_MAGIC_NUMBER 0x4D5A
#define VALID_PARTITION_CHECK 510
#define BYTE510 0x55
#define BYTE511 0xAA
#define PARTITION_TABLE_LOC 0x1BE
#define PARTITION_TYPE 0x81
#define SECTOR_SIZE 512


typedef struct superblock { /* Minix Version 3 Superblock
                     * this structure found in fs/super.h
                     * in minix 3.1.1
                     */
   /* ~~ON DISK~~ These fields and orientation are non–negotiable */
   uint32_t ninodes; /* number of inodes in this filesystem */
   uint16_t pad1; /* make things line up properly */
   int16_t i_blocks; /* # of blocks used by inode bit map */
   int16_t z_blocks; /* # of blocks used by zone bit map */
   uint16_t firstdata; /* number of first data zone */
   int16_t log_zone_size; /* log2 of blocks per zone */
   int16_t pad2; /* make things line up again */
   uint32_t max_file; /* maximum file size */
   uint32_t zones; /* number of zones on disk */
   int16_t magic; /* magic number */
   int16_t pad3; /* make things line up again */
   uint16_t blocksize; /* block size in bytes */
   uint8_t subversion; /* filesystem sub–version */
} __attribute__ ((__packed__)) superblock_t;

#define DIRECT_ZONES 7
#define MAX_DIRECTORY_ENTRIES 512
#define DIRECTORY_ENTRY_SIZE_BYTES 64

typedef struct inode {
   uint16_t mode; /* mode */
   uint16_t links; /* number or links */
   uint16_t uid;
   uint16_t gid;
   uint32_t size;
   int32_t atime;
   int32_t mtime;
   int32_t ctime;
   uint32_t zone[DIRECT_ZONES];
   uint32_t indirect;
   uint32_t two_indirect;
   uint32_t unused;
} inode_t;

/** Mode bit definitions */
#define FILE_TYPE_MASK 0170000
#define REGULAR_FILE 0100000
#define DIRECTORY 0040000
#define OWNER_READ_PERMISSION 0000400
#define OWNER_WRITE_PERMISSION 0000200
#define OWNER_EXECUTE_PERMISSION 0000100
#define GROUP_READ_PERMISSION 0000040
#define GROUP_WRITE_PERMISSION 0000020
#define GROUP_EXECUTE_PERMISSION 0000010
#define OTHER_READ_PERMISSION 0000004
#define OTHER_WRITE_PERMISSION 0000002
#define OTHER_EXECUTE_PERMISSION 0000001

typedef struct partition_entry {
   uint8_t bootind; /* Boot magic number (0x80 if bootable) */
   uint8_t start_head; /* Start of partition in CHS */
   uint8_t start_sec;
   uint8_t start_cyl;
   uint8_t type; /* Type of partition (0x81 is minix) */
   uint8_t end_head; /* End of partition in CHS */
   uint8_t end_sec;
   uint8_t end_cyl;
   uint32_t lFirst; /* First sector (LBA addressing) */
   uint32_t size;
} __attribute__ ((__packed__)) partition_entry_t;

typedef struct directory_entry {
   uint32_t inode_num;
   unsigned char name[60];
} directory_entry_t;

/*** 
 
 EXPORTED METHODS
 
 ***/

FILE *parse_arguments(int argc, char *argv[],
                      int *verbose_flag,
                      int *partition_num, int *subpartition_num,
                      char **path_1, char **path_2);

uint32_t get_partition_offset(int partition_num, int subpartition_num,
                              FILE *image_file, uint32_t subpartition_offset);

superblock_t parse_superblock(uint32_t base_offset, FILE *image_file);

inode_t *inode_from_inode_num(int32_t inode_num, superblock_t superblock,
                              uint32_t base_offset, FILE *image_file);

int directory_entries_from_inode(inode_t *inode, FILE *image_file,
                                 superblock_t superblock, uint32_t base_offset,
                                 directory_entry_t **result);

inode_t *get_inode_from_path(char *path, superblock_t superblock,
                             uint32_t base_offset, FILE *image_file,
                             char **file_name);


#endif /* min_common_h */
