#ifndef MAIN_H_
#define MAIN_H_

// Mount path for espfs (read-only filesystem)
#define ESPFS_BASE_PATH "/espfs"

// Mount path for the internal SPI-Flash file storage partition (read-write)
#define FS_BASE_PATH	"/internalfs"
#define FS_PART_NAME	"internalfs"

#define BLEFTP_BASE_PATH	FS_BASE_PATH
#define BLEFTP_BASE_PATH2	ESPFS_BASE_PATH

#endif /* MAIN_H_ */
