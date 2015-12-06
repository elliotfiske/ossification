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
#include <fcntl.h>

uint32_t base_offset;
superblock_t superblock;

int verbose_flag = 0;
int partition_num = -1;
int subpartition_num = -1;

/** 
 * Hole detected! Write the right # of zeroes.
 */
void write_zeroes(int out_fd, size_t how_many) {
   int ndx;
   for (ndx = 0; ndx < how_many; ndx++) {
      char zero = 0;
      write(out_fd, &zero, sizeof(char));
   }
}

int handle_indirect_zone(uint32_t zone_size, FILE *image_file,
                          inode_t *file_inode, int total_read,
                          int out_file_descriptor, int indirect_zone_num) {
   uint32_t *indirect_zone = malloc(zone_size);
   size_t read_bytes;
   char *file_data = malloc(zone_size);
   
   int indirect_ndx;
   
   fseek(image_file, indirect_zone_num * zone_size + base_offset, SEEK_SET);
   read_bytes = fread(indirect_zone, 1, zone_size, image_file);
   
   for (indirect_ndx = 0;
        indirect_ndx < zone_size / ZONE_INDEX_BYTES; indirect_ndx++) {
      if (total_read >= file_inode->size) {
         break;
      }
      
      if (indirect_zone[indirect_ndx] == 0) {
         write_zeroes(out_file_descriptor, zone_size);
         total_read += zone_size;
      }
      else {
         fseek(image_file, indirect_zone[indirect_ndx] * zone_size
               + base_offset, SEEK_SET);
         read_bytes = fread(file_data, 1,
                            MIN(zone_size, file_inode->size - total_read),
                            image_file);
         
         write(out_file_descriptor, file_data, read_bytes);
         
         total_read += read_bytes;
      }
   }
   
   return total_read;
}

void output_file_contents(char *out_filename, inode_t *file_inode,
                          FILE *image_file, superblock_t superblock) {
   uint32_t zone_size = superblock.blocksize << superblock.log_zone_size;
   uint32_t total_read = 0;
   size_t read_bytes;
   int i = 0;
   
   uint32_t *double_indirect_zone = malloc(zone_size);
   
   char *file_data = malloc(zone_size);
   int d_indirect_ndx;
   
   int out_file_descriptor = STDOUT_FILENO;
   
   /* Check that it's not a directory */
   if ((file_inode->mode & FILE_TYPE_MASK) != REGULAR_FILE) {
      printf("Can't call minget on a directory\n");
      exit(EXIT_FAILURE);
   }
   
   if (strlen(out_filename) > 0) {
      out_file_descriptor = open(out_filename, O_CREAT | O_TRUNC | O_WRONLY);
   }
   
   if (out_file_descriptor == -1) {
      perror(out_filename);
      exit(EXIT_FAILURE);
   }
   
   while (total_read < file_inode->size && i < DIRECT_ZONES) {
      if (file_inode->zone[i] == 0) {
         write_zeroes(out_file_descriptor, zone_size);
         
         total_read += zone_size;
         i++;
      }
      else {
         fseek(image_file, file_inode->zone[i] * zone_size +
               base_offset, SEEK_SET);
         
         read_bytes = fread(file_data, 1,
                            MIN(zone_size, file_inode->size - total_read),
                            image_file);
         
         write(out_file_descriptor, file_data, read_bytes);
         
         total_read += read_bytes;
         i++;
      }
   }
   
   /* Indirect zone */
   if (total_read < file_inode->size) {
      handle_indirect_zone(zone_size, image_file, file_inode, total_read,
                           out_file_descriptor, file_inode->indirect);
   }
   
   /* Double indirect zone */
   if (total_read < file_inode->size) {
      fseek(image_file,
            file_inode->two_indirect * zone_size + base_offset, SEEK_SET);
      read_bytes = fread(double_indirect_zone, 1, zone_size, image_file);
      
      for (d_indirect_ndx = 0;
           d_indirect_ndx < zone_size / ZONE_INDEX_BYTES; d_indirect_ndx++) {
         if (total_read >= file_inode->size) {
            break;
         }
         
         total_read = handle_indirect_zone(zone_size, image_file, file_inode,
                                           total_read, out_file_descriptor,
                                       double_indirect_zone[d_indirect_ndx]);
      }
   }
}

int main(int argc, char *argv[]) {
   FILE *image_file;
   
   char *search_path = "";
   char *file_name = "";
   char *output_path = "";
   
   /** The inode of whatever directory / file the user asked for */
   inode_t *specified_inode;
   
   image_file = parse_arguments(argc, argv, &verbose_flag,
                                &partition_num, &subpartition_num,
                                &search_path, &output_path);
   
   if (strlen(search_path) == 0) {
      printf("Please specify a file to get, like:\
              %s <Image File> <File Name>\n", argv[0]);
      exit(EXIT_FAILURE);
   }
   
   base_offset = get_partition_offset(partition_num, subpartition_num,
                                      image_file, 0);
   
   superblock = parse_superblock(base_offset, image_file);
   
   specified_inode = get_inode_from_path(search_path, superblock,
                                         base_offset, image_file, &file_name);
   
   output_file_contents(output_path, specified_inode, image_file, superblock);
   
   return 0;
}