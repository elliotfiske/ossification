#include "minls.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define PARTITION_TABLE_LOC 0x1BE
#define PARTITION_TYPE 0x81
#define BYTE510 0x55
#define BYTE511 0xAA
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
#define MAX_DIRECTORY_ENTRIES 512
#define PERMISSIONS_STRING_SIZE 100
#define BIG_STRING_SIZE 256
#define SECTOR_SIZE 512
#define VALID_PARTITION_CHECK 510

uint32_t offset = 0;
int bitmapSize = 0;
int zoneSize = 0;
char originalFileName[BIG_STRING_SIZE] = "/:";
char destinationPath[BIG_STRING_SIZE] = { 0 };
uint32_t fileInode = 0;
char runLS = 0;
char runGet = 0;
char printGetOut = 0;



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
} __attribute__ ((__packed__));


void printSuperblock(struct superblock *block);  // TODO: DELETE

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

struct inode curNode = { 0 }; // INODE is here

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

void printSuperblock(struct superblock *block);
void printInode(struct inode *node);
void printPermissionString(uint16_t fileMode);
void printDirectory(FILE *imageFile, struct directory_entry *entry, int
 numOfDirectories,
 struct superblock *block); // TODO: DELETE ^
void printFile(FILE *imageFile, struct inode *node,
 struct superblock *block);

/* Initializes the superblock, and partition table entry if specified */
FILE *initialize(struct superblock *block, int partition, int subpartition,
                 int pFlag, int spFlag, char *imageName, char vFlag) {
   struct partition_entry mainPartition = { 0 };
   struct partition_entry subPartition = { 0 };
   uint32_t validByte1 = 0;
   uint32_t validByte2 = 0;
   FILE *diskImage = fopen(imageName, "r");
   size_t readBytes = 0;
   
   if (!diskImage) {
      printf("Disk image does not exist\n");
      exit(EXIT_FAILURE);
   }
 
   /* Attempt to read partition & subpartition */
   if (pFlag == 1) {
      /* Ensure it is a valid partition table */
      fseek(diskImage, VALID_PARTITION_CHECK, SEEK_SET);
      fread(&validByte1, 1, 1, diskImage);
      fread(&validByte2, 1, 1, diskImage); /* Check byte 511 */
      
      if (!(validByte1 == BYTE510 && validByte2 == BYTE511)) {
         fprintf(stderr, "Not a valid partition table\n");
         exit(EXIT_FAILURE);
      }
   
      /* Reset the file pointer */
      fseek(diskImage, PARTITION_TABLE_LOC, SEEK_SET);
      readBytes = fread(&mainPartition, sizeof(struct partition_entry), 1,
       diskImage);
      
      /* Ensure it is a Minix partition */
      if (mainPartition.type == 0x81) {
         offset = (mainPartition.lFirst + partition * mainPartition.size)
          * SECTOR_SIZE;
      }
      else {
         printf("Not a Minix partition\n");
         exit(EXIT_FAILURE);
      }
      
      if (spFlag == 1) {
         /* Ensure it is a valid partition table */
         fseek(diskImage, VALID_PARTITION_CHECK + offset, SEEK_SET);
         fread(&validByte1, 1, 1, diskImage);
         fread(&validByte2, 1, 1, diskImage); /* Check byte 511 */
         
         if (!(validByte1 == BYTE510 && validByte2 == BYTE511)) {
            printf("Not a valid sub partition table\n");
            exit(EXIT_FAILURE);
         }
      
         /* Reset the file pointer */
         fseek(diskImage, PARTITION_TABLE_LOC + offset, SEEK_SET);
         readBytes = fread(&subPartition, sizeof(struct partition_entry),
          1, diskImage);
         
         if (readBytes != 1) {
            printf("Read subpartition error\n");
            exit(EXIT_FAILURE);
         }
         
         /* Ensure it is a Minix partition */
         if (subPartition.type == 0x81) {
            offset = subPartition.lFirst * SECTOR_SIZE + (subpartition *
             subPartition.size)  * SECTOR_SIZE;
         }
         else {
            printf("Not a Minix partition\n");
         }
      }
   }
   else { /* Otherwise we treat it as unpartitioned */
      fseek(diskImage, START_OF_SUPERBLOCK + offset, SEEK_CUR);
      
      readBytes = fread(block, sizeof(struct superblock), 1, diskImage);
      
      /* Error checking */
      if (readBytes == 1) {
         if (block->magic != MINIX_MAGIC_NUMBER) {
            printf("Bad magic number. (%x)\n", block->magic);
            printf("This doesn't look like a MINIX filesystem.\n");
            exit(EXIT_FAILURE);
         }
         if (vFlag == 1) {
            printSuperblock(block);
         }
      }
      else {
         printf("Failed to read disk image properly\n");
      }
   }
   return diskImage;
}

/* Given an inode number, find the actual inode */
struct inode* findInodeFile(FILE *imageFile, int inode,
 struct superblock *block, char vFlag) {
   struct inode* node = calloc(1, sizeof(struct inode));
   uint16_t inodeBitmapBlocks = block->i_blocks;
   uint16_t zoneBitmapBlocks = block->z_blocks;
   uint16_t blockSize = block->blocksize;
   size_t readBytes = 0;
  
   /* 1 boot block, 1 super block, 
      X inode bitmap blocks, X zone bitmap blocks */
   int numPaddingBlocks = 1 + 1 + inodeBitmapBlocks + zoneBitmapBlocks;
   
   inode -= 1; /* inodes start at 1 so yeah */
   
   uint32_t offsetToInodes = numPaddingBlocks * blockSize + inode *
    sizeof(struct inode);
  
   fseek(imageFile, offsetToInodes + offset, SEEK_SET);
 
   readBytes = fread(node, sizeof(struct inode), 1, imageFile);

   if (readBytes == 1) {
      if (vFlag == 1) {
         printInode(node);
      }
   }
   else {
      perror("iNode read failed\n");
      exit(EXIT_FAILURE);
   }
 
   return node;
}

/* Finds the actual file given the root inode */
void findActualFile(struct inode *node, FILE *imageFile,
 struct superblock *block, char
  *path, char vFlag, char firstIteration) {
   uint32_t fileSize = node->size; /* File size in bytes */
   uint32_t totalRead = 0;
   uint32_t totalConverted = 0;
   size_t readBytes = 0;
   int i = 0;
   int i2 = 0;
   void *directory = calloc(1, MAX_DIRECTORY_ENTRIES *
    DIRECTORY_ENTRY_SIZE_BYTES);
   struct directory_entry *entries = calloc(MAX_DIRECTORY_ENTRIES,
    DIRECTORY_ENTRY_SIZE_BYTES);
   struct inode *newNode;
   void *curPtr = directory;
   uint32_t zoneSize = block->blocksize << block->log_zone_size;
   char *token;
   char foundSomething = 0;
   
   /* Reset pointer to start of file */
   fseek(imageFile, offset, SEEK_SET);

   /* Read through all the direct zones */
   while (totalRead <= node->size && i < 7) {
      fseek(imageFile, node->zone[i] * zoneSize + offset, SEEK_SET);
      readBytes = fread(curPtr, zoneSize, 1, imageFile);
      
      if (readBytes == 1) {
         curPtr += zoneSize;
         totalRead += zoneSize;
         i++;
      }
      else {
         printf("Read zones failed\n");
         break;
      }
   }
   
   /* Reset the curptr */
   curPtr = directory;
   
   /* Determine if we're looking at a directory or file */
   if ((node->mode & FILE_TYPE_MASK) == DIRECTORY) {
      i = 0;
      /* Treat directory as many directory_entrys */
      while (totalConverted < fileSize) {
         memcpy(&entries[i], curPtr, sizeof(struct directory_entry));
         curPtr += DIRECTORY_ENTRY_SIZE_BYTES;
         totalConverted += DIRECTORY_ENTRY_SIZE_BYTES;
     
         i++;
      }
      
      /* Base case */
      if (path == NULL) {
         printDirectory(imageFile, directory, i, block);
      }
      else {
         if (firstIteration) {
            token = strtok(path, "/");
         }
         else {
            token = strtok(NULL, "/");
         }
         
         if (token == NULL) {
            printDirectory(imageFile, directory, i, block);
            exit(EXIT_SUCCESS);
         }

         while (i2 < i) {
            /* On the right path */
            if (!strcmp(token, (const char *)entries[i2].name)) {
               fileInode = entries[i2].inode;
               
               /* Recurse */
               newNode = findInodeFile(imageFile, entries[i2].inode, block, 0);
               findActualFile(newNode, imageFile, block, path, vFlag, 0);
               foundSomething = 1;
               break;
            }
            else
               i2++;
         }
         
         /* If we didn't find anything */
         if (!foundSomething) {
            printf("minls: %s: No such file or directory\n",
             originalFileName);
         }
      }
   }
   /* Plain old file you see */
   else if ((node->mode & FILE_TYPE_MASK) == REGULAR_FILE) {
      printFile(imageFile, node, block);
   }
   else { /* We got something wacky going on here! */
      printf("Some wacky business\n");
   }
   
   /* Free directory_entrys */
   free(entries);
}

/* Prints the LS information for a directory */
void printDirectory(FILE *imageFile, struct directory_entry *entry,
 int numOfDirectories,
  struct superblock *block) {
   int i;
   struct inode *node;
   printf("%s\n", originalFileName);
   
   for (i = 0; i < numOfDirectories; i++) {
      node = findInodeFile(imageFile, entry[i].inode, block, 0);
      
      if (entry[i].inode != 0) { /* Don't print deleted files */
         printPermissionString(node->mode);
         printf("%10lu", (unsigned long)node->size);
         printf(" %s\n", entry[i].name);
      }
   }
}

/* Prints the LS information for just a file */
void printFile(FILE *imageFile, struct inode *node,
 struct superblock *block) {
   printPermissionString(node->mode);
   printf("%9lu",  (unsigned long)node->size);
   printf(" %s\n", originalFileName);
}

/** Given a file mode from the inode, print the permission string like
    "drw-rwx-w-" or whatever. */
void printPermissionString(uint16_t fileMode) {
   char dir =         ((fileMode & FILE_TYPE_MASK) == DIRECTORY) ? 'd' : '-';
   
   char owner_read  = ((fileMode & OWNER_READ_PERMISSION))       ? 'r' : '-';
   char owner_write = ((fileMode & OWNER_WRITE_PERMISSION))      ? 'w' : '-';
   char owner_exec  = ((fileMode & OWNER_EXECUTE_PERMISSION))    ? 'x' : '-';
   
   char group_read =  ((fileMode & GROUP_READ_PERMISSION))       ? 'r' : '-';
   char group_write = ((fileMode & GROUP_WRITE_PERMISSION))      ? 'w' : '-';
   char group_exec  = ((fileMode & GROUP_EXECUTE_PERMISSION))    ? 'x' : '-';
   
   char other_read =  ((fileMode & OTHER_READ_PERMISSION))       ? 'r' : '-';
   char other_write = ((fileMode & OTHER_WRITE_PERMISSION))      ? 'w' : '-';
   char other_exec  = ((fileMode & OTHER_EXECUTE_PERMISSION))    ? 'x' : '-';
   
   printf("%c%c%c%c%c%c%c%c%c%c",
          dir,
          owner_read, owner_write, owner_exec,
          group_read, group_write, group_exec,
          other_read, other_write, other_exec  );
}

/* Prints the superblock contents */
void printSuperblock(struct superblock *block) {
   fprintf(stderr, "Superblock Contents\n");
   fprintf(stderr, "Stored Fields:\n");
   fprintf(stderr, "  ninodes %d\n", block->ninodes);
   fprintf(stderr, "  i_blocks %d\n", block->i_blocks);
   fprintf(stderr, "  z_blocks %d\n", block->z_blocks);
   fprintf(stderr, "  firstdata %d\n", block->firstdata);
   fprintf(stderr, "  log_zone_size %d\n", block->log_zone_size);
   fprintf(stderr, "  max_file %d\n", block->max_file);
   fprintf(stderr, "  magic 0x%x\n", block->magic);
   fprintf(stderr, "  zones %d\n", block->zones);
   fprintf(stderr, "  blocksize %d\n", block->blocksize);
   fprintf(stderr, "  subversion %d\n", block->subversion);
}

/* Prints the inode contents */
void printInode(struct inode *node) {
   fprintf(stderr, "File inode:\n");
   fprintf(stderr, "  uint16_t mode 0x%x\n", node->mode);
   fprintf(stderr, "  uint16_t links %d\n", node->links);
   fprintf(stderr, "  uint16_t uid %d\n", node->uid);
   fprintf(stderr, "  uint16_t gid %d\n", node->gid);
   fprintf(stderr, "  uint32_t size %d\n", node->size);
   fprintf(stderr, "  int32_t atime %d\n", node->atime);
   fprintf(stderr, "  int32_t mtime %d\n", node->mtime);
   fprintf(stderr, "  int32_t ctime %d\n", node->ctime);
   fprintf(stderr, "  Direct zones:\n");
   fprintf(stderr, "        zone[0]  =   %d\n", node->zone[0]);
   fprintf(stderr, "        zone[1]  =   %d\n", node->zone[1]);
   fprintf(stderr, "        zone[2]  =   %d\n", node->zone[2]);
   fprintf(stderr, "        zone[3]  =   %d\n", node->zone[3]);
   fprintf(stderr, "        zone[4]  =   %d\n", node->zone[4]);
   fprintf(stderr, "        zone[5]  =   %d\n", node->zone[5]);
   fprintf(stderr, "        zone[6]  =   %d\n", node->zone[6]);
   fprintf(stderr, "  uint32_t indirect %d\n", node->indirect);
   fprintf(stderr, "  uint32_t two_indirect %d\n", node->two_indirect);
   fprintf(stderr, "  uint32_t unused %d\n", node->unused);
}

/* Prints the partition table */
void printPartitionTable(struct partition_entry *entry) {

}


/* Prints the correct usage of minls */
void print_correct_minls_usage() {
   printf("usage: minls [ -v ] [ -p num [ -s num ] ] imagefile [ path ]\n");
   printf("Options:\n");
   printf("\t-p part --- select partition for filesystem (default: none)\n");
   printf("\t-s sub --- select subpartition for filesystem (default: none)\n");
   printf("\t-h help --- print usage information and exit\n");
   printf("\t-v verbose --- increase verbosity level)\n");
}
/* Prints correct usage of minget */
void print_correct_minget_usage() {
   printf("usage: minget [ -v ] [ -p num [ -s num ] ] imagefile [ path ]\n");
   printf("Options:\n");
   printf("\t-p part --- select partition for filesystem (default: none)\n");
   printf("\t-s sub --- select subpartition for filesystem (default: none)\n");
   printf("\t-h help --- print usage information and exit\n");
   printf("\t-v verbose --- increase verbosity level)\n");
}

int main(int argc, char **argv) {
   char vFlag = 0;
   char pFlag = 0;
   char spFlag = 0;
   char pathFlag = 0;
   char *imageName = NULL;
   char *path = NULL;
   int partition = 0;
   int subpartition = 0;
   int i;
   struct superblock block;
   struct inode* currentInode;
   FILE *imageFile;
   
   if (strcmp(argv[0], "minls"))
      runLS = 1;
   else if (strcmp(argv[0], "minget"))
      runGet = 1;
   
   if (argc < 2) {
      if (runLS == 1)
         print_correct_minls_usage();
      else if (runGet == 1)
         print_correct_minget_usage();
   }
   
   for (i = 1; i < argc ; i++) {
      if (!strcmp(argv[i], "-v")) {
         vFlag = 1;
      }
      else if (argv[i][0] == '-' && argv[i][1] == 'p') {
         partition = atoi(argv[i]+2);
         
         pFlag = 1;
      }
      else if (argv[i][0] == '-' && argv[i][1] == 's') {
         subpartition = atoi(argv[i]+2);
         
         spFlag = 1;
      }
      else {
         imageName = calloc(1, strlen(argv[i]));
         strcpy(imageName, argv[i]);
         
         if (runLS == 1) { /* minls */
            if (i + 2 == argc) {
               i++;
               path = calloc(1, strlen(argv[i]));
               strcpy(path, argv[i]);
               strcpy(originalFileName, path);
               
               pathFlag = 1;
               break;
            }
         }
         else if (runGet == 1) { /* minget */
            if (i + 3 == argc) { /* Print to path */
               i++;
               path = calloc(1, strlen(argv[i]));
               strcpy(path, argv[i]);
               strcpy(originalFileName, path);
               
               i++;
               strcpy(destinationPath, argv[i]);
               
               pathFlag = 1;
               break;
            }
            else if (i + 2 == argc) { /* Print to stdout */
               printGetOut = 1;
               
            }
         }
      }
   }
   
   if (imageName == NULL) {
      if (runLS == 1)
         print_correct_minls_usage();
      else if (runGet == 1)
         print_correct_minget_usage();
      exit(EXIT_FAILURE);
   }
   
   /* Initialize the superblock */
   imageFile = initialize(&block, partition,
    subpartition, pFlag, spFlag, imageName, vFlag);
   
   /* Get the first inode */
   currentInode = findInodeFile(imageFile, 1, &block, vFlag);
   
   /* Iterate through inode and compare paths */
   findActualFile(currentInode, imageFile, &block, path, vFlag, 1);

   return EXIT_SUCCESS;
}
