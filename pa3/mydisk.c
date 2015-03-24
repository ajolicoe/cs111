#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "mydisk.h"

/*
 * Create a new disk in a file
 * Parameters: 
 * - char *disk_name  --  a string representing the name of the file that will contain the disk
 * - int size -- the size (in blocks) of the "disk"
 */

struct directory
{
   short int next;
   short int iNodeLocation[63];
   char filename[63][6];
};

typedef struct directory* directory;

struct iNode
{
   short int size;
   short int blocks[255];
};

typedef struct iNode* iNode;

struct fileD
{
   short int blockNum;
   char data[512];
};

short int findFreeMemory(disk_t);
short int createFile(disk_t, char filename[6]);
void createiNode(disk_t,short  int location, int length, char filename[6]);
int writeRoot(disk_t, char filename[6], short int iNodeLocation);

void printByteMap(disk_t disk)
{
   char byteMap[512];
   readblock(disk, 3, byteMap);
   int n;
   printf("Bytemap: ");
   for (n = 0; n < 512; n++) {
      if ((n%32) == 0) printf("\n\t");
      printf("%d", byteMap[n]);
   }
   printf("\n\n");
}

void createdisk(char *disk_name, int size)
{
   printf("Creating disk %s\n", disk_name);
   int fd;
   disk_t disk = malloc(BLOCK_SIZE);
   ssize_t bytes_written;

   fd = creat(disk_name, S_IRWXU);

   if(fd == -1) {
      printf("createdisk creat() failed\n");
      perror("createdisk");
      exit(-1);
   }

   // Write the partition table at disk block 0
   disk->fd = -1;
   disk->last_block = -1;
   disk->block_size = BLOCK_SIZE;
   disk->size = size;

   bytes_written = write(fd, disk, BLOCK_SIZE);

   if(bytes_written == -1) {
      printf("createdisk: write failed\n");
      perror("createdisk");
      exit(-1);
   }

   if(bytes_written != disk->block_size) {
      printf("createdisk: write failed, too few bytes written\n");
      perror("createdisk");
      exit(-1);
   }

   // Close the file and free up the disk structure
   close(disk->fd);
   free(disk);

   return;
}

/*
 * Open a disk file and read the partition table to get the disk parameters
 * Parameters: 
 * - char *disk_name -- the name of the disk file
 */
disk_t opendisk(char *disk_name)
{
   int fd;
   disk_t disk = malloc(BLOCK_SIZE);

   printf("Opening disk %s\n", disk_name);
   fd = open(disk_name, O_RDWR);

   if(fd == -1) {
      printf("opendisk failed\n");
      perror("opendisk");
      exit(-1);
   }

   // Read the partition table
   read(fd, (void *) disk, BLOCK_SIZE);
   disk->fd = fd;  // Set the fd

   printf("Disk %s opened. size = %d, block size = %d\n", disk_name, disk->size, disk->block_size);

   return(disk);
}


// Seeks to the specified location in the disk partition
// Note that we add 1 to block to avoid the partition table
int seekblock(disk_t disk, int block)
{
   off_t offset;

   // Check the block number against the beginning of the disk
   if(block < 0) {
      printf("seekblock: tried to seek below the beginning of the disk!\n");
      printf("seekblock: seek block %d, disk size is %d.\n", block, disk->size);
      exit(-1);
   }

   // Check the block number against the size of the disk
   if(block >= disk->size) {
      printf("seekblock: tried to seek past the end of the disk!\n");
      printf("seekblock: seek block %d.  Disk size is %d, so last block is %d.\n", block, disk->size, disk->size-1);
      exit(-1);
   }

   // Seek to the specified location
   // Note that we add 1 to avoid the partition table
   offset = lseek(disk->fd, (block+1)*disk->block_size, SEEK_SET);

   // Make sure the seek worked
   if(offset == -1) {
      printf("seekblock: lseek failed\n");
      perror("seekblock");
      exit(-1);
   }
   // Update last_block so the next read or write will work properly
   disk->last_block = block;
}

// Reads and writes one block of data
int readblock(disk_t disk, int block, unsigned char *databuf) 
{
   off_t offset;
   ssize_t bytes_read;

   if(block < 0) {
      printf("readblock: There are no blocks < 0!!!\n");
      exit(-1);
   }

   // Check the block number against the size of the disk
   if(block >= disk->size) {
      printf("readblock: tried to read past the end of the disk!\n");
      printf("readblock: read block %d, disk size is %d.\n", block, disk->size);
      exit(-1);
   }

   // Seek to the right location, if non_sequential
   if(block != disk->last_block+1) seekblock(disk, block);

   bytes_read = read(disk->fd, (void *)databuf, disk->block_size);

   // First see if the read worked at all
   if(bytes_read == -1) {
      printf("readblock read failed\n");
      perror("readblock");
      exit(-1);
   }

   // Then see if we got the right number of bytes
   if(bytes_read != disk->block_size) {
      printf("readblock: read failed, too few bytes read\n");
      printf("readblock: probably because you read data that was not previously written.\n");
      perror("readblock");
      exit(-1);
   }

   // Change last_block to indicate what we have already read
   disk->last_block = block;

   return 0;

}

int writeblock(disk_t disk, int block, unsigned char *databuf)
{
   off_t offset;
   ssize_t bytes_written;

   if(block < 0) {
      printf("writeblock: There are no blocks < 0!!!\n");
      exit(-1);
   }

   // Check the block number against the size of the disk
   if(block >= disk->size) {
      printf("writeblock: tried to write past the end of the disk!\n");
      printf("writeblock: write block %d, disk size is %d.\n", block, disk->size);
      exit(-1);
   }

   // Seek to the right location, if non_sequential
   if(block != disk->last_block + 1) seekblock(disk, block);

   bytes_written = write(disk->fd, databuf, disk->block_size);

   if(bytes_written == -1) {
      printf("writeblock failed\n");
      perror("writeblock");
      exit(-1);
   }

   if(bytes_written != disk->block_size) {
      printf("writeblock: write failed, too few bytes written\n");
      perror("writeblock");
      exit(-1);
   }

   disk->last_block = block;

   return 0;
}

// creates and writes the superblock, free block map,
// and root directory to a given disk.
int formatDisk(disk_t disk)
{
   int i;
   char byteMap[disk->size];   // free block byte map
   unsigned char buffer[512]; // buffer for block data
   struct directory root;     // the first inode of our singly linked list

   sprintf(buffer, "size = %d\nroot = 2\nbytemap = 3\ndata starts at 4", disk->size);
   
   // write the super block
   writeblock(disk, 1, buffer);
   
   // first four blocks are reserved by our file system
   byteMap[0] = byteMap[1] = byteMap[2] = byteMap[3] = 1;
   // the rest are free (0)
   for(i = 4; i < disk->size; i++) byteMap[i] = 0;

   // write the free block byte map
   writeblock(disk, 3, byteMap);

   // clear the rest of the disk
   for(i = 0; i < 512; i++){
      buffer[i] = '\0';
   }
   for(i = 4; i < disk->size; i++){
      writeblock(disk, i, buffer);
   }

   // initialize all filenames, and their locations
   for(i = 0; i < 63; i++){
      root.filename[i][0] = '\0';
      root.filename[i][1] = '\0';
      root.filename[i][2] = '\0';
      root.filename[i][3] = '\0';
      root.filename[i][4] = '\0';
      root.filename[i][5] = '\0';
      root.iNodeLocation[i] = '\0';
   }
   root.next = -1;
   
   // write the root directory
   writeblock(disk, 2, (unsigned char *)&root);
   return 0;
}

// finds an iNode on the disk given a filename
// returns -1 if the filename's iNode is not found
short int findiNode(disk_t disk, char filename[6])
{
   struct directory root;
   int i;
   char newFlag = 0;
   for(readblock(disk, 2, (unsigned char *)&root);
      root.next != 0;
      readblock(disk, root.next, (unsigned char*)&root)) {
      for(i = 0; i < 63; i++) {
         if(strcmp(root.filename[i], "\0") == 0)  return -1; // no holes (deleting files)
         if(strcmp(root.filename[i], filename) == 0) return i;
      } 
   }
   return -1;
}

// finds free memory on a pre-formatted disk
// returns free block number
short int findFreeMemory(disk_t disk)
{
   short int i;
   char byteMap[512];
   readblock(disk, 3, byteMap);
   for(i = 0; i < 512; i++){
      if(byteMap[i] == 0){
         byteMap[i] = 1;
         writeblock(disk, 3, byteMap);
         break;
      }
   }
   
   if(i == 512) {
      printf("No more free blocks\n");
      exit(EXIT_FAILURE);
   }
   return i;
}

// write an iNode to the disk and save the root directory
void createiNode(disk_t disk, short int fileLocation, int length, char filename[6])
{
   short int i = findFreeMemory(disk);
   struct iNode newish;
   newish.size = length;
   newish.blocks[0] = fileLocation;
   writeblock(disk, i, (unsigned char *)&newish);
   writeRoot(disk, filename, i);
}
    

// creates a new file and its iNode
short int createFile(disk_t disk, char filename[6])
{
   short int i;
   int length;
   i = findFreeMemory(disk);
   writeblock(disk, i, "\0");
   createiNode(disk, i, 0, filename);
   return i;
}

void updateFile(disk_t disk, char filename[6], char* file)
{
   short int location = findiNode(disk, filename);
   //printf("LOCATIONS: %d", location);
   int size = sizeof(file);
   writeblock(disk, location, file);
   //int length = strlen(file);
   createiNode(disk, location, size, filename);
}

// either open the file if its iNode is found, or create a new one if not
fileD openFile(disk_t disk, char filename[6])
{
   short int location = findiNode(disk, filename);
   fileD totalGym;
   totalGym = (fileD)malloc(sizeof(struct fileD));
   struct iNode myNode;
   struct directory root;
   if (location == -1) {
      // iNode was not found, must create a new one
      location = createFile(disk, filename);
      totalGym->blockNum = location;
   } else {
      // iNode was found, we can use the existing iNode and its data
      readblock(disk, 2, (unsigned char *)&root);
      readblock(disk, root.iNodeLocation[location], (unsigned char *)&myNode);
      // change this if files span more than one block
      readblock(disk, myNode.blocks[0], (unsigned char *)&(totalGym->data));
      totalGym->blockNum = myNode.blocks[0];
   }
   return totalGym;
}

// read a filedescriptor and return its data
int readFile(disk_t disk, fileD fileDescriptor, char *data)
{
   int i;
   if(fileDescriptor == 0){
      printf("Error: attempting read on un-opened file\n\n");
      data[0] = '\0';
      return 0;
   }
   readblock(disk, fileDescriptor->blockNum, (unsigned char *)&(fileDescriptor->data));
   for(i = 0; i < 512; i++) data[i] = fileDescriptor->data[i];
   return strlen(data);
}

// writes data to a file, newdata parameter must be a null terminated string
void writeFile(disk_t disk, fileD file, char* newdata)
{
   if(file == 0){
      printf("Error: attempting write on un-opened file\n\n");
      return;
   }
   int i;
   for(i = 0; i < 512; i++) file->data[i] = newdata[i];
   writeblock(disk, file->blockNum, file->data);
}

// close the file, and write the data
void closeFile(disk_t disk, fileD *fileDescriptor)
{
   free(*fileDescriptor);
   *fileDescriptor = 0;
}

// write the filename and inode location into the root directory
int writeRoot(disk_t disk, char filename[6], short int iNodeLocation)
{
   struct directory root;
   int i;
   int count = 0;
   short int rootLocation = 2;
   do {
       readblock(disk, rootLocation, (unsigned char *)&root);
       for(i = 0; i < 63; i++){
           if((strcmp(root.filename[i], "\0") == 0) || (strcmp(root.filename[i], filename) == 0)){
              root.filename[i][0] = filename[0];
              root.filename[i][1] = filename[1];
              root.filename[i][2] = filename[2];
              root.filename[i][3] = filename[3];
              root.filename[i][4] = filename[4];
              root.filename[i][5] = '\0';
              root.iNodeLocation[i] = iNodeLocation;
              break;
           }
      }
      if(i != 63) break;
      else if(root.next != -1) rootLocation = root.next;
      else { break; }
   } while(root.next != -1);

   if(i == 63){
      short int temp = findFreeMemory(disk);
      struct directory new;
      new.filename[0][0] = filename[0];
      new.filename[0][1] = filename[1];
      new.filename[0][2] = filename[2];
      new.filename[0][3] = filename[3];
      new.filename[0][4] = filename[4];
      new.filename[0][5] = filename[5];
      new.iNodeLocation[0] = iNodeLocation;
      root.next = temp;
      writeblock(disk, rootLocation, (unsigned char *)&root);
      rootLocation = temp;
      root = new;
   }
   writeblock(disk, rootLocation, (unsigned char *)&root);
   return 0;
}

