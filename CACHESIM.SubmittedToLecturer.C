/*
 * In this program's CacheSim( ), 'WRITE ALLOCATION' is implemented in this
 * way: bring in the line and update the byte in cache, then mark the line in
 * cache as 'DIRTY'. This approach will also affect the implement of 'WRITE
 * THROUGH', since here it needs to take account of whether it is 'DIRTY'.
 * Actually, the way to realise the 'WRITE THROUGH' I took below is not very
 * efficient.
 *                                                          XuZF 2003.4.18
 */
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<malloc.h>

#include	"cachesim.h"


char	ErrMessage[ 150] = "Err: An error has been catched!";


int	main( int argc, char ** argv);
void	ParaErr( void);
short	CacheSim( Cache * pCache, SimResult * pSR);
U_SHORT	GetBits( U_LONG value);
short	GetInstruct( U_SHORT * code, U_LONG * MemAdd);
U_LONG	GetValueInBits( U_LONG value, U_SHORT LeastBit, U_SHORT MostBit);
long	CheckAddInCache( CacheLine * CacheEntry, U_LONG UsedLines, U_LONG tag);
U_LONG	CacheAdd( Cache * pCache, U_LONG LineNum, U_LONG tag,
	U_LONG * TransBytes, U_LONG * AccTime);
short	RenewLRUList( Cache * pCache, U_LONG LineNum, U_LONG
	PositionInCacheEntry);
U_LONG  CheckPower2( U_LONG value);


int	main( int argc, char ** argv)
{
	Cache	CacheL1;
	SimResult	result;

	/*
	 * Check whether the parameters passed from the command line
	 * are valid or not
	 */
	if (argc != 8)
	{
		ParaErr( );
		goto ERR_EXIT;
	}

	
	CacheL1.SizeOfCache   = atol( argv[ 1]) * ONE_K;
	CacheL1.SizeOfLine    = atol( argv[ 2]);	/* XuZF - unsigned? */
	CacheL1.associativity = atol( argv[ 3]);
	CacheL1.NumOfEntries  = CacheL1.SizeOfCache / \
		(CacheL1.associativity * CacheL1.SizeOfLine);
	CacheL1.WriteHit      = * argv[ 4];
	CacheL1.WriteMiss     = * argv[ 5];
	CacheL1.CacheLatency  = atoi( argv[ 6]);
	CacheL1.MemoryLatency = atoi( argv[ 7]);

    if (CheckPower2( CacheL1.SizeOfCache / (CacheL1.associativity *
		CacheL1.SizeOfLine)))
    {
        printf( "Err: The number of entries in the built cache must be a power of 2\n");
        goto ERR_EXIT;
    }


	if (CacheSim( & CacheL1, & result))
		goto ERR_EXIT;

	printf( "# Cache size: %lu KB\n", CacheL1.SizeOfCache / ONE_K);
	printf( "# Line size: %lu Bytes\n", CacheL1.SizeOfLine);
	printf( "# Associativity: %lu\n", CacheL1.associativity);
	printf( "# Number of entries in the cache: %lu\n", CacheL1.NumOfEntries);
	printf( "# Write Hit Policy: %s\n", CacheL1.WriteHit == WRITE_BACK ?
		"WRITE BACK" : "WRITE THROUGH");
	printf( "# Write Miss Policy: %s\n", CacheL1.WriteMiss == WRITE_ALLOCATE ?
		"WRITE ALLOCATE" : "WRITE NO ALLOCATE");
	printf( "# Cache latency: %hu ns\n", CacheL1.CacheLatency);
	printf( "# Memory latency: %hu ns\n", CacheL1.MemoryLatency);

	printf( "Type\tAccesses\tMisses\t%%\tTransfer\tBytes\t\tAverage time(ns)\n");
	printf( "READ:\t%lu\t\t%lu\t%.3f\t%lu\t\t%lu\t\t%.3f\n", result.ReadAccess,
		result.ReadMiss, result.ReadRatio, result.ReadTransfer,
		result.ReadBytes, result.ReadAvAccTime);
	printf( "WRITE:\t%lu\t\t%lu\t%.3f\t%lu\t\t%lu\t\t%.3f\n",
		result.WriteAccess, result.WriteMiss, result.WriteRatio,
		result.WriteTransfer, result.WriteBytes, result.WriteAvAccTime);
	printf( "TOTAL:\t%lu\t\t%lu\t%.3f\t%lu\t\t%lu\t\t%.3f\n",
		result.TotalAccess, result.TotalMiss, result.TotalRatio,
		result.TotalTransfer, result.TotalBytes, result.TotalAvAccTime);

	return 0;

ERR_EXIT:
	printf( "%s\n", ErrMessage);
	exit( 1);
}

void	ParaErr( void)
{
	printf( "Err: The parameters typed are wrong\n"
		"USAGE:\n"
		"\tcachesim C L A WHP WMP CL ML\n\n"
		"For example:\n"
		"\tcachesim 16 32 1 B A 5 200\n"
	);
}

/*
 * FUNCTION:
 *   This function is to simulate the level 1 cache in CPU
 * RETURN:
 *   0  - OK
 *   -1 - something wrong happened, and the ErrMessage will be set
 *        with the failing reason
 * PARAMETERS:
 *   pCache - pointing to a struct 'Cache'
 *   pSR    - pointing to a struct 'SimResult', which contains the
 *            result we need after this call returns
 */
short	CacheSim( Cache * pCache, SimResult * pSR)
{
	/* The total number of lines in the cache */
	U_LONG	NumOfLines = pCache->NumOfEntries * pCache->associativity;

	U_SHORT	BitsOfByte;	/* Num of bits in an address to represent data(bytes)
						 * num in a line */
	U_SHORT	BitsOfLine;	/* Num of bits in an address to represent line num */
	U_SHORT	BitsOfTag;	/* Num of bits in an address to represent tag num */
	/* For example, a mem add: TTTTTTTTTTTTTTTTTTLLLLLLLLLBBBBB */

	/* The following 2 are used to store the value extracted from a mem add */
	U_LONG	LineNum;
	U_LONG	tag;
	
	U_SHORT	code;
	U_LONG	MemAdd;

	U_LONG	TransBytes;	/* The transferred bytes between cache and mem for every
						 * time running one instruction */
	U_LONG	AccTime;	/* The access time to cache and mem for every time
						 * running one instruction */
	long	CheckCacheFlag, ll;

	BitsOfByte = GetBits( pCache->SizeOfLine) - 1;	/* XuZF: Because, for
					* example 512 is '1000000000' 10 bits in binary code. But
					* just needs 9 bits to represent, 0 ~ 511
					*/
	BitsOfLine = GetBits( pCache->NumOfEntries) - 1;
	BitsOfTag  = MEM_ADD_WIDTH - BitsOfLine - BitsOfByte;
#ifdef DEBUG
	printf( "CacheSim( ) - NumOfEntries: %lu  NumOfLines: %lu\n", pCache->NumOfEntries, NumOfLines);
	printf( "CacheSim( ) - BitsOfByte: %hu  BitsOfLine: %hu  "
		"BitsOfTag: %hu\n\n", BitsOfByte, BitsOfLine, BitsOfTag);
	fflush( stdout);
#endif

	/* Build the simulating cache in the memory */
	if (! (pCache->CacheTable = (CacheLine *)calloc( NumOfLines, \
		sizeof( CacheLine))))
	{
		strcpy( ErrMessage, "Err: allocation for CacheTable failure");
		goto ERR_RET;
	}
	if (! (pCache->LRUList = (U_LONG *)calloc( NumOfLines, \
		sizeof( U_LONG))))
	{
		strcpy( ErrMessage, "Err: allocation for LRUList failure");
		goto ERR_RET;
	}
	if (! (pCache->LRUIndicList = (U_LONG *)calloc( \
		pCache->NumOfEntries, sizeof( U_LONG))))
	{
		strcpy( ErrMessage, "Err: allocation for LRUIndicList failure");
		goto ERR_RET;
	}

	/* Initialize CacheTable, LRUList & LRUIndicList */
	for (ll = 0; ll < NumOfLines; ll++)
	{
		pCache->CacheTable[ ll].valid = INVALID;
		pCache->CacheTable[ ll].dirty = CLEAN;
	}
	for (ll = 0; ll < NumOfLines; ll++)
	{
		/*
		 * Because the index of every line in a same entry is between
		 * 0 ~ associativity-1
		 */
		pCache->LRUList[ ll] = pCache->associativity;
	}
	for (ll = 0; ll < pCache->NumOfEntries; ll++)
	{
		/* At the beginning, no line in every entry has been used */
		pCache->LRUIndicList[ ll] = 0;
	}

	/* Initialize the struct 'SimResult' that is pointed by pSR */
	pSR->ReadAccess   = pSR->WriteAccess   = pSR->TotalAccess   = 0;
	pSR->ReadMiss     = pSR->WriteMiss     = pSR->TotalMiss     = 0;
	pSR->ReadRatio    = pSR->WriteRatio    = pSR->TotalRatio    = 0.0;
	pSR->ReadTransfer = pSR->WriteTransfer = pSR->TotalTransfer = 0;
	pSR->ReadBytes    = pSR->WriteBytes    = pSR->TotalBytes    = 0;
	pSR->ReadAccTime  = pSR->WriteAccTime  = pSR->TotalAccTime  = 0;
	pSR->ReadAvAccTime = pSR->WriteAvAccTime = pSR->TotalAvAccTime = 0.0;

	while (GetInstruct( & code, & MemAdd))
	{
#ifdef DEBUG
		printf( "CacheSim( ) - While ( ) LOOP beginning ...\n");
#endif
		TransBytes = 0;
		AccTime    = 0;

		LineNum = GetValueInBits( MemAdd, BitsOfByte, BitsOfByte +
			BitsOfLine - 1);
		tag     = GetValueInBits( MemAdd, BitsOfByte + BitsOfLine,
			MEM_ADD_WIDTH - 1);
#ifdef DEBUG
		printf( "CacheSim( ) - code: %hu  MemAdd: %lx\n", code, MemAdd);
		printf( "CacheSim( ) - LineNum: %lu  tag: %lu\n", LineNum, tag);
		fflush( stdout);
#endif
		CheckCacheFlag = CheckAddInCache( pCache->CacheTable + LineNum *
			pCache->associativity, pCache->LRUIndicList[ LineNum], tag);
		AccTime += pCache->CacheLatency;	/* XuZF: !!! */

		switch (code)
		{
			/* Read */
			case 0:
			case 5:
				pSR->ReadAccess++;

				if (CheckCacheFlag == -1)
				{
#ifdef DEBUG
					printf( "CacheSim( ) - READ miss\n");
					fflush( stdout);
#endif
					/* If miss in the cache */

					pSR->ReadMiss++;
					CacheAdd( pCache, LineNum, tag, & TransBytes, & AccTime);
				}
				else
				{
#ifdef DEBUG
					printf( "CacheSim( ) - READ hit\n");
					fflush( stdout);
#endif
					/* If hit in the cache */

					/* Renew the LRU list entry */
					if (RenewLRUList( pCache, LineNum, CheckCacheFlag))
					{
						strcpy( ErrMessage,
							"ERR - Error happened in RenewLRUList( ) for READ Code");
						goto ERR_RET;
					}
				}
				pSR->ReadBytes += TransBytes;
				pSR->ReadAccTime += AccTime;

				break;
			/* Write */
			case 1:
			case 6:
				pSR->WriteAccess++;

				if (CheckCacheFlag == -1)
				{
#ifdef DEBUG
					printf( "CacheSim( ) - WRITE miss\n");
					fflush( stdout);
#endif
					/* If miss in the cache */

					pSR->WriteMiss++;

					if (pCache->WriteMiss == WRITE_ALLOCATE)
					{
						/*
						 * Bring the old line including the mem add in the
						 * memory to cache, and update the byte in the cache,
						 * then mark the line as 'DIRTY'
						 */
						ll = CacheAdd( pCache, LineNum, tag, & TransBytes,
							& AccTime);
						(pCache->CacheTable + LineNum * pCache->associativity +
							ll)->dirty = DIRTY;
					}
					else
					{
						/* Write the word into the momery straightforward */
						TransBytes += WORD;
						AccTime += pCache->MemoryLatency;     /* XuZF: !!! */
					}
				}
				else
				{
#ifdef DEBUG
					printf( "CacheSim( ) - WRITE hit\n");
					fflush( stdout);
#endif
					/* If hit in the cache */

					if (pCache->WriteHit == WRITE_BACK)
					{
						/*
						 * Only update the byte in cache, and mark the line as
						 * 'DIRTY', and renew the LRUList
						 */
						(pCache->CacheTable + LineNum * pCache->associativity +
							CheckCacheFlag)->dirty = DIRTY;
					}
					else
					{
						/*
						 * Update the byte in cache, and don't mark the line as
						 * 'CLEAN', just leave it alone, because the line might
						 * be marked as 'DIRTY' by a prevous 'write miss'
						 * operation under 'WRITE ALLOCATION' conditions, so
						 * the whole line needs to be write back to memory when
						 * be evicted. Then write the word which includes the
						 * byte to the memory, and renew the LRUList
						 */
						/*
						(pCache->CacheTable + LineNum * pCache->associativity +
							CheckCacheFlag)->dirty = CLEAN;
						 */
						TransBytes += WORD;
						AccTime += pCache->MemoryLatency;     /* XuZF: !!! */
					}
					if (RenewLRUList( pCache, LineNum, CheckCacheFlag))
					{
						strcpy( ErrMessage,
							"ERR - Error happened in RenewLRUList( ) for a WRITE Code");
						goto ERR_RET;
					}
				}
				pSR->WriteBytes += TransBytes;
				pSR->WriteAccTime += AccTime;

				break;
			default:
				NULL;
		}
#ifdef DEBUG
		printf( "CacheSim( ) - TransBytes: %lu  AccTime: %lu\n", TransBytes, AccTime);
		fflush( stdout);
#endif
	}

	pSR->TotalAccess = pSR->ReadAccess + pSR->WriteAccess;
	pSR->TotalMiss = pSR->ReadMiss + pSR->WriteMiss;

	pSR->ReadRatio  = SSWR3( 100 * (double)pSR->ReadMiss /
		(double)pSR->ReadAccess);
	pSR->WriteRatio = SSWR3( 100 * (double)pSR->WriteMiss /
		(double)pSR->WriteAccess);
	pSR->TotalRatio = SSWR3( 100 * (double)pSR->TotalMiss /
		(double)pSR->TotalAccess);

	pSR->TotalBytes    = pSR->ReadBytes + pSR->WriteBytes;
	pSR->ReadTransfer  = pSR->ReadBytes / WORD;
	pSR->WriteTransfer = pSR->WriteBytes / WORD;
	pSR->TotalTransfer = pSR->TotalBytes / WORD;

	pSR->TotalAccTime = pSR->ReadAccTime + pSR->WriteAccTime;

	pSR->ReadAvAccTime  = SSWR3( (double)pSR->ReadAccTime /
		(double)pSR->ReadAccess);
	pSR->WriteAvAccTime = SSWR3( (double)pSR->WriteAccTime /
		(double)pSR->WriteAccess);
	pSR->TotalAvAccTime = SSWR3( (double)pSR->TotalAccTime /
		(double)pSR->TotalAccess);


	/* Release the memory space allocated for the cache, before end */
	free( pCache->CacheTable);
	free( pCache->LRUList);
	free( pCache->LRUIndicList);

	return 0;

ERR_RET:
	return -1;
}

/*
 * FUNCTION:
 *   Return the number of bits of the special value
 *
 * EXAMPLE:
 *   A value, '9' in decimal is '1001' in Binary format, so the function will
 *   return 4 as result.
 */
U_SHORT	GetBits( U_LONG value)
{
	U_SHORT	NumOfBits = 0;

	for (; value != 0; NumOfBits ++, value >>= 1)
		NULL;

	return NumOfBits;
}

/*
 * FUNCTION:
 *   Get the value from the specified bit range in a value
 *
 * EXAMPLE:
 *   A value, vv, in binary format: 10011011
 *                                  76543210
 *   After calling GetValueInBits( vv, 2, 4), it returns 110 in binary
 */
U_LONG	GetValueInBits( U_LONG value, U_SHORT LeastBit, U_SHORT MostBit)
{
	U_LONG	mask = 0L;
	U_LONG	one  = 1L;
	short	count;

	for (count = LeastBit; count <= MostBit; count++)
		mask |= one<<count;

	return (value & mask) >> LeastBit;
}

/*
 * FUNCTION:
 *   Read an instruction in the dinero format from stdin stream
 * RETURN:
 *   0 - if meets EOF (the end of file)
 *   1 - otherwise
 * PARAMETERS:
 *   Note: The 2 parameters are used to return value, not for passing value
 *
 *   code   - 0 (data read), 1 (data write), 2 (instruction fetch)
 *            3 (escape record, unknown access type)
 *            4 (escape record, causes cache flush)
 *            5 (heap data read, non-standard, Java traces only)
 *            6 (heap data write, non-standard, Java traces only)
 *   MemAdd - return a memory address
 *
 * EXAMPLE:
 *   The dinero format is:
 *     0 FFA0
 *
 *   After calling GetInstruct( & code, & MemAdd), code = 0, MemAdd = 0xFFA0
 */
short	GetInstruct( U_SHORT * code, U_LONG * MemAdd)
{
	/* XuZF: '%llx' is for 'long long' data type */
	return scanf( "%d%lx", code, MemAdd) == EOF ? 0 : 1;
}

/*
 * FUNCTION:
 *   Check whether or not the specified memory address in the specified entry
 *   of the cache
 * RETURN:
 *   -1  - this mem add (tag) is not in the special cache entry
 *   >=0 - having found it, and return the position in the entry
 * PARAMETERS:
 *   CacheEntry - the specified entry in the cache which will be checked
 *                through for the mem add (tag)
 *   UsedLines  - the number of used lines in a cache entry, it must equal to
 *                the entry's LRUIndicList
 *   tag - the tag of the specified memory address
 *
 * EXAMPLE:
 *   A cache entry with 4 lines, and 3 of the 4 are used:
 *   the lines     : Line 0, Line 1, Line 2, Line 3
 *   the tags      : 10,     11,     12
 *   the valid bits: VALID,  VALID,  VALID,  INVALID
 *
 *   Then:
 *   CheckAddInCache( CacheEntry, 3, 10) returns 0
 *   CheckAddInCache( CacheEntry, 3, 12) returns 2
 *   CheckAddInCache( CacheEntry, 3, 17) returns -1
 */
long	CheckAddInCache( CacheLine * CacheEntry, U_LONG UsedLines, U_LONG tag)
{
	long	ll;

	/* This statement has a bug. When the valid bit of the call next to this
	 * entry is 'VALID' the for () LOOP will never stop until meets 'INVALID'
	 * in the memory; and it seems often to happen if the cache is mostly full!
	for (ll = 0; CacheEntry->valid == VALID && CacheEntry->tag != tag;
		CacheEntry ++, ll++)
	 */
	for (ll = 0; ll < UsedLines && CacheEntry->tag != tag; CacheEntry ++, ll++)
		NULL;

	return ll == UsedLines ? -1 : ll;
}

/*
 * FUNCTION:
 *   Bring a new memory line, which the specified mem add locates in, into the
 *   cache. If the entry is full, it will evict an existing one in the cache,
 *   according to the Least Recently Used rule. Finally, renew the LRUList
 * RETURN:
 *   >=0 - return the position in the cache entry in which the new mem add was
 *     just stored
 * PARAMETERS:
 *   pCache  - a pointer to the cache
 *   LineNum - the line number of the new mem add in the cache
 *   tag     - the tag of the specified memory address
 *   TransBytes -
 *   AccTime -
 *
 *   Note: TransBytes & AccTime count the transferred bytes between cache and
 *     memory, and the access time to the cache and the memory for those
 *     operations on cache and memory exactly in this function
 *
 * EXAMPLE:
 *   A cache entry with 4 lines, and all the 4 are used:
 *     the lines     : Line 0, Line 1, Line 2, Line 3
 *     the tags      : 10,     11,     12,     13
 *     the valid bits: VALID,  VALID,  VALID,  VALID
 *
 *     its LRUList entry is: 3, 0, 2, 1 
 *     its LRUList indices :[0, 1, 2, 3]
 *
 *   Then:
 *     After CacheAdd( & Cache, LineNum, 17, & TransBytes, & AccTime), the
 *     cache entry will change to:
 *     the lines     : Line 0, Line 1, Line 2, Line 3
 *     the tags      : 10,     11,     12,     17
 *     the valid bits: VALID,  VALID,  VALID,  VALID
 *
 *     LRUList entry is: 0, 2, 1, 3
 *     LRUList indices :[0, 1, 2, 3]
 */
U_LONG	CacheAdd( Cache * pCache, U_LONG LineNum, U_LONG tag,
	U_LONG * TransBytes, U_LONG * AccTime)
{
	/* The num of memory op required to read/write a line */
	U_LONG	NumOfMemOp = pCache->SizeOfLine / WORD;

	CacheLine	* pCL = pCache->CacheTable + LineNum * pCache->associativity;
	U_LONG	* LRUListEntry = pCache->LRUList + LineNum * pCache->associativity;
	U_LONG	UsedLines      = pCache->LRUIndicList[ LineNum];
	U_LONG	position, ll, temp;


	* TransBytes += pCache->SizeOfLine;
	* AccTime    += NumOfMemOp * pCache->MemoryLatency;	/* XuZF: !!! */

	if (UsedLines == pCache->associativity)
	{
		/* The entry in the cache is full */

		temp = LRUListEntry[ 0];
		if (pCL[ temp].dirty == DIRTY)
		{
			/* Write the line of data back to memory */

			* TransBytes += pCache->SizeOfLine;
			* AccTime    += NumOfMemOp * pCache->MemoryLatency;	/* XuZF: !!! */
		}
#ifdef DEBUG
		printf( "CacheAdd( ) - Evicted tag: %lu  Evicted DIRTY Bit: %lu\n",
			pCL[ temp].tag, pCL[ temp].dirty);
		fflush( stdout);
#endif
		pCL[ temp].tag   = tag;
		pCL[ temp].valid = VALID;
		pCL[ temp].dirty = CLEAN;

		for (ll = 0; ll < UsedLines-1; ll++)
			LRUListEntry[ ll] = LRUListEntry[ ll+1];
		LRUListEntry[ UsedLines-1] = position = temp;
	}
	else
	{
		/* The entry in the cache isn't full */

		pCL[ UsedLines].tag   = tag;
		pCL[ UsedLines].valid = VALID;
		pCL[ UsedLines].dirty = CLEAN;

		LRUListEntry[ UsedLines] = UsedLines;
		pCache->LRUIndicList[ LineNum] ++;

		position = UsedLines;
	}

	* AccTime += pCache->CacheLatency;	/* XuZF: !!! */

	return position;
}

/*
 * FUNCTION:
 *   The funcion renews the Least Recently Used List, according to the given
 *   entry number (LineNum) in the LRUList and the cell in this LRUList entry,
 *   whose position is calculated from the data's position in the cache entry
 *
 *   Note: this function isn't responsible to count cache access time, so the
 *     cache time should be considered outside to calculate
 * RETURN:
 *   0 - OK
 *   1 - fail
 * PARAMETERS:
 *   pCache  - a pointer to the cache
 *   LineNum - the line number of the new mem add in the cache
 *   PositionInCacheEntry -
 *
 * EXAMPLE:
 *   A cache entry with 4 lines, and all the 4 are used:
 *     the lines     : Line 0, Line 1, Line 2, Line 3
 *     the tags      : 10,     11,     12,     13
 *     the valid bits: VALID,  VALID,  VALID,  VALID
 *
 *     its LRUList entry is: 3, 0, 2, 1 
 *     its LRUList indices :[0, 1, 2, 3]
 *
 *   Then:
 *     After RenewLRUList( & Cache, LineNum, 0), the cache entry will change to:
 *     the lines     : Line 0, Line 1, Line 2, Line 3
 *     the tags      : 10,     11,     12,     13
 *     the valid bits: VALID,  VALID,  VALID,  VALID
 *
 *     LRUList entry is: 3, 2, 1, 0
 *     LRUList indices :[0, 1, 2, 3]
 */
short	RenewLRUList( Cache * pCache, U_LONG LineNum,
	U_LONG PositionInCacheEntry)
{
	U_LONG	* LRUListEntry = pCache->LRUList + LineNum * pCache->associativity;
	U_LONG	UsedLines      = pCache->LRUIndicList[ LineNum];
	U_LONG	ll, * pLRU;

	for (ll = 0, pLRU = LRUListEntry; ll < UsedLines && * pLRU !=
		PositionInCacheEntry; ll++, pLRU++)
		NULL;
	if (ll == UsedLines)
		goto ERR_RET;

	for ( ; ll < UsedLines-1; ll++ )
		LRUListEntry[ ll] = LRUListEntry[ ll+1];
	LRUListEntry[ UsedLines-1] = PositionInCacheEntry;

	return 0;

ERR_RET:
	return 1;
}

/*
 * FUNCTION:
 *   Check whether the passed value is a power of 2. For example, 1, 2,
 *   4, 8, 16, 32, ..., 2^n ...
 * RETURN:
 *   0  - Yes, the value is a power of 2
 *   >0 - No
 */
U_LONG  CheckPower2( U_LONG value)
{
	U_SHORT NumOfBits;
	U_LONG  mask = (U_LONG)1L;

	if (value < 1)	return 1;

	NumOfBits = GetBits( value);
	mask = ~(mask << NumOfBits - 1);

	return value & mask;
}
