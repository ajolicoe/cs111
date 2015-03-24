Purpose:
This document specifies the design of a disk file system simulated in a file.

Data:

   directory: 
      data structure for a directory, such as the root directory.
      
      directory.next -
         the pointer to the next block's directory
      
      directory.iNodeLocation[63] -
         an array containing all of the iNode locations for this directory.

      directory.filename[63][6] -
         an array of six character long filenames (including the "\0").

   iNode:
      data structure for an iNode.

      iNode.size -
         size of the iNode data

      iNode.blocks[255] -
         array of iNode's data block locations.

   fileD:
      data structure for a file descriptor.

      fileD.blockNum -
         the starting block number of the file

      fileD.data[512] -
         the file's data array
      

Operations:

Algorithms:

