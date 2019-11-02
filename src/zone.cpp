/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2010-2014 QuakeSpasm developers
Copyright (C) 2019-.... FAETHER / Etherr
*/

#include "startup.h"
#include "zone.h"
#include <cstddef>
#include <csetjmp>
#include "flog.h"

/*
==============================================================================

					MEMORY/STRING MANIPULATION

==============================================================================
*/

unsigned char* stack_mem = nullptr;

//This is a hook to c++ style allocations.
//VEther will filter it's c++ libraries like glsl compiler
//thorough this memory zone instead.
void* operator new(size_t size)
{
  return zone::Z_TagMalloc(size, 1);
}

void operator delete(void* p)
{
  zone::Z_Free(p);
  return;
}

void operator delete(void* p, size_t size)
{
  zone::Z_Free(p);
  // Here this will keep the memory alive but does not is free.
  // I question size parameter.
  // "If present, the std::size_t size argument must equal the
  // size argument passed to the allocation function that returned ptr."
  // Concluded to ignore size alltogether. 
  return;
}


namespace zone
{

size_t q_strlcpy (char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0)
	{
		while (--n != 0)
		{
			if ((*d++ = *s++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0)
	{
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1);	/* count does not include NUL */
}

void Q_memset(void *dest, int fill, size_t count)
{
	size_t		i;

	if ( (((size_t)dest | count) & 3) == 0)
	{
		count >>= 2;
		fill = fill | (fill<<8) | (fill<<16) | (fill<<24);
		for (i = 0; i < count; i++)
			((int *)dest)[i] = fill;
	}
	else
		for (i = 0; i < count; i++)
			((unsigned char *)dest)[i] = fill;
}

void Q_memcpy(void *dest, const void *src, size_t count)
{
	size_t		i;

	if (( ( (size_t)dest | (size_t)src | count) & 3) == 0 )
	{
		count >>= 2;
		for (i = 0; i < count; i++)
			((int *)dest)[i] = ((int *)src)[i];
	}
	else
		for (i = 0; i < count; i++)
			((unsigned char *)dest)[i] = ((unsigned char *)src)[i];
}

int Q_memcmp(const void *m1, const void *m2, size_t count)
{
	while(count)
	{
		count--;
		if (((unsigned char *)m1)[count] != ((unsigned char *)m2)[count])
			return -1;
	}
	return 0;
}

void Q_strcpy(char *dest, const char *src)
{
	while (*src)
	{
		*dest++ = *src++;
	}
	*dest++ = 0;
}

void Q_strncpy (char *dest, const char *src, int count)
{
	while (*src && count--)
	{
		*dest++ = *src++;
	}
	if (count)
		*dest++ = 0;
}

int Q_strlen(const char *str)
{
	int		count;

	count = 0;
	while (str[count])
		count++;

	return count;
}

char *Q_strrchr(const char *s, char c)
{
	int len = Q_strlen(s);
	s += len;
	while (len--)
	{
		if (*--s == c)
			return (char *)s;
	}
	return NULL;
}

void Q_strcat(char *dest, const char *src)
{
	dest += Q_strlen(dest);
	Q_strcpy (dest, src);
}

int Q_strcmp(const char *s1, const char *s2)
{
	while (1)
	{
		if (*s1 != *s2)
			return -1;		// strings not equal
		if (!*s1)
			return 0;		// strings are equal
		s1++;
		s2++;
	}

	return -1;
}

int Q_strncmp(const char *s1, const char *s2, int count)
{
	while (1)
	{
		if (!count--)
			return 0;
		if (*s1 != *s2)
			return -1;		// strings not equal
		if (!*s1)
			return 0;		// strings are equal
		s1++;
		s2++;
	}

	return -1;
}

/*
==============================================================================

						STACK MEMORY ALLOCATION
It is important to understand that this memory allocation is expremely dangerous
and can possibly cause Undefined Behavior (UB).

It can be used on a global scope.
Last size is stored in stack_mem pointer value;
Make sure to clean up the stack before or after, if needed.

==============================================================================
*/

__attribute__((noinline)) void stack_alloc(int size, int mult)
{
	jmp_buf ret;
	int val = setjmp(ret);
	if(val == 1)
	{
		//we have to give a reason for compiler
		//to know that this is a valid return function
		//and is not UB.
		return;
	}
	if(stack_mem != nullptr)
	{
		unsigned char mem[*((int*)stack_mem)+size*mult];
		*((int*)mem) = size;
		*((int*)mem + sizeof(int)) = reinterpret_cast<uintptr_t>(stack_mem);
		stack_mem = mem;
	}
	else
	{
		unsigned char mem[size*mult];
		*((int*)mem) = size;
		stack_mem = mem;
	}
	longjmp(ret, 1);
}

void stack_clear(int size)
{
	if(stack_mem != nullptr)
	{
		Q_memset(stack_mem, 0, size);
	}
}

/*
==============================================================================

						ZONE MEMORY ALLOCATION

There is never any space between memblocks, and there will never be two
contiguous free memblocks.

The rover can be left pointing at a non-empty block

The zone calls are pretty much only used for small strings and structures,
all big things are allocated on the hunk.
==============================================================================
*/

#define	DYNAMIC_SIZE	(32 * 1024 * 1024) //4mb

#define	ZONEID	0x1d4a11
#define MINFRAGMENT	64

typedef struct memblock_s
{
	int	size;		// including the header and possibly tiny fragments
	int	tag;		// a tag of 0 is a free block
	int	id;		// should be ZONEID
	int	pad;		// pad to 64 bit boundary
	struct	memblock_s	*next, *prev;
} memblock_t;

typedef struct
{
	int		size;		// total bytes malloced, including header
	memblock_t	blocklist;	// start / end cap for linked list
	memblock_t	*rover;
} memzone_t;

void Cache_FreeLow (int new_low_hunk);
void Cache_FreeHigh (int new_high_hunk);

static memzone_t	*mainzone;

/*
========================
Z_Free
========================
*/
void Z_Free (void *ptr)
{
	memblock_t	*block, *other;

	if (!ptr)
	{
		info("Z_Free: NULL pointer");
		return;
	}
	block = (memblock_t *) ( (unsigned char *)ptr - sizeof(memblock_t));
	if (block->id != ZONEID)
	        warn("Z_Free: freed a pointer without ZONEID");
	if (block->tag == 0)
		warn("Z_Free: freed a freed pointer");

	block->tag = 0;		// mark as free

	other = block->prev;
	if (!other->tag)
	{
		// merge with previous free block
		other->size += block->size;
		other->next = block->next;
		other->next->prev = other;
		if (block == mainzone->rover)
			mainzone->rover = other;
		block = other;
	}

	other = block->next;
	if (!other->tag)
	{
		// merge the next free block onto the end
		block->size += other->size;
		block->next = other->next;
		block->next->prev = block;
		if (other == mainzone->rover)
			mainzone->rover = block;
	}
}


void *Z_TagMalloc (int size, int tag)
{
	int		extra;
	memblock_t	*start, *rover, *newblock, *base;

	if (!tag)
		warn("Z_TagMalloc: tried to use a 0 tag");

//
// scan through the block list looking for the first free block
// of sufficient size
//
	size += sizeof(memblock_t);	// account for size of block header
	size += 4;					// space for memory trash tester
	size = (size + 7) & ~7;		// align to 8-unsigned char boundary

	base = rover = mainzone->rover;
	start = base->prev;

	do
	{
		if (rover == start)	// scaned all the way around the list
			return NULL;
		if (rover->tag)
			base = rover = rover->next;
		else
			rover = rover->next;
	}
	while (base->tag || base->size < size);

//
// found a block big enough
//
	extra = base->size - size;
	if (extra >  MINFRAGMENT)
	{
		// there will be a free fragment after the allocated block
		newblock = (memblock_t *) ((unsigned char *)base + size );
		newblock->size = extra;
		newblock->tag = 0;			// free block
		newblock->prev = base;
		newblock->id = ZONEID;
		newblock->next = base->next;
		newblock->next->prev = newblock;
		base->next = newblock;
		base->size = size;
	}

	base->tag = tag;				// no longer a free block

	mainzone->rover = base->next;	// next allocation will start looking here

	base->id = ZONEID;

// marker for memory trash testing
	*(int *)((unsigned char *)base + base->size - 4) = ZONEID;

	return (void *) ((unsigned char *)base + sizeof(memblock_t));
}

/*
========================
Z_CheckHeap
========================
*/
static void Z_CheckHeap (void)
{
	memblock_t	*block;

	for (block = mainzone->blocklist.next ; ; block = block->next)
	{
		if (block->next == &mainzone->blocklist)
			break;			// all blocks have been hit
		if ( (unsigned char *)block + block->size != (unsigned char *)block->next)
			printf ("Z_CheckHeap: block size does not touch the next block\n");
		if ( block->next->prev != block)
			printf ("Z_CheckHeap: next block doesn't have proper back link\n");
		if (!block->tag && !block->next->tag)
			printf ("Z_CheckHeap: two consecutive free blocks\n");
	}
}


/*
========================
Z_Malloc
========================
*/
void *Z_Malloc (int size)
{
	void	*buf;

	//Z_CheckHeap ();
	
	buf = Z_TagMalloc (size, 1);
	if (!buf)
		printf ("Z_Malloc: failed on allocation of %i bytes",size);
	Q_memset (buf, 0, size);

	return buf;
}

/*
========================
Z_Realloc
========================
*/
void *Z_Realloc(void *ptr, int size)
{
	int old_size;
	void *old_ptr;
	memblock_t *block;

	if (!ptr)
		return Z_Malloc (size);

	block = (memblock_t *) ((unsigned char *) ptr - sizeof (memblock_t));
	if (block->id != ZONEID)
		printf ("Z_Realloc: realloced a pointer without ZONEID");
	if (block->tag == 0)
		printf ("Z_Realloc: realloced a freed pointer");

	old_size = block->size;
	old_size -= (4 + (int)sizeof(memblock_t));	/* see Z_TagMalloc() */
	old_ptr = ptr;

	Z_Free (ptr);
	ptr = Z_TagMalloc (size, 1);
	if (!ptr)
		printf ("Z_Realloc: failed on allocation of %i bytes", size);

	if (ptr != old_ptr)
		memmove (ptr, old_ptr, q_min(old_size, size));
	if (old_size < size)
		memset ((unsigned char *)ptr + old_size, 0, size - old_size);

	return ptr;
}

char *Z_Strdup (const char *s)
{
	size_t sz = strlen(s) + 1;
	char *ptr = (char *) Z_Malloc (sz);
	memcpy (ptr, s, sz);
	return ptr;
}


/*
========================
Z_Print
========================
*/
void Z_Print (memzone_t *zone)
{
	memblock_t	*block;

	printf("zone size: %i  location: %p\n",mainzone->size,mainzone);

	for (block = zone->blocklist.next ; ; block = block->next)
	{
		printf("block:%p    size:%7i    tag:%3i\n", block, block->size, block->tag);

		if (block->next == &zone->blocklist)
			break;			// all blocks have been hit
		if ( (unsigned char *)block + block->size != (unsigned char *)block->next)
			printf ("ERROR: block size does not touch the next block\n");
		if ( block->next->prev != block)
			printf ("ERROR: next block doesn't have proper back link\n");
		if (!block->tag && !block->next->tag)
			printf ("ERROR: two consecutive free blocks\n");
	}
}


//============================================================================

#define	HUNK_SENTINAL	0x1df001ed

#define HUNKNAME_LEN	24
typedef struct
{
	int		sentinal;
	int		size;		// including sizeof(hunk_t), -1 = not allocated
	char	name[HUNKNAME_LEN];
} hunk_t;

unsigned char	*hunk_base;
int		hunk_size;

int		hunk_low_used;
int		hunk_high_used;

bool	hunk_tempactive;
int		hunk_tempmark;

/*
==============
Hunk_Check

Run consistancy and sentinal trahing checks
==============
*/
void Hunk_Check (void)
{
	hunk_t	*h;

	for (h = (hunk_t *)hunk_base ; (unsigned char *)h != hunk_base + hunk_low_used ; )
	{
		if (h->sentinal != HUNK_SENTINAL)
			printf ("Hunk_Check: trahsed sentinal");
		if (h->size < (int) sizeof(hunk_t) || h->size + (unsigned char *)h - hunk_base > hunk_size)
			printf ("Hunk_Check: bad size");
		h = (hunk_t *)((unsigned char *)h+h->size);
	}
}


/*
===================
Hunk_AllocName
===================
*/
void *Hunk_AllocName (int size, const char *name)
{
	hunk_t	*h;

#ifdef DEBUG
	Hunk_Check ();
#endif

	if (size < 0)
	  {
	        fatal("Hunk_Alloc: bad size: %i", size);
		startup::debug_pause();
	  }

	size = sizeof(hunk_t) + ((size+15)&~15);

	if (hunk_size - hunk_low_used - hunk_high_used < size)
	  {
		fatal("Hunk_Alloc: failed on %i bytes",size);
		startup::debug_pause();
	  }
	h = (hunk_t *)(hunk_base + hunk_low_used);
	hunk_low_used += size;

	Cache_FreeLow(hunk_low_used);

	memset (h, 0, size);

	h->size = size;
	h->sentinal = HUNK_SENTINAL;
	q_strlcpy (h->name, name, HUNKNAME_LEN);

	return (void *)(h+1);
}

/*
===================
Hunk_Alloc
===================
*/
void *Hunk_Alloc (int size)
{
	return Hunk_AllocName (size, "unknown");
}

int	Hunk_LowMark (void)
{
	return hunk_low_used;
}

void Hunk_FreeToLowMark (int mark)
{
	if (mark < 0 || mark > hunk_low_used)
		printf ("Hunk_FreeToLowMark: bad mark %i", mark);
	memset (hunk_base + mark, 0, hunk_low_used - mark);
	hunk_low_used = mark;
}

void Hunk_FreeToHighMark (int mark)
{
	if (hunk_tempactive)
	{
		hunk_tempactive = false;
		Hunk_FreeToHighMark (hunk_tempmark);
	}
	if (mark < 0 || mark > hunk_high_used)
		printf ("Hunk_FreeToHighMark: bad mark %i", mark);
	memset (hunk_base + hunk_size - hunk_high_used, 0, hunk_high_used - mark);
	hunk_high_used = mark;
}

int	Hunk_HighMark (void)
{
	if (hunk_tempactive)
	{
		hunk_tempactive = false;
		Hunk_FreeToHighMark (hunk_tempmark);
	}

	return hunk_high_used;
}

/*
===================
Hunk_HighAllocName
===================
*/
void *Hunk_HighAllocName (int size, const char *name)
{
	hunk_t	*h;

	if (size < 0)
		printf("Hunk_HighAllocName: bad size: %i", size);

	if (hunk_tempactive)
	{
		Hunk_FreeToHighMark (hunk_tempmark);
		hunk_tempactive = false;
	}

#ifdef PARANOID
	Hunk_Check ();
#endif

	size = sizeof(hunk_t) + ((size+15)&~15);

	if (hunk_size - hunk_low_used - hunk_high_used < size)
	{
		printf("Hunk_HighAlloc: failed on %i bytes\n",size);
		return NULL;
	}

	hunk_high_used += size;
	Cache_FreeHigh (hunk_high_used);

	h = (hunk_t *)(hunk_base + hunk_size - hunk_high_used);

	memset (h, 0, size);
	h->size = size;
	h->sentinal = HUNK_SENTINAL;
	q_strlcpy (h->name, name, HUNKNAME_LEN);

	return (void *)(h+1);
}


/*
=================
Hunk_TempAlloc

Return space from the top of the hunk
=================
*/
void *Hunk_TempAlloc (int size)
{
	void	*buf;

	size = (size+15)&~15;

	if (hunk_tempactive)
	{
		Hunk_FreeToHighMark (hunk_tempmark);
		hunk_tempactive = false;
	}

	hunk_tempmark = Hunk_HighMark ();

	buf = Hunk_HighAllocName (size, "temp");

	hunk_tempactive = true;

	return buf;
}

char *Hunk_Strdup (const char *s, const char *name)
{
	size_t sz = strlen(s) + 1;
	char *ptr = (char *) Hunk_AllocName (sz, name);
	memcpy (ptr, s, sz);
	return ptr;
}

/*
===============================================================================

CACHE MEMORY

===============================================================================
*/

#define CACHENAME_LEN	32
typedef struct cache_system_s
{
	int			size;		// including this header
	cache_user_t		*user;
	char			name[CACHENAME_LEN];
	struct cache_system_s	*prev, *next;
	struct cache_system_s	*lru_prev, *lru_next;	// for LRU flushing
} cache_system_t;

cache_system_t *Cache_TryAlloc (int size, bool nobottom);

cache_system_t	cache_head;

/*
===========
Cache_Move
===========
*/
void Cache_Move ( cache_system_t *c)
{
	cache_system_t		*new_cs;

// we are clearing up space at the bottom, so only allocate it late
	new_cs = Cache_TryAlloc (c->size, true);
	if (new_cs)
	{
		printf("cache_move ok\n");

		Q_memcpy ( new_cs+1, c+1, c->size - sizeof(cache_system_t) );
		new_cs->user = c->user;
		Q_memcpy (new_cs->name, c->name, sizeof(new_cs->name));
		Cache_Free (c->user, false); //johnfitz -- added second argument
		new_cs->user->data = (void *)(new_cs+1);
	}
	else
	{
		printf ("cache_move failed\n");

		Cache_Free (c->user, true); // tough luck... //johnfitz -- added second argument
	}
}

/*
============
Cache_FreeLow

Throw things out until the hunk can be expanded to the given point
============
*/
void Cache_FreeLow (int new_low_hunk)
{
	cache_system_t	*c;

	while (1)
	{
		c = cache_head.next;
		if (c == &cache_head)
			return;		// nothing in cache at all
		if ((unsigned char *)c >= hunk_base + new_low_hunk)
			return;		// there is space to grow the hunk
		Cache_Move ( c );	// reclaim the space
	}
}

/*
============
Cache_FreeHigh

Throw things out until the hunk can be expanded to the given point
============
*/
void Cache_FreeHigh (int new_high_hunk)
{
	cache_system_t	*c, *prev;

	prev = NULL;
	while (1)
	{
		c = cache_head.prev;
		if (c == &cache_head)
			return;		// nothing in cache at all
		if ( (unsigned char *)c + c->size <= hunk_base + hunk_size - new_high_hunk)
			return;		// there is space to grow the hunk
		if (c == prev)
			Cache_Free (c->user, true);	// didn't move out of the way //johnfitz -- added second argument
		else
		{
			Cache_Move (c);	// try to move it
			prev = c;
		}
	}
}

void Cache_UnlinkLRU (cache_system_t *cs)
{
	if (!cs->lru_next || !cs->lru_prev)
		printf ("Cache_UnlinkLRU: NULL link");

	cs->lru_next->lru_prev = cs->lru_prev;
	cs->lru_prev->lru_next = cs->lru_next;

	cs->lru_prev = cs->lru_next = NULL;
}

void Cache_MakeLRU (cache_system_t *cs)
{
	if (cs->lru_next || cs->lru_prev)
		printf ("Cache_MakeLRU: active link");

	cache_head.lru_next->lru_prev = cs;
	cs->lru_next = cache_head.lru_next;
	cs->lru_prev = &cache_head;
	cache_head.lru_next = cs;
}

/*
============
Cache_TryAlloc

Looks for a free block of memory between the high and low hunk marks
Size should already include the header and padding
============
*/
cache_system_t *Cache_TryAlloc (int size, bool nobottom)
{
	cache_system_t	*cs, *new_cs;

// is the cache completely empty?

	if (!nobottom && cache_head.prev == &cache_head)
	{
		if (hunk_size - hunk_high_used - hunk_low_used < size)
			printf ("Cache_TryAlloc: %i is greater then free hunk", size);

		new_cs = (cache_system_t *) (hunk_base + hunk_low_used);
		memset (new_cs, 0, sizeof(*new_cs));
		new_cs->size = size;

		cache_head.prev = cache_head.next = new_cs;
		new_cs->prev = new_cs->next = &cache_head;

		Cache_MakeLRU (new_cs);
		return new_cs;
	}

// search from the bottom up for space

	new_cs = (cache_system_t *) (hunk_base + hunk_low_used);
	cs = cache_head.next;

	do
	{
		if (!nobottom || cs != cache_head.next)
		{
			if ( (unsigned char *)cs - (unsigned char *)new_cs >= size)
			{
				// found space
				memset (new_cs, 0, sizeof(*new_cs));
				new_cs->size = size;

				new_cs->next = cs;
				new_cs->prev = cs->prev;
				cs->prev->next = new_cs;
				cs->prev = new_cs;

				Cache_MakeLRU (new_cs);

				return new_cs;
			}
		}

		// continue looking
		new_cs = (cache_system_t *)((unsigned char *)cs + cs->size);
		cs = cs->next;

	}
	while (cs != &cache_head);

// try to allocate one at the very end
	if ( hunk_base + hunk_size - hunk_high_used - (unsigned char *)new_cs >= size)
	{
		memset (new_cs, 0, sizeof(*new_cs));
		new_cs->size = size;

		new_cs->next = &cache_head;
		new_cs->prev = cache_head.prev;
		cache_head.prev->next = new_cs;
		cache_head.prev = new_cs;

		Cache_MakeLRU (new_cs);

		return new_cs;
	}

	return NULL;		// couldn't allocate
}

/*
============
Cache_Flush

Throw everything out, so new data will be demand cached
============
*/
void Cache_Flush (void)
{
	while (cache_head.next != &cache_head)
		Cache_Free ( cache_head.next->user, true); // reclaim the space //johnfitz -- added second argument
}

/*
============
Cache_Print

============
*/
void Cache_Print (void)
{
	cache_system_t	*cd;

	for (cd = cache_head.next ; cd != &cache_head ; cd = cd->next)
	{
		printf("%8i : %s\n", cd->size, cd->name);
	}
}

/*
============
Cache_Report

============
*/
void Cache_Report (void)
{
	printf("%4.1f megabyte data cache\n", (hunk_size - hunk_high_used - hunk_low_used) / (float)(1024*1024) );
}

/*
============
Cache_Init

============
*/
void Cache_Init (void)
{
	cache_head.next = cache_head.prev = &cache_head;
	cache_head.lru_next = cache_head.lru_prev = &cache_head;

	//Cmd_AddCommand ("flush", Cache_Flush);
}

/*
==============
Cache_Free

Frees the memory and removes it from the LRU list
==============
*/
void Cache_Free (cache_user_t *c, bool freetextures) //johnfitz -- added second argument
{
	cache_system_t	*cs;

	if (!c->data)
		printf ("Cache_Free: not allocated");

	cs = ((cache_system_t *)c->data) - 1;

	cs->prev->next = cs->next;
	cs->next->prev = cs->prev;
	cs->next = cs->prev = NULL;

	c->data = NULL;

	Cache_UnlinkLRU (cs);
}



/*
==============
Cache_Check
==============
*/
void *Cache_Check (cache_user_t *c)
{
	cache_system_t	*cs;

	if (!c->data)
		return NULL;

	cs = ((cache_system_t *)c->data) - 1;

// move to head of LRU
	Cache_UnlinkLRU (cs);
	Cache_MakeLRU (cs);

	return c->data;
}


/*
==============
Cache_Alloc
==============
*/
void *Cache_Alloc (cache_user_t *c, int size, const char *name)
{
	cache_system_t	*cs;

	if (c->data)
		printf ("Cache_Alloc: allready allocated");

	if (size <= 0)
		printf ("Cache_Alloc: size %i", size);

	size = (size + sizeof(cache_system_t) + 15) & ~15;

// find memory for it
	while (1)
	{
		cs = Cache_TryAlloc (size, false);
		if (cs)
		{
			q_strlcpy (cs->name, name, CACHENAME_LEN);
			c->data = (void *)(cs+1);
			cs->user = c;
			break;
		}

		// free the least recently used candedat
		if (cache_head.lru_prev == &cache_head)
			printf ("Cache_Alloc: out of memory"); // not enough memory at all

		Cache_Free (cache_head.lru_prev->user, true); //johnfitz -- added second argument
	}

	return Cache_Check (c);
}

//============================================================================


static void Memory_InitZone (memzone_t *zone, int size)
{
	memblock_t	*block;

// set the entire zone to one free block

	zone->blocklist.next = zone->blocklist.prev = block =
	                           (memblock_t *)( (unsigned char *)zone + sizeof(memzone_t) );
	zone->blocklist.tag = 1;	// in use block
	zone->blocklist.id = 0;
	zone->blocklist.size = 0;
	zone->rover = block;

	block->prev = block->next = &zone->blocklist;
	block->tag = 0;			// free block
	block->id = ZONEID;
	block->size = size - sizeof(memzone_t);
}

/*
========================
Memory_Init
========================
*/
void Memory_Init (void *buf, int size)
{
	int zonesize = DYNAMIC_SIZE;

	hunk_base = (unsigned char *) buf;
	hunk_size = size;
	hunk_low_used = 0;
	hunk_high_used = 0;

	Cache_Init();
	mainzone = (memzone_t *) Hunk_AllocName (zonesize, "zone" );
	Memory_InitZone (mainzone, zonesize);
}

} //namespace zone