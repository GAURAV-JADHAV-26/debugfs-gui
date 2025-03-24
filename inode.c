#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ext2fs/ext2_fs.h>

#define _FILE_OFFSET_BITS 64 // used for lseek64
#define _LARGEFILE64_SOURCE

int main(int argc, char *argv[]) {


	

	struct ext2_super_block sb;



    if (argc < 3) {
    
        printf("Enter in this format : sudo ./inode /dev/sdb inode_number");
        return 1;
        
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("Error opening device");
        return errno;
    }

    lseek64(fd, 1024, SEEK_SET); // Move to superblock

    
    read(fd, &sb, sizeof(struct ext2_super_block));


    int block_size = 1024 << sb.s_log_block_size;
    
    
    unsigned int offset_from_superblock = (block_size == 1024) ? 2048 : block_size;  // 4096


    printf("Block size: %d bytes\n", block_size);
    printf("Offset to group descriptors: %u\n", offset_from_superblock);

    int inode_number = atoi(argv[2]);

    int number_of_groups = (sb.s_blocks_count + sb.s_blocks_per_group - 1) / sb.s_blocks_per_group;
    
    
    printf("Number of groups in this partition: %d\n", number_of_groups);

    short int inode_size = sb.s_inode_size;
    printf("Inode size: %d bytes\n", inode_size);

    int groupnumber = (inode_number - 1) / sb.s_inodes_per_group;
    printf("Group number of given inode: %d\n", groupnumber);

    int index_of_inode_in_group = (inode_number - 1) % sb.s_inodes_per_group;

    // Read group descriptors
    lseek64(fd, offset_from_superblock, SEEK_SET);
    struct ext2_group_desc grpdescs[number_of_groups];
    read(fd, grpdescs, sizeof(struct ext2_group_desc) * number_of_groups);

    printf("Group %d: Inode Table starts at block %d\n", groupnumber, grpdescs[groupnumber].bg_inode_table);

    // Locate the inode in the inode table
    int offset = grpdescs[groupnumber].bg_inode_table * block_size;
    offset += index_of_inode_in_group * inode_size;

    lseek64(fd, offset, SEEK_SET);

    struct ext2_inode required_inode;
    read(fd, &required_inode, sizeof(struct ext2_inode));

    printf("Size of file: %d bytes\n", required_inode.i_size);
    printf("Number of blocks: %d\n\n", required_inode.i_blocks);

    printf("Contents of File:\n\n");

    if (required_inode.i_size > 0) {
    
		    char *data = (char *)malloc(required_inode.i_size + 1);
		    
		    if (!data) {
		        perror("Memory allocation failed");
		        close(fd);
		        return 1;
		    }

		    for (int i = 0; i < required_inode.i_blocks; i++) {
		        
				    if (required_inode.i_block[i] == 0)
				        break;

				    offset = required_inode.i_block[i] * block_size;
				    
				    lseek64(fd, offset, SEEK_SET);
				    
				    read(fd, data, required_inode.i_size);
				    
				    data[required_inode.i_size] = '\0'; // Null-terminate for safety
				    
				    printf("%s", data);
		    }

		    free(data);
    } 
    
    else {
    
        printf("File is empty.\n");
    }

    close(fd);
    return 0;
}

