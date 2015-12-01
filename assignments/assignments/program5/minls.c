//
//  minls.c
//  
//
//  Created by Jack Wang on 11/30/15.
//
//

#include "minls.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define PARTITION_TABLE_LOC 0x1BE
#define PARTITION_TYPE 0x81
#define BYTE510 0x55
#define BYTE11 0xAA
#define MINIX_MAGIC_NUMBER 0x4D5A
#define MINIX_MAGIC_NUMBER_REVERSED 0x5A4D
#define INODE_SIZE_BYTES 64
#define DIRECTORY_ENTRY_SIZE_BYTES 64

#define DIRECT_ZONES 7

struct superblock { /* Minix Version 3 Superblock
   * this structure found in fs/super.h
   * in minix 3.1.1
   */
   /* on disk. These fields and orientation are non–negotiable */
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
};

struct inode {
   uint16_t mode; /* mode */
   uint16_t links; /* number or links */
   uint16_t uid;
   uint16_t gid;
   uint32_t size;
   int32_t atime;
   int32_t mtime;
   int32_t ctime;
   uint32_t zone[DIRECT ZONES];
   uint32_t indirect;
   uint32_t two_indirect;
   uint32_t unused;
};

struct partition_entry {
   uint8_t bootind /* Boot magic number (0x80 if bootable) */
   uint8_t start_head /* Start of partition in CHS */
   uint8_t start_sec
   uint8_t start_cyl
   uint8_t type /* Type of partition (0x81 is minix) */
   uint8_t end head /* End of partition in CHS */
   uint8_t end sec
   uint8_t end cyl
   uint32_t lFirst /* First sector (LBA addressing) */
   uint32_t size
};


void initialize(superblock *block, int pFlag, int partition, ) {
   
}


int main(int argc, char **argv) {
   char vFlag = 0;
   char pFlag = 0;
   char spFlag = 0;
   char pathFlag = 0;
   char *imageName, *path;
   int partition, subpartition;
   int i;
   
   for (i = 1; i < argc ; i++) {
      if (!strcmp(argv[i], "-v")) {
         vFlag = 1;
         
         printf("Verbose\n");
      }
      else if (!strcmp(argv[i], "-p")){
         i++;
         partition = atoi(argv[i]);
         
         printf("Partition: %d\n", subpartition);
         pFlag = 1;
      }
      else if (!strcmp(argv[i], "-s")){
         i++;
         subpartition = atoi(argv[i]);
         
         printf("Subpartition: %d\n", subpartition);
         spFlag = 1;
      }
      else {
         imageName = calloc(1, strlen(argv[i]));
         strcpy(imageName, argv[i]);
         printf("Image name: %s\n", imageName);
         
         if (i + 2 == argc) {
            i++;
            path = calloc(1, strlen(argv[i]));
            strcpy(path, argv[i]);
            
            printf("Path name: %s\n", path);
            
            pathFlag = 1;
            break;
         }
      }
   }


}