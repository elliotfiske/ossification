/*
//  min_common.c
//
//  Stuff that both minget and minls need
//
//  my_program5
//
//  Created by Elliot Fiske on 12/4/15.
//  Copyright © 2015 Elliot Fiske. All rights reserved.
*/

#include "min_common.h"

#include <ctype.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/** Verbose flag for THIS file (min_common.c) */
int common_verbose_flag = 0;

void d_printf(int vflag, const char *format, ...) {
   va_list args;
   va_start(args, format);
   
   if(vflag)
      vprintf(format, args);
   
   va_end(args);
}

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
   
   d_printf(common_verbose_flag, "vflag = %d, partnum = %d, subpartnum = %d\n",
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
   /* TODO: me */
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
 * Take an inode and populate the list of directory entries it contains
 *
 * exits if the inode isn't a directory
 *
 * Returns # of files in this directory
 */
int directory_entries_from_inode(inode_t *inode, FILE *image_file,
                                 superblock_t superblock, uint32_t base_offset,
                                 directory_entry_t **result) {
   directory_entry_t *entries = calloc(MAX_DIRECTORY_ENTRIES,
                                       DIRECTORY_ENTRY_SIZE_BYTES);
   char *curr_entry_pointer = (char *) entries; /* Copy entries one at a time
                                                 * with this pointer. */
   
   uint32_t total_read = 0;
   uint32_t zone_size = superblock.blocksize << superblock.log_zone_size;
   size_t read_bytes;
   int i = 0;
   
   *result = NULL;
   
   if ((inode->mode & FILE_TYPE_MASK) != DIRECTORY) {
      printf("That's not a directory!");
      return 0;
   }
   
   while (total_read < inode->size && i < 7) {
      fseek(image_file, inode->zone[i] * zone_size + base_offset, SEEK_SET);
      read_bytes = fread(curr_entry_pointer, 1, zone_size, image_file);
      
      if (read_bytes == zone_size) {
         curr_entry_pointer += zone_size;
         total_read += zone_size;
         i++;
      }
      else {
         printf("Read zones failed\n");
         exit(EXIT_FAILURE);
      }
   }
   
   *result = entries;
   
   return inode->size / DIRECTORY_ENTRY_SIZE_BYTES;
}

/**
 * Given a pathname, go down the path and return the inode we want
 *
 * (Could be a directory or a file)
 */
inode_t *get_inode_from_path(char *path, superblock_t superblock,
                             uint32_t base_offset, FILE *image_file) {
   inode_t *curr_inode = NULL;
   directory_entry_t *curr_entries = NULL;
   int num_files_in_directory = 0;
   directory_entry_t *single_entry;
   char *token;
   char strtok_path[MAX_PATH_LENGTH]; /* Make a copy of path b/c strtok mangles
                                       * strings */
   
   uint32_t curr_inode_num = 1;
   int found_path = 0;
   
   strcpy(strtok_path, path);
   token = strtok(strtok_path, "/");
   
   while (token != NULL) {
      printf("Path: %s\n", token);
      
      curr_inode = inode_from_inode_num(curr_inode_num, superblock,
                                        base_offset, image_file);
      
      num_files_in_directory = directory_entries_from_inode(curr_inode,
                                                            image_file,
                                                            superblock,
                                                            base_offset,
                                                            &curr_entries);
      
      /* Check all the directory entries for the right name */
      single_entry = curr_entries;
      found_path = 0;
      while (single_entry != NULL) {
         if (strcmp(token, (const char *) single_entry->name) == 0) {
            curr_inode_num = single_entry->inode_num;
            found_path = 1;
            break;
         }
         
         single_entry++;
      }
      
      token = strtok(NULL, "/");
      
      /* User specified non-existent file */
      if (!found_path) {
         printf("minls: %s: No such file or directory\n", path);
         exit(EXIT_FAILURE);
      }
      
      /* User tried to navigate through a file as if it was a directory */
      if (curr_entries == NULL && token != NULL) {
         printf("minls: %s: No such file or directory\n", path);
         exit(EXIT_FAILURE);
      }
      
      if (curr_entries != NULL) {
         free(curr_entries);
      }
   }
   
   return curr_inode;
}