#ifndef CACHESIM_H
#define CACHESIM_H

//#define DEBUG

typedef unsigned short	U_SHORT;
typedef unsigned long	U_LONG;
typedef unsigned long long	U_LLONG;

/*
 * Macro definitions for the Cache parameters which are
 * given at the command line when typing the 'cachesim'
 */
/* For the write hit policy */
#define WRITE_BACK	'B'
#define WRITE_THROUGH	'T'

/* For the write miss policy */
#define WRITE_ALLOCATE	'A'
#define WRITE_NO_ALLOCATE	'N'	/* Also known as 'write around' */


#define ONE_K	1024		/* 2^10 = 1024 (1K) */
#define WORD	2			/* 1 word = 2 bytes, may not used */
#define MEM_ADD_WIDTH	32	/* Physical memory address width (in bits) */

/* For the valid and dirty bits in the cache line */
#define INVALID	((U_SHORT)0)
#define VALID	((U_SHORT)1)
#define DIRTY	((U_SHORT)0)
#define CLEAN	((U_SHORT)1)

#define SSWR3(ff)	((double)((int)((ff)*10000+5)/10)/1000)


/*
 * The cell in the cache table
 */
typedef struct
{
	U_LONG	tag;		/* XuZF - Is long enough? */
	U_SHORT	valid:1;	/* 0 - invalid; 1 - valid */
	U_SHORT	dirty:1;	/* 0 - dirty; 1- clean */

	/*
	 * Here should be the data of the cache line,
	 * but it's been ignored in this case, as it's just simulation.
	 */
} CacheLine;

typedef struct
{
	U_LONG	SizeOfCache;	/* In bytes */
	U_LONG	SizeOfLine;		/* In bytes */
	U_LONG	associativity;	/* How many lines in the same entry */
	U_LONG 	NumOfEntries;
	char	WriteHit;		/* Write hit policy */
	char	WriteMiss;		/* Write miss policy */
	U_SHORT	CacheLatency;	/* In nano-seconds */
	U_SHORT	MemoryLatency;	/* In nano-seconds */

	CacheLine	* CacheTable;	/* Pointer to the cache table whose
								 * cell is 1 CacheLine, not an
								 * whole entry */
	U_LONG	* LRUList;		/* Pointer to the Least Recently Used list */
	U_LONG	* LRUIndicList;	/* Pointer to a list of indicators. Every entry
							 * needs one to indicate how many lines in the
							 * entry have been used. The value is between
							 * 0 ~ associativity
							 *
							 * Note: In every entry, the lines should be
							 * used in sequence, from Index 0 upwards; the
							 * valid bit of the used line must be 'valid'
							 */
} Cache;

/*
 * Contains the result of simulation
 */
typedef struct
{
	U_LONG	ReadAccess;
	U_LONG	WriteAccess;
	U_LONG	TotalAccess;

	U_LONG	ReadMiss;
	U_LONG	WriteMiss;
	U_LONG	TotalMiss;

	double	ReadRatio;	/* In percent */
	double	WriteRatio;
	double	TotalRatio;

	U_LONG	ReadTransfer;	/* In times */
	U_LONG	WriteTransfer;
	U_LONG	TotalTransfer;

	U_LONG	ReadBytes;	/* Transferred bytes */
	U_LONG	WriteBytes;
	U_LONG	TotalBytes;

	U_LONG	ReadAccTime;	/* In nano-seconds, all the 3 are total time, */
	U_LONG	WriteAccTime;	/* not average time; so need to be divide by  */
	U_LONG	TotalAccTime;	/* the number of access to get average time   */

	double	ReadAvAccTime;	/* In nano-seconds, they are average time */
	double	WriteAvAccTime;
	double	TotalAvAccTime;
} SimResult;

#endif