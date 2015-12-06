//
//  minls.c
//  assignments
//
//  Created by Elliot Fiske on 12/4/15.
//  Copyright © 2015 Elliot Fiske. All rights reserved.
//

#include "min_common.h"

#include <stdio.h>

int verbose_flag = 0;
int partition_num = -1;
int subpartition_num = -1;

int base_offset; /* Base offset, calculated from the partition */

superblock_t superblock;



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

/* Prints the LS information for a directory */
void print_directory(FILE *imageFile, struct directory_entry *entry,
                     int numOfDirectories, struct superblock *block,
                     uint32_t base_offset) {
   int i;
//   struct inode *node;
//   printf("%s\n", originalFileName);
//   
//   for (i = 0; i < numOfDirectories; i++) {
//      node = findInodeFile(imageFile, entry[i].inode, block, 0);
//      
//      if (entry[i].inode != 0) { /* Don't print deleted files */
//         printPermissionString(node->mode);
//         printf("%10lu", (unsigned long)node->size);
//         printf(" %s\n", entry[i].name);
//      }
//   }
}

/**
 * We've got the inode the user specified via the path!
 *
 * If it's a directory, list its content. If it's a file,
 * list its permissions.
 */
void list_contents_of_inode(inode_t *inode) {
   if ((inode->mode & FILE_TYPE_MASK) == DIRECTORY) {
      
   }
   else if ((inode->mode & FILE_TYPE_MASK) == REGULAR_FILE) {
      
   }
   else {
      printf("Given file's mode is 0x%x, and isn't a file OR a directory.\n",
             inode->mode);
   }
}

int main(int argc, char *argv[]) {
   FILE *image_file;
   
   char *search_path = "/";
   char *output_path; /* Not used for minls */
   
   /** The inode of whatever directory / file the user asked for */
   inode_t *specified_inode;
   
   image_file = parse_arguments(argc, argv, &verbose_flag,
                               &partition_num, &subpartition_num,
                               &search_path, &output_path);
   
   base_offset = get_partition_offset(partition_num, subpartition_num,
                                      image_file);
   
   superblock = parse_superblock(base_offset, image_file);
   
   specified_inode = get_inode_from_path(search_path, superblock,
                                         base_offset, image_file);
   
   
   list_contents_of_inode(specified_inode);
   
   return 0;
}