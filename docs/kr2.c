#include <stdio.h>
#include <unistd.h>

typedef long Align;             /* for alignment to long boundary */
union header {                  /* block header */
    struct {
        union header *ptr;      /* next block if on free list */
        size_t size;            /* size of this block */
                                /*      including the Header itself */
                                /*      measured in count of Header chunks */
                                /*      not less than NALLOC Header's */
    } s;
    Align x;                    /* force alignment of blocks */
};
typedef union header Header;

static Header *morecore(size_t);
void *mmalloc(size_t);
void _mfree(void **);
void visualize(const char*);
size_t getfreem(void);
size_t totmem = 0;              /* total memory in chunks */

static Header base;             /* empty list to get started */
static Header *freep = NULL;    /* start of free list */

#define NALLOC 1                /* minimum chunks to request */
#define MAXMEM 2048             /* max memory available (in bytes) */

#define mfree(p) _mfree((void **)&p)

void *sbrk(__intptr_t incr);


int main(void)
{
    char *pc, *pcc, *pccc, *ps;
    long *pd, *pdd;
    int dlen = 100;
    int ddlen = 50;

    visualize("start");


    /* trying to fragment as much as possible to get a more interesting view */

    /* claim a char */
    if ((pc = (char *) mmalloc(sizeof(char))) == NULL)
        return -1;

    /* claim a string */
    if ((ps = (char *) mmalloc(dlen * sizeof(char))) == NULL)
        return -1;

    /* claim some long's */
    if ((pd = (long *) mmalloc(ddlen * sizeof(long))) == NULL)
        return -1;

    /* claim some more long's */
    if ((pdd = (long *) mmalloc(ddlen * 2 * sizeof(long))) == NULL)
        return -1;

    /* claim one more char */
    if ((pcc = (char *) mmalloc(sizeof(char))) == NULL)
        return -1;

    /* claim the last char */
    if ((pccc = (char *) mmalloc(sizeof(char))) == NULL)
        return -1;


    /* free and visualize */
    printf("\n");
    mfree(pccc);
    /*      bugged on purpose to test free(NULL) */
    mfree(pccc);
    visualize("free(the last char)");

    mfree(pdd);
    visualize("free(lot of long's)");

    mfree(ps);
    visualize("free(string)");

    mfree(pd);
    visualize("free(less long's)");

    mfree(pc);
    visualize("free(first char)");

    mfree(pcc);
    visualize("free(second char)");


    /* check memory condition */
    size_t freemem = getfreem();
    printf("\n");
    printf("--- Memory claimed  : %ld chunks (%ld bytes)\n",
                totmem, totmem * sizeof(Header));
    printf("    Free memory now : %ld chunks (%ld bytes)\n",
                freemem, freemem * sizeof(Header));
    if (freemem == totmem)
        printf("    No memory leaks detected.\n");
    else
        printf("    (!) Leaking memory: %ld chunks (%ld bytes).\n",
                    (totmem - freemem), (totmem - freemem) * sizeof(Header));

    printf("// Done.\n\n");
    return 0;
}


/* visualize: print the free list (educational purpose) */
void visualize(const char* msg)
{
    Header *tmp;

    printf("--- Free list after \"%s\":\n", msg);

    if (freep == NULL) {                   /* does not exist */
        printf("\tList does not exist\n\n");
        return;
    }

    if  (freep == freep->s.ptr) {          /* self-pointing list = empty */
        printf("\tList is empty\n\n");
        return;
    }

    printf("  ptr: %10p size: %-3lu -->  ", (void *) freep, freep->s.size);

    tmp = freep;                           /* find the start of the list */
    while (tmp->s.ptr > freep) {           /* traverse the list */
        tmp = tmp->s.ptr;
        printf("ptr: %10p size: %-3lu -->  ", (void *) tmp, tmp->s.size);
    }
    printf("end\n\n");
}


/* calculate the total amount of available free memory */
size_t getfreem(void)
{
    if (freep == NULL)
        return 0;

    Header *tmp;
    tmp = freep;
    size_t res = tmp->s.size;

    while (tmp->s.ptr > tmp) {
        tmp = tmp->s.ptr;
        res += tmp->s.size;
    }

    return res;
}


/* mmalloc: general-purpose storage allocator */
void *mmalloc(size_t nbytes)
{
    Header *p, *prevp;
    size_t nunits;

    /* smallest count of Header-sized memory chunks */
    /*  (+1 additional chunk for the Header itself) needed to hold nbytes */
    nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;

    /* too much memory requested? */
    if (((nunits + totmem + getfreem())*sizeof(Header)) > MAXMEM) {
        printf("Memory limit overflow!\n");
        return NULL;
    }

    if ((prevp = freep) == NULL) {          /* no free list yet */
        /* set the list to point to itself */
        base.s.ptr = freep = prevp = &base;
        base.s.size = 0;
    }

    /* traverse the circular list */
    for (p = prevp->s.ptr; ; prevp = p, p = p->s.ptr) {

        if (p->s.size >= nunits) {          /* big enough */
            if (p->s.size == nunits)        /* exactly */
                prevp->s.ptr = p->s.ptr;
            else {                          /* allocate tail end */
                /* adjust the size */
                p->s.size -= nunits;
                /* find the address to return */
                p += p->s.size;
                p->s.size = nunits;
            }
            freep = prevp;
            return (void *)(p+1);
        }

        /* back where we started and nothing found - we need to allocate */
        if (p == freep)                     /* wrapped around free list */
            if ((p = morecore(nunits)) == NULL)
                return NULL;                /* none left */
    }
}


/* morecore: ask system for more memory */
/*      nu: count of Header-chunks needed */
static Header *morecore(size_t nu)
{
    char *cp;
    Header *up;

    /* get at least NALLOC Header-chunks from the OS */
    if (nu < NALLOC)
        nu = NALLOC;

    cp = (char *) sbrk(nu * sizeof(Header));

    if (cp == (char *) -1)                  /* no space at all */
        return NULL;

    printf("... (sbrk) claimed %ld chunks.\n", nu);
    totmem += nu;                           /* keep track of allocated memory */

    up = (Header *) cp;
    up->s.size = nu;

    /* add the free space to the circular list */
    void *n = (void *)(up+1);
    mfree(n);

    return freep;
}


/* mfree: put block ap in free list */
void _mfree(void **ap)
{
    if (*ap == NULL)
        return;

    Header *bp, *p;
    bp = (Header *)*ap - 1;                 /* point to block header */

    if (bp->s.size == 0 || bp->s.size > totmem) {
        printf("_mfree: impossible value for size\n");
        return;
    }

    /* the free space is only marked as free, but 'ap' still points to it */
    /* to avoid reusing this address and corrupt our structure set it to '\0' */
    *ap = NULL;

    /* look where to insert the free space */

    /* (bp > p && bp < p->s.ptr)    => between two nodes */
    /* (p > p->s.ptr)               => this is the end of the list */
    /* (p == p->p.str)              => list is one element only */
    for (p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
        if (p >= p->s.ptr && (bp > p || bp < p->s.ptr))
            /* freed block at start or end of arena */
            break;

    if (bp + bp->s.size == p->s.ptr) {      /* join to upper nbr */
    /* the new block fits perfect up to the upper neighbor */

        /* merging up: adjust the size */
        bp->s.size += p->s.ptr->s.size;
        /* merging up: point to the second next */
        bp->s.ptr = p->s.ptr->s.ptr;

    } else
        /* set the upper pointer */
        bp->s.ptr = p->s.ptr;

    if (p + p->s.size == bp) {              /* join to lower nbr */
    /* the new block fits perfect on top of the lower neighbor */

        /* merging below: adjust the size */
        p->s.size += bp->s.size;
        /* merging below: point to the next */
        p->s.ptr = bp->s.ptr;

    } else
        /* set the lower pointer */
        p->s.ptr = bp;

    /* reset the start of the free list */
    freep = p;
}
