//
//  min_common.c
//
//  Stuff that both minget and minls need
//
//  my_program5
//
//  Created by Elliot Fiske on 12/4/15.
//  Copyright © 2015 Elliot Fiske. All rights reserved.
//

#include "min_common.h"

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/** Verbose flag for THIS file (min_common.c) */
int common_verbose_flag = 0;

/**
 * Parse arguments, stick 'em in the places that I point to.
 * 
 * Returns the FILE * with the fancy pancy filesystem itself.
 */
FILE *parse_arguments(int argc, char *argv[],
                      int *verbose_flag,
                      int *partition_num, int *subpartition_num,
                      char **path_1, char **path_2) {
   
   int c;
   int index;
   FILE *result;
   char *image_filename;
   
   while ((c = getopt (argc, argv, "vp:s:")) != -1) {
      switch (c) {
         case 'v':
            *verbose_flag = 1;
            common_verbose_flag = 1;
            break;
         case 'p':
            *partition_num = atoi(optarg);
            break;
         case 's':
            *subpartition_num = atoi(optarg);
            break;
            
         default:
            printf("Huh? Real weird argument error.\n");
            exit(EXIT_FAILURE);
            
      }
   }
   
   my_printf("vflag = %d, partnum = %d, subpartnum = %d\n",
            *verbose_flag, *partition_num, *subpartition_num);
   
   /* Copy string arguments */
   index = optind;
   
   /** Parse Image Filename */
   if (index == argc) {
      printf("Please specify an image file, like %s <ImageFile>\n", argv[0]);
      exit(EXIT_FAILURE);
   }
   image_filename = argv[index];
   
   /** Parse path to search **/
   index++;
   if (index < argc) {
      *path_1 = argv[index];
   }
   
   /** Parse output path **/
   index++;
   if (index < argc) {
      *path_2 = argv[index];
   }

   result = fopen(image_filename, "r");
   
   if (!result) {
      printf("Disk image does not exist\n");
      exit(EXIT_FAILURE);
   }
   
   return result;
}

/**
 * Given a partition num and subpartition num, return the offset to
 *  the start of the data we're interested in.
 */
uint32_t get_partition_offset(int partition_num, int subpartition_num,
                              FILE *image_file) {
   // TODO: me
   return 0;
}

/* Prints the superblock contents */
void print_superblock(struct superblock *block) {
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

/**
 * Get the superblock info
 */
superblock_t parse_superblock(uint32_t base_offset, FILE *image_file) {
   superblock_t result;
   size_t bytes_read;
   
   fseek(image_file, START_OF_SUPERBLOCK + base_offset, SEEK_CUR);
   bytes_read = fread(&result, 1, sizeof(superblock_t), image_file);
   
   if (bytes_read != sizeof(superblock_t)) {
      printf("Weird, couldn't read entire superblock. Tiny filesystem?\n");
      exit(EXIT_FAILURE);
   }
   
   if (result.magic != MINIX_MAGIC_NUMBER) {
      printf("Bad magic number. (%x)\n", result.magic);
      printf("This doesn't look like a MINIX filesystem.\n");
      exit(EXIT_FAILURE);
   }
   
   if (common_verbose_flag) {
      print_superblock(&result);
   }
   
   return result;
}

/**
 * Given inode #, return the inode and all its tasty data
 */
inode_t *inode_from_inode_num(int32_t inode_num, superblock_t superblock,
                              uint32_t base_offset, FILE *image_file) {
   struct inode* node = calloc(1, sizeof(inode_t));
   uint16_t inode_bitmap_blocks = superblock.i_blocks;
   uint16_t zone_bitmap_blocks  = superblock.z_blocks;
   uint16_t blockSize = superblock.blocksize;
   
   size_t bytes_read = 0;
   
   /* 1 boot block, 1 super block,
      X inode bitmap blocks, X zone bitmap blocks */
   int numPaddingBlocks = 1 + 1 + inode_bitmap_blocks + zone_bitmap_blocks;
   
   uint32_t offset_to_inodes = numPaddingBlocks * blockSize +
                               (inode_num - 1) * sizeof(inode_t) + base_offset;
                              /* inodes start @ 1 */
   
   fseek(image_file, offset_to_inodes, SEEK_SET);
   bytes_read = fread(node, 1, sizeof(struct inode), image_file);
   
   if (bytes_read != sizeof(struct inode)) {
      printf("Error trying to read inode (%d)\n", inode_num);
      printf("Tiny filesystem?\n");
      exit(EXIT_FAILURE);
   }
   
   if (common_verbose_flag) {
      printInode(node);
   }
   
   return node;
}

/**
 * Given a pathname, go down the path and return the inode we want
 *
 * (Could be a directory or a file)
 */
inode_t get_inode_from_path(char *path, superblock_t superblock,
                            uint32_t base_offset, FILE *image_file) {
   inode_t result;
   
   
   
   return result;
}