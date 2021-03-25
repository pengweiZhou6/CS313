#include "ext2.h"
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fsuid.h>
#include <sys/stat.h>
#include <sys/types.h>

#define EXT2_OFFSET_SUPERBLOCK 1024
#define EXT2_INVALID_BLOCK_NUMBER ((uint32_t) -1)

/* open_volume_file: Opens the specified file and reads the initial
   EXT2 data contained in the file, including the boot sector, file
   allocation table and root directory.
   
   Parameters:
     filename: Name of the file containing the volume data.
   Returns:
     A pointer to a newly allocated volume_t data structure with all
     fields initialized according to the data in the volume file
     (including superblock and group descriptor table), or NULL if the
     file is invalid or data is missing, or if the file is not an EXT2
     volume file system (s_magic does not contain the correct value).
 */
volume_t *open_volume_file(const char *filename) {
  volume_t *volume = NULL;
  
  int fd = open(filename, O_RDONLY);
  if (fd < 0) return NULL;

  /* Seek past the first 1024 bytes and read in a superblock. */
  if (lseek(fd, 1024, SEEK_CUR) != 1024) goto err;

  /* Use calloc, so all fields are 0. */
  if ((volume = calloc(1, sizeof(volume_t))) == NULL) goto err;
  volume->fd = fd;

  /* Now read the superblock. */
  if (read(fd, &volume->super, sizeof(struct superblock)) != sizeof(struct superblock)) goto err;

  /* Now verify that this is a valid ext2 image. */
  if (volume->super.s_magic != EXT2_SUPER_MAGIC) goto err;

  /* Now fill in fields in the volume structure. */
  volume->block_size = 1024 << volume->super.s_log_block_size;
  volume->volume_size = volume->block_size * volume->super.s_blocks_count;

  /*
   * To compute groups, subtract the initial space consumed by the superblock,
   * the reserved space, and the group descriptor table. Assumption: group
   * descriptor table fits in one block. Start with two blocks: one containing
   * the superblock and one containing the group descriptor table.
   */
  int metablocks = 2;
  /*
    If we have a 1024 block size, then we need an extra block for the 1024
    reserved bytes.
   */
  if (volume->block_size == 1024)
	metablocks++;
  volume->num_groups = (volume->super.s_blocks_count - metablocks) /
    volume->super.s_blocks_per_group;

  /*
   * This is a hack: the file system can have fewer blocks than the number
   * set in super.s_blocks_per_group, so handle this explicitly.
   */
  if (volume->num_groups == 0)
	volume->num_groups = 1;

  /* Make sure the block descriptors fit in a single block. */
  size_t groupmeta = volume->num_groups * sizeof(struct group_desc);
  if (groupmeta > volume->block_size) {
    fprintf(stderr, "Group block descriptors exceed block size\n");
    goto err;
  }
  volume->groups = calloc(1, groupmeta);
  if (volume->groups == NULL) goto err;

  /* Now, seek to where we expect to find the group descriptor information. */
  off_t group_desc_offset = volume->block_size * (metablocks - 1);
  if (lseek(fd, group_desc_offset, SEEK_SET) != group_desc_offset) goto err;

  if (read(fd, volume->groups, groupmeta) != groupmeta) {
    fprintf(stderr, "Unable to read groupmeta data\n");
    goto err;
  }

  /*
   * This is a common system paradigm for placing all your cleanup code together
   * so that on an error, you can be sure to free all the resources that you
   * allocated.
   */
  if (0) {
err: 
    close_volume_file(volume);
    volume = NULL;
  }
  return volume;
}

/* close_volume_file: Frees and closes all resources used by a EXT2 volume.
   
   Parameters:
     volume: pointer to volume to be freed.
 */
void close_volume_file(volume_t *volume) {
  if (volume != NULL) {
    close(volume->fd); 
    if (volume->groups != NULL)
      free(volume->groups);
    free(volume);
  }
}

/* read_disk_block: Given a disk address (physical block number on the drive),
   read the appropriate block from the disk and place it in the buffer. 

   Parameters:
     volume: Pointer to the volume
     daddr: Disk block number to read
     buffer: Pointer to the locatoin in which to place the block

   Returns:
     0 on success; -1 on failure.
*/
int read_disk_block(volume_t *volume, uint32_t daddr, void *buffer) {
  off_t offset = daddr * volume->block_size;
  if (lseek(volume->fd, offset, SEEK_SET) != offset) return -1;
  if (read(volume->fd, buffer, volume->block_size) != volume->block_size) return -1;
  return 0;
}

/* read_inode: Fills an inode data structure with the data from one
   inode in disk. Determines the block group number and index within
   the group from the inode number, then reads the inode from the
   inode table in the corresponding group. Saves the inode data in
   buffer 'buffer'.
   
   Parameters:
     volume: pointer to volume.
     inode_no: Number of the inode to read from disk.
     buffer: Pointer to location where data is to be stored.

   Returns:
     In case of success, returns a positive value. In case of error,
     returns -1.
 */
ssize_t read_inode(volume_t *volume, uint32_t inode_no, inode_t *buffer) {
  uint8_t block_buf[volume->block_size];
  
  /*
   * We find the inode using the following steps:
   * 0. Check for errors.
   * 1. Determine in which block group the inode resides
   * 2. Determine the inode's offset within the block group
   * 3. Identify the block in which that inode resides.
   * 4. Determine the block address for that block
   * 5. Read the block
   * 6. Place the proper bytes into the inode structure.
   */
  /* 0. Error checking. */
  if (inode_no != EXT2_ROOT_INO && inode_no < volume->super.s_first_ino) return -1;

  /* XXX This is awful, but it seems that because inode number 0 is an invalid inode,
   * Linux does not even store this inode, so all inode computation takes the value
   * of the inode and decrements it by one. Don't ever do that when designing a system.
   * if 0 is an invalid object, that you don't use it, but don't change the values
   * of object IDs (which is what an inode number is).
   */
  inode_no--;

  /* 1. Find the block group. */
  int block_group = inode_no / volume->super.s_inodes_per_group;

  /* 2. Find the offset within the group. */
  int inode_in_group = inode_no % volume->super.s_inodes_per_group;

  /* 3. Figure out in which inode block the inode resides. */
  int inodes_per_block = volume->block_size / volume->super.s_inode_size;
  uint32_t block_within_group = inode_in_group / inodes_per_block;


  /* 4. Determine the block address for that block */
  uint32_t inode_daddr = volume->groups[block_group].bg_inode_table + block_within_group;

   /* 5. Read the block */
  if (read_disk_block(volume, inode_daddr, block_buf) != 0) return -1;

   /* 6. Place the proper bytes into the inode structure. */
  memcpy(buffer, block_buf + (inode_in_group % inodes_per_block) * volume->super.s_inode_size, volume->super.s_inode_size);

  return 0;
}

/* lbn_to_daddr: Given a logical block number, returns the physical
   block number or disk address for that block.
   
   Parameters:
     volume: Pointer to volume.
     inode: Pointer to inode structure where data is to be sourced.
     lbn: Logical block number

   Returns:
     In case of success, returns the physical block number (disk address)
     indicating where the given lbn resides on disk. If no disk block has
     been allocated for this lbn, returns 0 (zero).  In case of error,
     returns EXT2_INVALID_BLOCK_NUMBER.
 */
static uint32_t lbn_to_daddr(volume_t *volume, inode_t *inode, uint64_t lbn) {
  uint32_t buffer[volume->block_size / 4];
  uint32_t blocks =(volume->block_size / 4);
  uint32_t startPoint_2ind=(blocks+EXT2_NDIR_BLOCKS);
  uint32_t startPoint_3ind=(blocks + blocks*blocks+EXT2_NDIR_BLOCKS);
  uint32_t endPoint_3ind=(blocks + blocks*blocks+ blocks*blocks*blocks+EXT2_NDIR_BLOCKS);
  if(lbn>=0 && lbn <EXT2_NDIR_BLOCKS){
    return inode -> i_block[lbn];
  }
  if(lbn >= EXT2_NDIR_BLOCKS && lbn < startPoint_2ind){
    if (read_disk_block(volume, inode->i_block_1ind , buffer) != 0) return EXT2_INVALID_BLOCK_NUMBER;
    return buffer[lbn-EXT2_NDIR_BLOCKS];
  }//handle 1ind
  if(lbn >= startPoint_2ind && lbn< startPoint_3ind){
    uint32_t buffer_2ind[volume->block_size / 4];
    uint64_t offset_2ind = (lbn-startPoint_2ind) % blocks;
    uint64_t offset_1ind = (lbn-startPoint_2ind) / blocks;
    if (read_disk_block(volume, inode->i_block_2ind , buffer) != 0) return EXT2_INVALID_BLOCK_NUMBER;
    if (read_disk_block(volume, buffer[offset_1ind] , buffer_2ind) != 0) return EXT2_INVALID_BLOCK_NUMBER;
    return buffer_2ind[offset_2ind];
  }//handle 2ind
  if(lbn >= startPoint_3ind && lbn< endPoint_3ind){
    uint32_t buffer_2ind[volume->block_size / 4];
    uint32_t buffer_3ind[volume->block_size / 4];
    uint64_t offset_3ind = (lbn-startPoint_3ind) % blocks;
    uint64_t offset_2ind = (lbn-startPoint_3ind) / blocks;
    uint64_t offset_1ind = (lbn-startPoint_3ind) / (blocks*blocks);
    if (read_disk_block(volume, inode->i_block_3ind , buffer) != 0) return EXT2_INVALID_BLOCK_NUMBER;
    if (read_disk_block(volume, buffer[offset_1ind] , buffer_2ind) != 0) return EXT2_INVALID_BLOCK_NUMBER;
    if (read_disk_block(volume, buffer_2ind[offset_2ind] , buffer_3ind) != 0) return EXT2_INVALID_BLOCK_NUMBER;
    return buffer_3ind[offset_3ind];
  }//handle 3ind
  return EXT2_INVALID_BLOCK_NUMBER;
}
//call lbn_to_daddr(volume,&inode,12)
//call read_disk_block(volume,179,buffer)
//call read_file_block(volume,&inode,12,buffer)
//12 268 65804 16843020
//  gdb ext2test
//  break 78
//  run /home/c/cs-313/EXTimages/SparseSmall.img
//  call read_inode(volume, 12 , &inode)
/* read_file_block: Given a file and logical block number (lbn), reads
   the appropriate block from disk.

   Hint: You might find the previous routine helpful.
   
   Parameters:
     volume: Pointer to volume.
     inode: Pointer to inode structure for the file.
     lbn: Logical block number to be read.
     buffer: Pointer to location where data is to be stored.

   Returns:
     In case of success, returns 0; in case of error, returns -1.
     In case of success, returns 0; in case of error, returns -1.
 */
int read_file_block(volume_t *volume, inode_t *inode, uint64_t lbn, void *buffer) {
  uint32_t daddr = lbn_to_daddr(volume,inode,lbn);
  if(daddr == EXT2_INVALID_BLOCK_NUMBER || daddr==0) return -1;
  if(read_disk_block(volume, daddr, buffer) == 0) return 0;
  return -1;
}

/* read_file_content: This is the file system function that implements
   the read system call for ext2. It reads some number of bytes (possibly
   larger than a single block) from a file, starting at the given offset.
   This function should never try to read past the end of the file.
   
   Parameters:
     volume: Pointer to volume.
     inode: Pointer to inode structure for the file.
     offset: Offset, in bytes from the start of the file, of the data
             to be read.
     nbytes: Maximum number of bytes to read from the file.
     buffer: Pointer to location where data is to be stored.

   Returns:
     In case of success, returns the number of bytes read from the
     disk. In case of error, returns -1.
 */
ssize_t read_file_content(volume_t *volume, inode_t *inode, uint64_t offset, uint64_t nbytes, void *buffer) {

  uint64_t bytes_read = 0;
  uint64_t lbn;
  uint8_t block_buf[volume->block_size];/* Buffer to hold a single block. */

  /* 
     If offset + nbytes exceeds the size of the file, we will read fewer
     than max_size bytes; compute that value now.
  */
  if (offset >= inode_file_size(volume, inode)) return 0;

  if (offset + nbytes > inode_file_size(volume, inode))
    nbytes = inode_file_size(volume, inode) - offset;
  
  /*
     We read files in block sized units, but the beginning of the read
     and the end of the read may not be block aligned. One could combine
     the first block handling with the rest of the blocks, but we handle
     it separately for clarity in reading.
  */
  lbn = offset / ext2_blocksize(volume);

  /* Check for partial block read at beginning. */
  uint64_t block_offset = offset % ext2_blocksize(volume);
  if (block_offset != 0) {
    int ret = read_file_block(volume, inode, lbn, block_buf);
    if (ret < 0)
        return ret;
    bytes_read = ext2_blocksize(volume) - offset;
    if (bytes_read > nbytes)
        bytes_read = nbytes;
    memcpy(buffer, block_buf + block_offset, bytes_read);
    lbn++;
  }

  /* Now loop through remaining blocks. */
  uint64_t copy_bytes = ext2_blocksize(volume);
  while (bytes_read < nbytes) {
    int ret = read_file_block(volume, inode, lbn, block_buf);
    if (ret < 0)
        return ret;

    /* Check for last block. */
    if (nbytes - bytes_read < ext2_blocksize(volume))
    copy_bytes = nbytes - bytes_read;
    memcpy(buffer + bytes_read, block_buf, copy_bytes);
    bytes_read += copy_bytes;
    lbn++;
  }
  return bytes_read;
}

/* follow_directory_entries: Reads all entries in a directory, calling
   function 'f' for each entry in the directory. Stops when the
   function returns a non-zero value, or when all entries have been
   traversed.
   
   Parameters:
     volume: Pointer to volume.
     inode: Pointer to inode structure for the directory.
     context: This pointer is passed as an argument to function 'f'
              unmodified.
     buffer: If function 'f' returns non-zero for any file, and this
             pointer is set to a non-NULL value, this buffer is set to
             the directory entry for which the function returned a
             non-zero value. If the pointer is NULL, nothing is
             saved. If none of the existing entries returns non-zero
             for 'f', the value of this buffer is unspecified.
     f: Function to be called for each directory entry. Receives three
        arguments: the file name as a NULL-terminated string, the
        inode number, and the context argument above.

   Returns:
     If the function 'f' returns non-zero for any directory entry,
     returns the inode number for the corresponding entry. If the
     function returns zero for all entries, or the inode is not a
     directory, or there is an error reading the directory data,
     returns 0 (zero).
 */
uint32_t follow_directory_entries(volume_t *volume,
  inode_t *inode, void *context, dir_entry_t *buffer,
   int (*f)(const char *name, uint32_t inode_no, void *context)) {
  if((inode->i_mode >> 12) != 0x4) return 0;
  uint64_t file_size=inode_file_size(volume,inode);
  unsigned char file_buffer[file_size];
  if (read_file_content(volume, inode,0,file_size , file_buffer) == -1) return 0;
  dir_entry_t *dir_buffer;

  char de_name_buffer[256];
	for(int i = 0;i < file_size;i += dir_buffer->de_rec_len) {
		dir_buffer = (dir_entry_t *) (file_buffer + i);
		memcpy(de_name_buffer, dir_buffer->de_name, dir_buffer->de_name_len);
    de_name_buffer[dir_buffer->de_name_len] = '\0';
    if (dir_buffer->de_inode_no != 0) {
        if(f(de_name_buffer, dir_buffer->de_inode_no, context) != 0){  
          if(buffer != NULL) {
            *buffer = *dir_buffer;
          }
			  return dir_buffer->de_inode_no;
		    }  
	  }
  }
  return 0;
}
//call follow_directory_entries(volume, &inode, 0, 0, print_dir_entry)
/*
 * Function to compare two null-terminated strings. Used by
 * find_file_in_directory to compare a component name against the names
 * stored in directory entries. Note that the inode argument is ignored.
 * Super tricky: strcmp returns 0 when the two strings are the same. Note
 * that we are returning !strcmp -- thus this returns non-zero when the
 * strings are the same!
 */
static int compare_file_name(const char *name, uint32_t inode_no, void *context) {
  return !strcmp(name, (char *) context);
}

/* find_file_in_directory: Searches for a file in a directory.
   
   Parameters:
     volume: Pointer to volume.
     inode: Pointer to inode structure for the directory.
     name: NULL-terminated string for the name of the file. The file
           name must match this name exactly, including case.
     buffer: If the file is found, and this pointer is set to a
             non-NULL value, this buffer is set to the directory entry
             of the file. If the pointer is NULL, nothing is saved. If
             the file is not found, the value of this buffer is
             unspecified.

   Returns:
     If the file exists in the directory, returns the inode number
     associated to the file. If the file does not exist, or the inode
     is not a directory, or there is an error reading the directory
     data, returns 0 (zero).
 */
uint32_t find_file_in_directory(volume_t *volume, inode_t *inode, const char *name, dir_entry_t *buffer) {
  
  return follow_directory_entries(volume, inode, (char *) name, buffer, compare_file_name);
}

/* find_file_from_path: Searches for a file based on its full path.
   
   Parameters:
     volume: Pointer to volume.
     path: NULL-terminated string for the full absolute path of the
           file. Must start with '/' character. Path components
           (subdirectories) must be delimited by '/'. The root
           directory can be obtained with the string "/".
     dest_inode: If the file is found, and this pointer is set to a
                 non-NULL value, this buffer is set to the inode of
                 the file. If the pointer is NULL, nothing is
                 saved. If the file is not found, the value of this
                 buffer is unspecified.

   Returns:
     If the file exists, returns the inode number associated to the
     file. If the file does not exist, or there is an error reading
     any directory or inode in the path, returns 0 (zero).
 */
uint32_t find_file_from_path(volume_t *volume, const char *path, inode_t *dest_inode) {



	if (path[0] != '/') return 0;
  if(strlen(path) == 1) {
    if(dest_inode != NULL) {
			read_inode(volume, EXT2_ROOT_INO, dest_inode);
      return EXT2_ROOT_INO;
	  }else{
      return EXT2_ROOT_INO;
    }
	}else{
    inode_t temp_inode;
	  inode_t *inode = &temp_inode;
    dir_entry_t dir;
	  if(dest_inode != NULL) {
		  inode = dest_inode;
	  }
		read_inode(volume, EXT2_ROOT_INO, inode); // root inode
    //reference: https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm
    const char s[2] = "/";
    char *token;
    char path_copy[strlen(path)];
  	strcpy(path_copy, path);
    token = strtok(path_copy, s);//first token
    while( token != NULL ) { 
      if(find_file_in_directory(volume, inode, token, &dir) == 0) return 0;
      token = strtok(NULL, s);
      read_inode(volume, dir.de_inode_no, inode);
    }
    return dir.de_inode_no;
  }
  return 0;
}
