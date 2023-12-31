/******************************************************************************/
#ifdef JEMALLOC_H_TYPES

typedef struct tcache_bin_info_s tcache_bin_info_t;
typedef struct tcache_bin_s tcache_bin_t;
typedef struct tcache_s tcache_t;
typedef struct tcaches_s tcaches_t;

/*
 * tcache pointers close to NULL are used to encode state information that is
 * used for two purposes: preventing thread caching on a per thread basis and
 * cleaning up during thread shutdown.
 */
#define	TCACHE_STATE_DISABLED		((tcache_t *)(uintptr_t)1)
#define	TCACHE_STATE_REINCARNATED	((tcache_t *)(uintptr_t)2)
#define	TCACHE_STATE_PURGATORY		((tcache_t *)(uintptr_t)3)
#define	TCACHE_STATE_MAX		TCACHE_STATE_PURGATORY

/*
 * Absolute minimum number of cache slots for each small bin.
 */
#define	TCACHE_NSLOTS_SMALL_MIN		20

/*
 * Absolute maximum number of cache slots for each small bin in the thread
 * cache.  This is an additional constraint beyond that imposed as: twice the
 * number of regions per run for this size class.
 *
 * This constant must be an even number.
 */
#define	TCACHE_NSLOTS_SMALL_MAX		200

/* Number of cache slots for large size classes. */
#define	TCACHE_NSLOTS_LARGE		20

/* (1U << opt_lg_tcache_max) is used to compute tcache_maxclass. */
#define	LG_TCACHE_MAXCLASS_DEFAULT	15

/*
 * TCACHE_GC_SWEEP is the approximate number of allocation events between
 * full GC sweeps.  Integer rounding may cause the actual number to be
 * slightly higher, since GC is performed incrementally.
 */
#define	TCACHE_GC_SWEEP			8192

/* Number of tcache allocation/deallocation events between incremental GCs. */
#define	TCACHE_GC_INCR							\
    ((TCACHE_GC_SWEEP / NBINS) + ((TCACHE_GC_SWEEP / NBINS == 0) ? 0 : 1))

#endif /* JEMALLOC_H_TYPES */
/******************************************************************************/
#ifdef JEMALLOC_H_STRUCTS

typedef enum {
	tcache_enabled_false   = 0, /* Enable cast to/from bool. */
	tcache_enabled_true    = 1,
	tcache_enabled_default = 2
} tcache_enabled_t;

/*
 * Read-only information associated with each element of tcache_t's tbins array
 * is stored separately, mainly to reduce memory usage.
 */
struct tcache_bin_info_s {
	unsigned	ncached_max;	/* Upper limit on ncached. */
};

struct tcache_bin_s {
	tcache_bin_stats_t tstats;
	int		low_water;	/* Min # cached since last GC. */
	unsigned	lg_fill_div;	/* Fill (ncached_max >> lg_fill_div). */
	unsigned	ncached;	/* # of cached objects. */
	void		**avail;	/* Stack of available objects. */
};

struct tcache_s {
	ql_elm(tcache_t) link;		/* Used for aggregating stats. */
	uint64_t	prof_accumbytes;/* Cleared after arena_prof_accum(). */
	unsigned	ev_cnt;		/* Event count since incremental GC. */
	szind_t		next_gc_bin;	/* Next bin to GC. */
	tcache_bin_t	tbins[1];	/* Dynamically sized. */
	/*
	 * The pointer stacks associated with tbins follow as a contiguous
	 * array.  During tcache initialization, the avail pointer in each
	 * element of tbins is initialized to point to the proper offset within
	 * this array.
	 */
};

/* Linkage for list of available (previously used) explicit tcache IDs. */
struct tcaches_s {
	union {
		tcache_t	*tcache;
		tcaches_t	*next;
	};
};

#endif /* JEMALLOC_H_STRUCTS */
/******************************************************************************/
#ifdef JEMALLOC_H_EXTERNS

extern bool	opt_tcache;
extern ssize_t	opt_lg_tcache_max;

extern tcache_bin_info_t	*tcache_bin_info;

/*
 * Number of tcache bins.  There are NBINS small-object bins, plus 0 or more
 * large-object bins.
 */
extern size_t	nhbins;

/* Maximum cached size class. */
extern size_t	tcache_maxclass;

/*
 * Explicit tcaches, managed via the tcache.{create,flush,destroy} mallctls and
 * usable via the MALLOCX_TCACHE() flag.  The automatic per thread tcaches are
 * completely disjoint from this data structure.  tcaches starts off as a sparse
 * array, so it has no physical memory footprint(until individual pages are
 * touched.  This allows the entire array to be allocated the first time an
 * explicit tcache is created without a disproportionate impact on memory usage.
 */
extern tcaches_t	*tcaches;

size_t	tcache_salloc(const void *ptr);
void	tcache_event_hard(tsd_t *tsd, tcache_t *tcache);
void	*tcache_alloc_small_hard(tsd_t *tsd, arena_t *arena, tcache_t *tcache,
    tcache_bin_t *tbin, szind_t binind);
void	tcache_bin_flush_small(tsd_t *tsd, tcache_t *tcache, tcache_bin_t *tbin,
    szind_t binind, unsigned rem);
void	tcache_bin_flush_large(tsd_t *tsd, tcache_bin_t *tbin, szind_t binind,
    unsigned rem, tcache_t *tcache);
void	tcache_arena_associate(tcache_t *tcache, arena_t *arena);
void	tcache_arena_reassociate(tcache_t *tcache, arena_t *oldarena,
    arena_t *newarena);
void	tcache_arena_dissociate(tcache_t *tcache, arena_t *arena);
tcache_t *tcache_get_hard(tsd_t *tsd);
tcache_t *tcache_create(tsd_t *tsd, arena_t *arena);
void	tcache_cleanup(tsd_t *tsd);
void	tcache_enabled_cleanup(tsd_t *tsd);
void	tcache_stats_merge(tcache_t *tcache, arena_t *arena);
bool	tcaches_create(tsd_t *tsd, unsigned *r_ind);
void	tcaches_flush(tsd_t *tsd, unsigned ind);
void	tcaches_destroy(tsd_t *tsd, unsigned ind);
bool	tcache_boot(void);

#endif /* JEMALLOC_H_EXTERNS */
/******************************************************************************/
#ifdef JEMALLOC_H_INLINES

#ifndef JEMALLOC_ENABLE_INLINE
void	tcache_event(tsd_t *tsd, tcache_t *tcache);
void	tcache_flush(void);
bool	tcache_enabled_get(void);
tcache_t *tcache_get(tsd_t *tsd, bool create);
void	tcache_enabled_set(bool enabled);
void	*tcache_alloc_easy(tcache_bin_t *tbin);
void	*tcache_alloc_small(tsd_t *tsd, arena_t *arena, tcache_t *tcache,
    size_t size, bool zero);
void	*tcache_alloc_large(tsd_t *tsd, arena_t *arena, tcache_t *tcache,
    size_t size, bool zero);
void	tcache_dalloc_small(tsd_t *tsd, tcache_t *tcache, void *ptr,
    szind_t binind);
void	tcache_dalloc_large(tsd_t *tsd, tcache_t *tcache, void *ptr,
    size_t size);
tcache_t	*tcaches_get(tsd_t *tsd, unsigned ind);
#endif

#if (defined(JEMALLOC_ENABLE_INLINE) || defined(JEMALLOC_TCACHE_C_))
JEMALLOC_INLINE void
tcache_flush(void)
{
	tsd_t *tsd;

	cassert(config_tcache);

	tsd = tsd_fetch();
	tcache_cleanup(tsd);
}

JEMALLOC_INLINE bool
tcache_enabled_get(void)
{
	tsd_t *tsd;
	tcache_enabled_t tcache_enabled;

	cassert(config_tcache);

	tsd = tsd_fetch();
	tcache_enabled = tsd_tcache_enabled_get(tsd);
	if (tcache_enabled == tcache_enabled_default) {
		tcache_enabled = (tcache_enabled_t)opt_tcache;
		tsd_tcache_enabled_set(tsd, tcache_enabled);
	}

	return ((bool)tcache_enabled);
}

JEMALLOC_INLINE void
tcache_enabled_set(bool enabled)
{
	tsd_t *tsd;
	tcache_enabled_t tcache_enabled;

	cassert(config_tcache);

	tsd = tsd_fetch();

	tcache_enabled = (tcache_enabled_t)enabled;
	tsd_tcache_enabled_set(tsd, tcache_enabled);

	if (!enabled)
		tcache_cleanup(tsd);
}

JEMALLOC_ALWAYS_INLINE tcache_t *
tcache_get(tsd_t *tsd, bool create)
{
	tcache_t *tcache;

	if (!config_tcache)
		return (NULL);

	tcache = tsd_tcache_get(tsd);
	if (!create)
		return (tcache);
	if (unlikely(tcache == NULL) && tsd_nominal(tsd)) {
		tcache = tcache_get_hard(tsd);
		tsd_tcache_set(tsd, tcache);
	}

	return (tcache);
}

JEMALLOC_ALWAYS_INLINE void
tcache_event(tsd_t *tsd, tcache_t *tcache)
{

	if (TCACHE_GC_INCR == 0)
		return;

	tcache->ev_cnt++;
	assert(tcache->ev_cnt <= TCACHE_GC_INCR);
	if (unlikely(tcache->ev_cnt == TCACHE_GC_INCR))
		tcache_event_hard(tsd, tcache);
}

JEMALLOC_ALWAYS_INLINE void *
tcache_alloc_easy(tcache_bin_t *tbin)
{
	void *ret;

	if (unlikely(tbin->ncached == 0)) {
		tbin->low_water = -1;
		return (NULL);
	}
	tbin->ncached--;
	if (unlikely((int)tbin->ncached < tbin->low_water))
		tbin->low_water = tbin->ncached;
	ret = tbin->avail[tbin->ncached];
	return (ret);
}

JEMALLOC_ALWAYS_INLINE void *
tcache_alloc_small(tsd_t *tsd, arena_t *arena, tcache_t *tcache, size_t size,
    bool zero)
{
	void *ret;
	szind_t binind;
	size_t usize;
	tcache_bin_t *tbin;

	binind = size2index(size);
	assert(binind < NBINS);
	tbin = &tcache->tbins[binind];
	usize = index2size(binind);
	ret = tcache_alloc_easy(tbin);
	if (unlikely(ret == NULL)) {
		ret = tcache_alloc_small_hard(tsd, arena, tcache, tbin, binind);
		if (ret == NULL)
			return (NULL);
	}
	assert(tcache_salloc(ret) == usize);

	if (likely(!zero)) {
		if (config_fill) {
			if (unlikely(opt_junk_alloc)) {
				arena_alloc_junk_small(ret,
				    &arena_bin_info[binind], false);
			} else if (unlikely(opt_zero))
				memset(ret, 0, usize);
		}
	} else {
		if (config_fill && unlikely(opt_junk_alloc)) {
			arena_alloc_junk_small(ret, &arena_bin_info[binind],
			    true);
		}
		memset(ret, 0, usize);
	}

	if (config_stats)
		tbin->tstats.nrequests++;
	if (config_prof)
		tcache->prof_accumbytes += usize;
	tcache_event(tsd, tcache);
	return (ret);
}

JEMALLOC_ALWAYS_INLINE void *
tcache_alloc_large(tsd_t *tsd, arena_t *arena, tcache_t *tcache, size_t size,
    bool zero)
{
	void *ret;
	szind_t binind;
	size_t usize;
	tcache_bin_t *tbin;

	binind = size2index(size);
	usize = index2size(binind);
	assert(usize <= tcache_maxclass);
	assert(binind < nhbins);
	tbin = &tcache->tbins[binind];
	ret = tcache_alloc_easy(tbin);
	if (unlikely(ret == NULL)) {
		/*
		 * Only allocate one large object at a time, because it's quite
		 * expensive to create one and not use it.
		 */
		ret = arena_malloc_large(arena, usize, zero);
		if (ret == NULL)
			return (NULL);
	} else {
		if (config_prof && usize == LARGE_MINCLASS) {
			arena_chunk_t *chunk =
			    (arena_chunk_t *)CHUNK_ADDR2BASE(ret);
			size_t pageind = (((uintptr_t)ret - (uintptr_t)chunk) >>
			    LG_PAGE);
			arena_mapbits_large_binind_set(chunk, pageind,
			    BININD_INVALID);
		}
		if (likely(!zero)) {
			if (config_fill) {
				if (unlikely(opt_junk_alloc))
					memset(ret, 0xa5, usize);
				else if (unlikely(opt_zero))
					memset(ret, 0, usize);
			}
		} else
			memset(ret, 0, usize);

		if (config_stats)
			tbin->tstats.nrequests++;
		if (config_prof)
			tcache->prof_accumbytes += usize;
	}

	tcache_event(tsd, tcache);
	return (ret);
}

JEMALLOC_ALWAYS_INLINE void
tcache_dalloc_small(tsd_t *tsd, tcache_t *tcache, void *ptr, szind_t binind)
{
	tcache_bin_t *tbin;
	tcache_bin_info_t *tbin_info;

	assert(tcache_salloc(ptr) <= SMALL_MAXCLASS);

	if (config_fill && unlikely(opt_junk_free))
		arena_dalloc_junk_small(ptr, &arena_bin_info[binind]);

	tbin = &tcache->tbins[binind];
	tbin_info = &tcache_bin_info[binind];
	if (unlikely(tbin->ncached == tbin_info->ncached_max)) {
		tcache_bin_flush_small(tsd, tcache, tbin, binind,
		    (tbin_info->ncached_max >> 1));
	}
	assert(tbin->ncached < tbin_info->ncached_max);
	tbin->avail[tbin->ncached] = ptr;
	tbin->ncached++;

	tcache_event(tsd, tcache);
}

JEMALLOC_ALWAYS_INLINE void
tcache_dalloc_large(tsd_t *tsd, tcache_t *tcache, void *ptr, size_t size)
{
	szind_t binind;
	tcache_bin_t *tbin;
	tcache_bin_info_t *tbin_info;

	assert((size & PAGE_MASK) == 0);
	assert(tcache_salloc(ptr) > SMALL_MAXCLASS);
	assert(tcache_salloc(ptr) <= tcache_maxclass);

	binind = size2index(size);

	if (config_fill && unlikely(opt_junk_free))
		arena_dalloc_junk_large(ptr, size);

	tbin = &tcache->tbins[binind];
	tbin_info = &tcache_bin_info[binind];
	if (unlikely(tbin->ncached == tbin_info->ncached_max)) {
		tcache_bin_flush_large(tsd, tbin, binind,
		    (tbin_info->ncached_max >> 1), tcache);
	}
	assert(tbin->ncached < tbin_info->ncached_max);
	tbin->avail[tbin->ncached] = ptr;
	tbin->ncached++;

	tcache_event(tsd, tcache);
}

JEMALLOC_ALWAYS_INLINE tcache_t *
tcaches_get(tsd_t *tsd, unsigned ind)
{
	tcaches_t *elm = &tcaches[ind];
	if (unlikely(elm->tcache == NULL))
		elm->tcache = tcache_create(tsd, arena_choose(tsd, NULL));
	return (elm->tcache);
}
#endif

#endif /* JEMALLOC_H_INLINES */
/******************************************************************************/
