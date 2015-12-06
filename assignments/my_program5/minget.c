/*
//  minget.c
//  assignments
//
//  Created by Elliot Fiske on 12/4/15.
//  Copyright © 2015 Elliot Fiske. All rights reserved.
*/

#include "min_common.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

uint32_t base_offset;
superblock_t superblock;

int verbose_flag = 0;
int partition_num = -1;
int subpartition_num = -1;

void output_file_contents(char *out_filename, inode_t *file_inode,
                          FILE *image_file, superblock_t superblock) {
   uint32_t zone_size = superblock.blocksize << superblock.log_zone_size;
   uint32_t total_read = 0;
   size_t read_bytes;
   int i = 0;
   
   char *file_data = malloc(zone_size);
   
   FILE *out_file = stdout;
   
   if (strlen(out_filename) > 0) {
      out_file = fopen(out_filename, "w");
   }
   
   if (out_filename == NULL) {
      perror(out_filename);
      exit(EXIT_FAILURE);
   }
   
   while (total_read < file_inode->size && i < 7) {
      fseek(image_file, file_inode->zone[i] * zone_size +
            base_offset, SEEK_SET);
      read_bytes = fread(file_data, 1,
                         MIN(zone_size, file_inode->size - total_read),
                         image_file);
      
      write(fileno(out_file), file_data, read_bytes);
      
      total_read += zone_size;
      i++;
   }
}

int main(int argc, char *argv[]) {
   FILE *image_file;
   
   char *search_path = "/";
   char *file_name;
   char *output_path = "";
   
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
   
   output_file_contents(output_path, specified_inode, image_file, superblock);
   
   return 0;
}