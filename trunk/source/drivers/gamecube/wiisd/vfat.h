/****************************************************************************
* FAT16 - VFAT Support
*
* NOTE: Only supports FAT16 with Long File Names
*       I have no interest in adding FAT32
*
* Reference Documentation:
*
*	FAT: General Overview of On-Disk Format
*	Version 1.02 May 05, 1999
*	Microsoft Corporation
*
*       FAT: General Overview of On-Disk Format
*	Version 1.03 December 06, 2000
*	Microsoft Corporation
*
* This is targetted at MMC/SD cards.
*
* Copyright softdev 2007. All rights reserved.
*
* $Date: 2007-08-05 14:27:48 +0100 (Sun, 05 Aug 2007) $
* $Rev: 8 $
****************************************************************************/
#ifndef __FATVFAT__
#define __FATVFAT__

#include "integer.h"

/* x86 type definitions */
/**\typedef VDWORD
 * Unsigned 32 bit integer
 */
typedef unsigned int VDWORD;
/**\typedef WORD
 * Unsigned 16 bit short
 */
//typedef unsigned short WORD;
/**\typedef BYTE
 * Unsigned 8 bit char
 */
//typedef unsigned char BYTE;

/* Big Endian Support */
#ifdef WORDS_BIGENDIAN
#define SWAP16(a) (((a&0xff)<<8) | ((a&0xff00)>>8))
#define SWAP32(a) (((a&0xff000000)>>24) | ((a&0xff0000) >> 8) | ((a&0xff00)<<8) |((a&0xff)<<24))
#else
#define SWAP16(a) (a)
#define SWAP32(a) (a)
#endif

/* General */
/**\def SECTOR_SIZE
 * Set to sector size of device\n
 * Default is 512, which is suitable for most SD cards\n
 * All sector sizes must be a power of 2
 */
#define SECTOR_SIZE		512		/* Default sector is 512 bytes */
/**\def SECTOR_SHIFT_BITS
 * Number of shift bits for faster multiplication and division
 */
#define SECTOR_SHIFT_BITS	9		/* Sector shift bits */
#define LFN_LAST_ENTRY		0x40		/* Long File Name last entry mask */
#define LFN_ENTRY_MASK		0x3F		/* Long File Name entry number mask */
#define ROOTCLUSTER		0xdeadc0de	/* Unique root directory cluster id */

/**\def PSEP
 * Path separator. Default unix style /
 */
#define PSEP			'/'		/* Path separator. Default unix / */
/**\def PSEPS
 * Path separator string. Default unix style /
 */
#define PSEPS			"/"		/* Path separator string. Default unix / */

#define DIR_ROOT		"."		/* Root directory entry */
#define DIR_PARENT		".."		/* Parent directory entry */

/* FSTYPES */
#define FS_TYPE_NONE	0
#define FS_TYPE_FAT16	1

/* Errors */
#define FS_FILE_OK	0
#define FS_SUCCESS	FS_FILE_OK
#define FS_ERR_NOMEM	-128
#define FS_NO_FILE	-64
#define FS_ERR_IO	-32
#define FS_ERR_PARAM	-16

/* File modes */
#define FS_READ		1

/* Gamecube Specific */
#define FS_SLOTA	0
#define FS_SLOTB	1

/* FAT12/16 */
typedef struct
  {
    BYTE jmpBoot[3];	/**< Always 0xEBxx90 or 0xE9xxxx */
    BYTE OEMName[8];	/**< OEM Name 'MSWIN4.1' or similar */
    WORD bytesPerSec;	/**< Bytes per sector */
    BYTE secPerClust;	/**< Sectors per cluster */
    WORD reservedSec;	/**< Reserved Sector Count */
    BYTE numFATs;	/**< Number of FAT copies */
    WORD rootEntCount;	/**< FAT12/16 number of root entries. */
    WORD totSec16;	/**< Sector count if < 0x10000 */
    BYTE media;		/**< Media ID byte (HD == 0xF8) */
    WORD FATsz16;	/**< Sectors occupied by one copy of FAT */
    WORD secPerTrack;	/**< Sectors per track */
    WORD numHeads;	/**< Number of heads */
    VDWORD hiddenSec;	/**< Hidden sector count */
    VDWORD totSec32;	/**< Total sectors when >= 0x10000 */
    BYTE drvNum;	/**< BIOS Drive Number (0x80) */
    BYTE reserved1;	/**< Unused - always zero */
    BYTE bootSig;	/**< Boot signature */
    VDWORD volID;	/**< Volume serial number */
    BYTE volName[11];	/**< Volume Name */
    BYTE FilSysType[8];	/**< File system type */
    BYTE filler[SECTOR_SIZE-64]; /**< Byte padding */
    BYTE sigkey1;	/**< 0x55 */
    BYTE sigkey2;	/**< 0xAA */
  }
#ifndef DOXYGEN_FIX_ATTR
__attribute__((__packed__))
#endif
BPB16;

/* Partition entry */
typedef struct
  {
    BYTE bootindicator;		/**< Boot indicator 00 == No 0x80 == Boot */
    BYTE startCHS[3];		/**< Start Cylinder / Head / Sector */
    BYTE partitiontype;		/**< Partition Type. ID 06 FAT 16 */
    BYTE endCHS[3];		/**< End Cylinder / Head / Sector */
    VDWORD partitionstart;	/**< LBA Start Sector */
    VDWORD partitionsize;	/**< Partition size in sectors */
  }
#ifndef DOXYGEN_FIX_ATTR
__attribute__((__packed__))
#endif
PARTENTRY;

/* VFAT - Main structure */
typedef struct
  {
    VDWORD	BaseOffset;		/**< Offset in sectors to BPB */
    VDWORD	SectorsPerCluster;	/**< Sectors per cluster (Native) */
    VDWORD	BytesPerSector;		/**< Bytes per sector (Native) */
    VDWORD	ReservedSectors;	/**< Reserved sectors (Native) */
    VDWORD	RootDirSectors;		/**< Root directory sectors (Native) */
    VDWORD	SectorsPerFAT;		/**< Sectors per FAT (Native) */
    VDWORD	NumberOfFATs;		/**< Number of FAT copies (Native) */
    VDWORD	FirstDataSector;	/**< First data sector offset (Native) */
    VDWORD	TotalSectors;		/**< Total sectors (Native) */
    VDWORD	CountOfClusters;	/**< Count of clusters (Native) */
    VDWORD	DataSectors;		/**< Number of Data sectors (Native) */
    VDWORD	RootDirOffset;		/**< Offset of root directory (Native) */
    VDWORD	FirstFATOffset;		/**< Offset to first copy of FAT (Native) */
    VDWORD	RootDirEntries;		/**< Number of root directory entries (Native) */
    WORD	*FAT;			/**< Holds first FAT copy */
    BYTE	*rootDir;		/**< Holds entire root directory */
  }
#ifndef DOXYGEN_FIX_ATTR
__attribute__((__packed__))
#endif
VFATFS;

/*
 * Directory Functions
 */

#define MAX_LONG_NAME		256

/* Directory entry attributes */
#define ATTR_READ_ONLY		0x01
#define ATTR_HIDDEN		0x02
#define ATTR_SYSTEM		0x04
#define ATTR_VOLUME_ID		0x08
#define ATTR_DIRECTORY		0x10
#define ATTR_ARCHIVE		0x20
#define ATTR_LONG_NAME		(ATTR_READ_ONLY | \
				 ATTR_HIDDEN | \
				 ATTR_SYSTEM | \
				 ATTR_VOLUME_ID )

#define ATTR_LONG_NAME_MASK	( ATTR_READ_ONLY | \
				  ATTR_HIDDEN | \
				  ATTR_SYSTEM | \
				  ATTR_VOLUME_ID | \
				  ATTR_DIRECTORY | \
				  ATTR_ARCHIVE )

/**\def CLUSTER_END_CHAIN
 * Any value equal or greater than 0xFFF8 should be
 * considered an end of chain marker.
 */
#define CLUSTER_END_CHAIN	  0xFFF8

/**\def CLUSTER_BAD
 * Documentation states that any BAD or unusable sector should be marked\n
 * as 0xFFF7.
 */
#define CLUSTER_BAD		  0xFFF7

/* Short file name */
typedef struct
  {
    BYTE dirname[11];	/**< Record name */
    BYTE attribute;	/**< Attributes */
    BYTE NTReserved;	/**< Reserved for Windows NT - set 0 */
    BYTE dirTenthSecs;	/**< Tenth of a second, 0-199 */
    WORD dirCreateTime;	/**< Time of creation */
    WORD dirCreateDate;	/**< Date of creation */
    WORD dirLastAccDate;/**< Date of last access */
    WORD fstClustHigh;	/**< High word of first cluster - ZERO on FAT16 (LE)*/
    WORD dirWriteTime;	/**< Time of last write */
    WORD dirWriteDate;	/**< Date of last write */
    WORD fstClustLow;	/**< Low word of first cluster (LE)*/
    VDWORD filesize;	/**< Filesize in bytes (LE)*/
  }
#ifndef DOXYGEN_FIX_ATTR
__attribute__((__packed__))
#endif
SFNDIRREC;

/* Long file name */
typedef struct
  {
    BYTE ordinal;	/**< Entry number (0x4x) == Last entry */
    BYTE dirname1[10];  /**< First part of filename in unicode */
    BYTE attribute;	/**< Attributes - MUST be ATTR_LONG_NAME (0x0F) */
    BYTE type;		/**< Reserved */
    BYTE checksum;	/**< SFN Checksum */
    BYTE dirname2[12];  /**< Second part of filename in unicode */
    WORD fstClustLo;	/**< MUST BE ZERO! */
    BYTE dirname3[4];	/**< Third part of filename in unicode */
  }
#ifndef DOXYGEN_FIX_ATTR
__attribute__((__packed__))
#endif
LFNDIRREC;

/* User dir entry */
typedef struct
  {
    BYTE	longname[MAX_LONG_NAME];	/**< Long file name, application friendly */
    BYTE	shortname[13];			/**< Short file name, application friendly */
    VDWORD	fpos;				/**< Current file position */
    VDWORD	fsize;				/**< File size in bytes (Native) */
    VDWORD	driveid;			/**< Device number */
    VDWORD	FirstCluster;			/**< First cluster in chain (Native) */
    VDWORD	CurrentCluster;			/**< Current cluster in chain (Native) */
    VDWORD	CachedCluster;			/**< Cached cluster (Native) */
    VDWORD	CurrentDirEntry;		/**< Current directory entry in current cluster (Native) */
    VDWORD	crosscluster;			/**< Record crosses cluster boundary */
    BYTE	*clusterdata;			/**< Cluster data */
    /* Now a copy of the current directory entry */
    SFNDIRREC dirent;				/**< Copy of physical SFNDIRREC */
  }
#ifndef DOXYGEN_FIX_ATTR
__attribute__((__packed__))
#endif
FSDIRENTRY;

/* VFAT API */
/** \defgroup VFATDir VFATFS Directory Level Functions
 * \brief VFAT FS Directory Functions
 */

/* Directory */

/** \ingroup VFATDir
 *  \brief Open a directory
 *  \param drive Device number to use
 *  \param d Pointer to user provided FSDIRENTRY
 *  \param search Path to open
 *  \return FS_SUCCESS if succesful
 */
int VFAT_opendir( int drive, FSDIRENTRY *d, char *search );

/** \ingroup VFATDir
 *  \brief Read directory entries
 *  \param d Pointer to user provided FSDIRENTRY, previously populated from VFAT_opendir
 *  \return FS_SUCCESS if successful
 */
int VFAT_readdir( FSDIRENTRY *d );

/** \ingroup VFATDir
 *  \brief Close a previously opened directory
 *  \param d Pointer to user provided FSDIRENTRY, previously populated from VFAT_opendir
 *  \return None.
 */
void VFAT_closedir( FSDIRENTRY *d );

/** \defgroup VFATFile VFATFS File Level Functions
 *  \brief VFAT FS File functions
 */

/** \ingroup VFATFile
 *  \brief Open a file
 *  \param drive Device number to use
 *  \param d Pointer to user provided FSDIRENTRY
 *  \param fname Filename, including full path, to open
 *  \param mode Currently only FS_READ is supported
 *  \return FS_SUCCESS if successful.
 */
int VFAT_fopen( int drive, FSDIRENTRY *d, char *fname, int mode );

/** \ingroup VFATFile
 *  \brief Close a previously opened file
 *  \param d Pointer to user provided FSDIRENTRY, previously populated by VFAT_fopen
 *  \return None.
 */
void VFAT_fclose( FSDIRENTRY *d );

/** \ingroup VFATFile
 *  \brief Read from an open file
 *  \param d Pointer to user provided FSDIRENTRY, populated by a successful call to VFAT_fopen
 *  \param buffer Buffer to receive data
 *  \param length Number of bytes to read to the buffer
 *  \return Number of bytes read if positive. If <0 an error has occurred.
 */
int VFAT_fread( FSDIRENTRY *d, void *buffer, int length );

/** \ingroup VFATFile
 *  \brief Return the current file position
 *  \param d Pointer to user provided FSDIRENTRY
 *  \return Current file position
 */
int VFAT_ftell( FSDIRENTRY *d );

/** \ingroup VFATFile
 *  \brief Move current file pointer to the requested position
 *  \param d Pointer to user provided FSDIRENTRY
 *  \param where New byte position
 *  \param whence Define movement type. SEEK_SET, SEEK_END and SEEK_CUR are supported
 *  \return FS_SUCCESS if successful
 */
int VFAT_fseek( FSDIRENTRY *d, int where, int whence );

/** \defgroup VFATMount VFATFS Device Mount Functions
 *  \brief Device mount / unmount functions
 */

/** \ingroup VFATMount
 *  \brief Mount a device
 *  \param driveid Device number to mount
 *  \param v Pointer to user provided VFATFS
 *  \return FS_TYPE_FAT16 on success.
 */
int VFAT_mount( int driveid, VFATFS *v );

/** \ingroup VFATMount
 *  \brief Unmount a device
 *  \param driveid Device drive number
 *  \param v Pointer to user provided VFATFS
 *  \return None
 */
void VFAT_unmount( int driveid, VFATFS *v );

#endif

