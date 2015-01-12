/*-------------------------------------------------------------------------
 *
 * relblock_io.h
 *	  POSTGRES relation block io utilities definitions.
 *
 *
 * src/include/access/relblock_io.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef RELBLOCK_H
#define RELBLOCK_H

#include "access/htup.h"
#include "utils/relcache.h"
#include "storage/buf.h"
#include "access/heapam.h"

#define BLOCK_FIXED_LENGTH_SIZE 100             /* In terms of number of tuples */
#define BLOCK_VARIABLE_LENGTH_SIZE 1024*32      /* Raw size in bytes (Must be < 2^16)*/

#define BLOCK_POINTER_SIZE  8                   /* 8 bytes */
#define NUM_REL_BLOCK_ENTRIES 1000              /* Entries in shared rel block table */

#define RELBLOCK_CACHELINE_SIZE   16            /* 64 bytes */

// RelationBlock storage information
typedef enum RelationBlockBackend{
	STORAGE_BACKEND_FS,
	STORAGE_BACKEND_VM,
	STORAGE_BACKEND_NVM
} RelBlockBackend;

#define STORAGE_BACKEND_DEFAULT STORAGE_BACKEND_FS

/* Possible block types */
typedef enum RelationBlockType
{
	/* Used to store fixed-length tuples */
	RELATION_FIXED_BLOCK_TYPE,
	/* Used to store variable-length attributes */
	RELATION_VARIABLE_BLOCK_TYPE
} RelBlockType;

/* RelationBlock structure */
typedef struct RelBlockData
{
	RelBlockType rb_type;
	RelBlockBackend rb_backend;
	Size rb_size;

	/* For fixed-length blocks */
	List *rb_tile_locations;

	// Keep these in sync
	bool *rb_slot_bitmap;
	int rb_free_slots;
	HeapTupleHeader *rb_tuple_headers;

	/* For variable-length blocks */
	void *rb_location;
	// Keep these in sync
	Size rb_free_space;

} RelBlockData;
typedef RelBlockData* RelBlock;

/* Tile information */
typedef struct RelTileData
{
	/* Id */
	int tile_id;
	/* Size of slot in tile */
	Size tile_size;
	/* Starting attr in tile */
	int tile_start_attr_id;

} RelTileData;
typedef RelTileData* RelTile;

typedef struct RelBlockInfoData
{
	Oid rel_id;
	Size rel_tuple_len;

	/* relation blocks on VM */
	List *rel_fixed_blocks_on_VM;
	List *rel_variable_blocks_on_VM;

	/* relation blocks on NVM */
	List *rel_fixed_blocks_on_NVM;
	List *rel_variable_blocks_on_NVM;

	/* column groups */
	int  *rel_attr_group;
	List *rel_column_groups;
} RelBlockInfoData;
typedef RelBlockInfoData* RelBlockInfo;

/* RelBlock HTAB */

/* Key for RelBlock Lookup Table */
typedef struct RelBlockTag{
	Oid       relid;
} RelBlockTag;

/* Entry for RelBlock Lookup Table */
typedef struct RelBlockLookupEnt{
	/*
	  XXX Payload required to handle a weird hash function issue in dynahash.c;
	  otherwise the keys don't collide
	*/
	int               payload;

	int               pid;
	RelBlockInfo      rel_block_info;
} RelBlockLookupEnt;

extern HTAB *Shared_Rel_Block_Hash_Table;

/* Variable-length block header */
typedef struct RelBlockVarlenHeaderData{

	/* occupied status for the slot */
	bool vb_slot_status;

	/* length of the slot */
	uint16 vb_slot_length;

	/* length of the previous slot */
	uint16 vb_prev_slot_length;

} RelBlockVarlenHeaderData;
typedef RelBlockVarlenHeaderData* RelBlockVarlenHeader;

#define RELBLOCK_VARLEN_HEADER_SIZE     8   /* 8 bytes */

typedef struct RelBlockLocation
{
	/* location of block */
	RelBlock rb_location;

	/* offset of slot within block (starts from 1) */
	OffsetNumber rb_offset;

} RelBlockLocation;

/* relblock.c */
extern void RelInitBlockTableEntry(Relation relation);
extern List** GetRelBlockList(Relation relation, RelBlockBackend relblockbackend,
							  RelBlockType relblocktype);

extern Oid  RelBlockInsertTuple(Relation relation, HeapTuple tup, CommandId cid,
								int options, BulkInsertState bistate);

/* relblock_table.c */
extern Size RelBlockTableShmemSize(Size size);
extern void InitRelBlockTable(Size size);

extern uint32 RelBlockTableHashCode(RelBlockTag *tagPtr);
extern RelBlockLookupEnt *RelBlockTableLookup(RelBlockTag *tagPtr, uint32 hashcode);
extern int	RelBlockTableInsert(RelBlockTag *tagPtr, uint32 hashcode,
								RelBlockInfo relblockinfo);
extern void RelBlockTableDelete(RelBlockTag *tagPtr, uint32 hashcode);
extern void RelBlockTablePrint();

/* relblock_fixed.c */
extern RelBlockLocation GetFixedLengthSlot(Relation relation, RelBlockBackend relblockbackend);

/* relblock_varlen.c */
extern void *GetVariableLengthSlot(Relation relation, RelBlockBackend relblockbackend,
								   Size allocation_size);
extern void  ReleaseVariableLengthSlot(Relation relation, RelBlockBackend relblockbackend,
									   void *location);

#endif   /* RELBLOCK_H */