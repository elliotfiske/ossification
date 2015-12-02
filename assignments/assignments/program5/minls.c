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
#define START_OF_SUPERBLOCK 1024

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

#define DIRECT_ZONES 7

int offset;
int bitmapSize;

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
   uint32_t zone[DIRECT_ZONES];
   uint32_t indirect;
   uint32_t two_indirect;
   uint32_t unused;
};

struct partition_entry {
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
};


struct directory_entry {
   uint32_t inode;
   unsigned char name[60];
};

/* Initializes the superblock, and partition table entry if specified */
struct superblock *initialize(int partition, int subpartition, int pFlag,
 int spFlag, char *imageName, char *path, char vFlag) {
   struct partition_entry mainPartition;
   struct partition_entry subPartition;
   struct superblock *block;
   uint32_t firstSector;
   FILE *diskImage = fopen(imageName, "r+");
   size_t readBytes = 0;
   
   if (!diskImage) {
      perror("Disk image does not exist\n");
      exit(EXIT_FAILURE);
   }
 
   /* Attempt to read partition & subpartition */
   if (pFlag == 1) {
      if (spFlag == 1) {
         
      }
      else {
         readBytes = fread(&mainPartition, sizeof(struct partition_entry), 1, diskImage);
         
         /* Ensure it is a Minix partition */
         if (mainPartition.type == 0x81) {
            firstSector = mainPartition.lFirst;
         }
         else {
            printf("Not a Minix partition\n");
         }
      }
   }
   else { /* Otherwise we treat it as unpartitioned */
      fseek(diskImage, START_OF_SUPERBLOCK, SEEK_CUR);
      
      readBytes = fread(block, sizeof(struct superblock), 1, diskImage);
      
      /* Error checking */
      if (readBytes == 1) {
         
      }
      else {
      
      }
   }

   return block;
}

/* Given an inode number, find the actual inode */
struct inode findInodeFile(FILE *image, int inode, struct superblock *block,
 char *path, char vFlag) {
   
}

/* Prints the LS information */
void printInfo() {

}



/* Prints the superblock contents */
void printSuperblock(struct superblock *block) {

}

/* Prints the inode contents */
void printInode(struct inode *node) {

}

/* Prints the partition table */
void printPartitionTable(struct partition_entry *entry) {

}


/* Prints the correct usage of minls */
void print_correct_usage() {
   printf("usage: minls [ -v ] [ -p num [ -s num ] ] imagefile [ path ]\n");
   printf("Options:\n");
   printf("-p part --- select partition for filesystem (default: none)\n");
   printf("-s sub --- select subpartition for filesystem (default: none)\n");
   printf("-h help --- print usage information and exit\n");
   printf("-v verbose --- increase verbosity level)\n");
}

int main(int argc, char **argv) {
   char vFlag = 0;
   char pFlag = 0;
   char spFlag = 0;
   char pathFlag = 0;
   char *imageName, *path;
   int partition, subpartition;
   int i;
   struct superblock* block;
   
   printf("Size of superblock: %lu\n", sizeof(struct superblock));
   printf("Size of inode: %lu\n", sizeof(struct inode));
   printf("Size of partition_entry: %lu\n", sizeof(struct partition_entry));
   printf("Size of directory_entry: %lu\n", sizeof(struct directory_entry));
   
   if (argc < 2) {
      print_correct_usage();
   }
   
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
   
   block = initialize(partition, subPartition, pFlag, spFlag,
    imageName, path, vFlag);
   
   /* Get the first inode */
      
   /* Iterate through inode and compare paths - lots of shit to do here for sure */
   

}