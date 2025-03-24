#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/fs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ext2fs/ext2_fs.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <device>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY); // Reads from /dev/sdb or any ext2 fs
    if (fd == -1) {
        perror("Error opening device");
        return errno;
    }

    struct ext2_super_block superblock;
    struct ext2_group_desc blockgroupdesc[1024];
    
    // Move to the start of the superblock (always at 1024 bytes)
    lseek(fd, 1024, SEEK_SET);

    // Read superblock
    
    if (read(fd, &superblock, sizeof(struct ext2_super_block)) != sizeof(struct ext2_super_block)) {
        perror("Error reading superblock");
        close(fd);
        return errno;
    }

    
    
    unsigned int block_size = 1024 << superblock.s_log_block_size;
    
    int ngroups = (superblock.s_blocks_count + superblock.s_blocks_per_group - 1) / superblock.s_blocks_per_group;  // ROUNDOFF
    
    printf("Magic number: 0x%x\n", superblock.s_magic);
    printf("Total Inodes: %d\n", superblock.s_inodes_count);
    printf("Block size entry: %d \n", superblock.s_log_block_size);

    // Number of block groups
    
    printf("Number of block groups: %d\n", ngroups);


    // Move to block group descriptor table
    
    int blockdesc_offset = (block_size == 1024) ? 2048 : block_size;
    
    lseek(fd, blockdesc_offset, SEEK_SET);

    // Read group descriptors
    if (read(fd, blockgroupdesc, sizeof(struct ext2_group_desc) * ngroups) != sizeof(struct ext2_group_desc) * ngroups) {
        perror("Error reading group descriptors");
        close(fd);
        return errno;
    }

    // Print inode table locations for each group
    for (int i = 0; i < ngroups; i++) {
        printf("Group %d: Inode Table Block = %u\n", i, blockgroupdesc[i].bg_inode_table);
    }

    close(fd);
    return 0;
}

