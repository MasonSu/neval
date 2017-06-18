#include <stdio.h>

#define NALLOC 1024

typedef long Align; // for alignment to long boundary

union header { // block header
  struct {
    union header *ptr; // next block if on free list
    unsigned size;     // size of this block
  } s;
  Align x; // force alignment of blocks
};

typedef union header Header;

static Header base;          // empty list to get started
static Header *freep = NULL; // start of free list

void *malloc(unsigned nbytes);
void free(void *ap);
static Header *morecore(unsigned nu);

/* malloc: general-purpose storage allocator */
void *malloc(unsigned nbytes) {
  Header *p, *prevp;
  unsigned nunits;

  nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;
  /* no free list yet */
  if ((prevp = freep) == NULL) {
    base.s.ptr = freep = prevp = &base;
    base.s.size = 0;
  }

  for (p = prevp->s.ptr;; prevp = p, p = p->s.ptr) {
    /* big enough */
    if (p->s.size >= nunits) {
      /* exactly */
      if (p->s.size == nunits) {
        prevp->s.ptr = p->s.ptr;
      } else {
        /* allocate tail end */
        p->s.size -= nunits;
        p += p->s.size;
        p->s.size = nunits;
      }

      freep = prevp;
      return (void *)(p + 1);
    }
    /* wrapped around free list */
    if (p == freep)
      if ((p = morecore(nunits)) == NULL)
        return NULL;
  }
}

/* ask system for more memory */
static Header *morecore(unsigned nu) {
  char *cp, *sbrk(int);
  Header *up;

  if (nu < NALLOC)
    nu = NALLOC;
  cp = sbrk(nu * sizeof(Header));
  /* no space at all*/
  if (cp == (char *)-1)
    return NULL;
  up = (Header *)cp;
  up->s.size = nu;
  free((void *)(up + 1));
  return freep;
}

/* free: put block ap in free list */
void free(void *ap) {
  Header *bp, *p;
  /* point to block header */
  bp = (Header *)ap - 1;
  for (p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr) {
    if (p >= p->s.ptr && (bp > p || bp < p->s.ptr))
      break;
  }
  /* join to upper nbr */
  if (bp + bp->s.size == p->s.ptr) {
    bp->s.size += p->s.ptr->s.size;
    bp->s.ptr = p->s.ptr->s.ptr;
  } else {
    bp->s.ptr = p->s.ptr->s.ptr;
  }
  /* join to lower nbr */
  if (p + p->s.size == bp) {
    p->s.size += bp->s.size;
    p->s.ptr = bp->s.ptr;
  } else {
    p->s.ptr = bp;
  }
  freep = p;
}