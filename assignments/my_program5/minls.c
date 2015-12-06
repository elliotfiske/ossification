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

int main(int argc, char *argv[]) {
   FILE *image_file;
   
   char *search_path = "";
   char *output_path; /* Not used for minls */
   
   char *test_path = "lets/do";
   char *base = basename(test_path);
   char *dir = dirname(test_path);
   printf("extracted basename: %s\n", base);
   printf("cont with dirname: %s\n", dir);
   printf("string now is %s\n", test_path);
   
   base = basename(dir);
   dir = dirname(dir);
   printf("extract base: %s\n", base);
   printf("cont with dirname: %s\n", dir);
   printf("string now is %s\n", test_path);
   
   /** The inode of whatever directory / file the user asked for */
   inode_t specified_inode;
   
   image_file = parse_arguments(argc, argv, &verbose_flag,
                               &partition_num, &subpartition_num,
                               &search_path, &output_path);
   
   base_offset = get_partition_offset(partition_num, subpartition_num,
                                      image_file);
   
   superblock = parse_superblock(base_offset, image_file);
   
   specified_inode = get_inode_from_path(search_path, superblock,
                                         base_offset, image_file);
   
   return 0;
}