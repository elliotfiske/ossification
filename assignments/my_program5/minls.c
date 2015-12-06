/*
//  minls.c
//  assignments
//
//  Created by Elliot Fiske on 12/4/15.
//  Copyright © 2015 Elliot Fiske. All rights reserved.
*/

#include "min_common.h"

#include <stdio.h>

int verbose_flag = 0;
int partition_num = -1;
int subpartition_num = -1;

int base_offset; /* Base offset, calculated from the partition */

superblock_t superblock;



/** Given a file mode from the inode, print the permission string like
 "drw-rwx-w-" or whatever. */
void print_permission_string(uint16_t fileMode) {
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

/* Prints the LS information for a directory */
void print_directory(FILE *image_file, superblock_t superblock,
                     uint32_t base_offset, inode_t *directory_inode) {
   int ndx, num_files;
   inode_t *node;
   directory_entry_t *entries = NULL;
   directory_entry_t *curr_entry;
   
   num_files = directory_entries_from_inode(directory_inode, image_file,
                                            superblock, base_offset, &entries);
   
   curr_entry = entries;
 
   for (ndx = 0; ndx < num_files; ndx++) {
      if (curr_entry->inode_num != 0) { /* Don't print deleted files */
         node = inode_from_inode_num(curr_entry->inode_num, superblock,
                                     base_offset, image_file);
      
         print_permission_string(node->mode);
         printf("%10lu", (unsigned long)node->size);
         printf(" %s\n", curr_entry->name);
      }
      
      curr_entry ++;
   }
}

/**
 * We've got the inode the user specified via the path!
 *
 * If it's a directory, list its content. If it's a file,
 * list its permissions.
 */
void list_contents_of_inode(inode_t *inode, char *path, FILE *image_file,
                            superblock_t superblock, uint32_t base_offset,
                            char *file_name) {
   if ((inode->mode & FILE_TYPE_MASK) == DIRECTORY) {
      printf("%s:\n", path);
      print_directory(image_file, superblock, base_offset, inode);
   }
   else if ((inode->mode & FILE_TYPE_MASK) == REGULAR_FILE) {
      print_permission_string(inode->mode);
      printf("%10lu", (unsigned long)inode->size);
      printf(" %s\n", file_name);
   }
   else {
      printf("Given file's mode is 0x%x, and isn't a file OR a directory.\n",
             inode->mode);
   }
}

int main(int argc, char *argv[]) {
   FILE *image_file;
   
   char *search_path = "/";
   char *file_name;
   char *output_path; /* Not used for minls */
   
   /** The inode of whatever directory / file the user asked for */
   inode_t *specified_inode;
   
   image_file = parse_arguments(argc, argv, &verbose_flag,
                               &partition_num, &subpartition_num,
                               &search_path, &output_path);
   
   base_offset = get_partition_offset(partition_num, subpartition_num,
                                      image_file, 0);
   
   superblock = parse_superblock(base_offset, image_file);
   
   specified_inode = get_inode_from_path(search_path, superblock,
                                         base_offset, image_file, &file_name);
   
   list_contents_of_inode(specified_inode, search_path, image_file,
                          superblock, base_offset, file_name);
   
   return 0;
}