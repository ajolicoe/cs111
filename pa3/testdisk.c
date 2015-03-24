#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mydisk.h"
void testFile(disk_t);
void copyFile(disk_t, char *filename);

void main(int argc, char *argv[])
{
   char *disk_name;
   disk_t disk;
   int i, j;

   // Read the parameters
   if(argc != 2) {
      printf("Usage: testdisk <disk_name>\n");
      exit(-1);
   }

   disk_name = (char *)argv[1];

   // Open the disk
   disk = opendisk(disk_name);
   testFile(disk);

   copyFile(disk, "lime");
   copyFile(disk, "groc");
   copyFile(disk, "raven");
   exit(EXIT_SUCCESS);
}

void testFile(disk_t disk)
{
   char *fileData = "Hello world!\0Hello world!";
   char testRead[512];
   char testRead2[512];

   printf("\n------FILE TEST------\n");
   printf("\nformatting disk...\n");
   formatDisk(disk);
   
   printf("printing bytemap...\n");
   printByteMap(disk);

   printf("creating file descriptor...\n");
   fileD file;
   
   printf("opening an empty test file...\n");
   file = openFile(disk, "test1");

   printf("reading the empty test file...\n");
   if(!readFile(disk, file, testRead)) {
      printf("the file does not exist\n");
   }

   printf("\n------FILE DATA------\n\n");
   printf("%s\n\n------FILE TEST------\n\n", testRead);
   
   printf("writing to the test file...\n");
   writeFile(disk, file, fileData);
   
   printf("reading the test file...\n");
   readFile(disk, file, testRead);
   
   printf("\n------FILE DATA------\n\n");
   printf("%s\n\n------FILE TEST------\n\n", testRead);

   printf("closing the test file...\n");
   closeFile(disk, &file);
   
   printf("printing free block bytemap...\n");
   printByteMap(disk);

   fileD newFile;
   
   printf("writing some data to the closed test file...\n");
   writeFile(disk, file, fileData);

   printf("reading the closed test file...\n");
   readFile(disk, file, testRead);
   
   printf("opening the test file again...\n");
   newFile = openFile(disk, "test1");
   
   printf("reading the test file...\n");
   readFile(disk, newFile, testRead2);
   
   printf("\n------FILE DATA------\n\n");
   printf("%s\n", testRead2);

}

void copyFile(disk_t disk, char *filename)
{
   char data[512];
   FILE *file;
   size_t readSize;
   char testRead[512];

   file = fopen(filename, "r");
   if (!file) {
      printf("unable to open file: %s\n", filename);
   } else {
      printf("\n------COPY FILE------\n\n");
      printf("read from file: \"%s\" in local folder...\n\n", filename);
      while ((readSize = fread(data, 1, sizeof data, file)) > 0) {
         fwrite(data, 1, readSize, stdout);
         printf("\nfile size: %d bytes\n\n", (int)readSize);
         data[(int)readSize] = '\0';
      }
      fclose(file);

      char *dataPtr = data;
      fileD fd;
      printf("opening file \"%s\" on the disk...\n", filename);
      fd = openFile(disk, filename);
      printf("writing data to disk...\n");
      writeFile(disk, fd, dataPtr);
      printf("reading file \"%s\" from the disk...\n", filename);
      readFile(disk, fd, testRead);
      printf("\nread from file \"%s\" on the disk...\n\n%s\n", filename, testRead);
      printf("file size: %d bytes\n", (int)strlen(testRead));
   }
}

