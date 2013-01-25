#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>

#define REPEAT9(x) { x x x x x x x x x }

/* gcc specific */
typedef unsigned int uint128_t __attribute__((__mode__(TI)));

#define hashmult 13493690561280548289ULL
/* #define hashmult 2654435761 */

struct block {
  char *addr;
  size_t len;
};

inline struct block slurp(char *filename)
{
  int fd=open(filename,O_RDONLY);
  struct stat statbuf;
  struct block r;
  if (fd==-1)
    err(1, "%s", filename);
  if (fstat(fd, &statbuf)==-1)
    err(1, "%s", filename);
  /* allocates an extra 7 bytes to allow full-word access to the last byte */
  r.addr = mmap(NULL,statbuf.st_size+7, PROT_READ|PROT_WRITE,MAP_FILE|MAP_PRIVATE,fd, 0);
  if (r.addr==MAP_FAILED)
    err(1, "%s", filename);
  r.len = statbuf.st_size;
  return r;
}

#define HASHSIZE (1<<20)
#define HASHMOD (HASHSIZE-1)

struct hashnode {
  struct hashnode *next; /* link in external chaining */
  char *keyaddr;
  size_t keylen;
  int value;
};

struct hashnode *ht[HASHSIZE];

inline unsigned long hash(char *addr, size_t len)
{
  /* assumptions: 1) unaligned accesses work 2) little-endian 3) 7 bytes
     beyond last byte can be accessed */
  uint128_t x;
  unsigned long * laddr = (unsigned long *) addr;
  unsigned long * end =  (unsigned long *) (addr+len);

  if(len > 7 ) {
    x = *laddr * hashmult;
    end--;
    for (laddr++; laddr <= end; laddr++) {
      x = (x + *laddr)*hashmult;
    }
    if (laddr < (end+1))
      x = ( x + ((*laddr)<< ( ((char*)laddr - (char*)end)*8)) ) * hashmult;
    return x+(x>>64);
  } else if (laddr < end) {
    x = (uint128_t)((*laddr)<<((8-len)*8)) * hashmult;
    return x+(x>>64);
  }
  return 0;
}

inline void insert(char *keyaddr, size_t keylen, int value)
{
  struct hashnode **l=&ht[hash(keyaddr, keylen) & (HASHMOD)];
  struct hashnode *n = malloc(sizeof(struct hashnode));
  n->next = *l;
  n->keyaddr = keyaddr;
  n->keylen = keylen;
  n->value = value;
  *l = n;
}

inline int mycmp(char* in1, char* in2, int len){
  do{
    if(*in1 ^ *in2) return 0;
    in1++; in2++; len--;
  }while(len>0);
  return 1;
}

inline int lookup(char *keyaddr, size_t keylen)
{
  struct hashnode *l=ht[hash(keyaddr, keylen) & (HASHMOD)];

  if(l != NULL) {
    if (keylen == l->keylen && mycmp(l->keyaddr, keyaddr, keylen))
        return l->value;
    l = l->next;
    while (l!=NULL) {
      if (keylen == l->keylen && mycmp(keyaddr, l->keyaddr, keylen))
        return l->value;
      l = l->next;
    }
  }
  return -1;
}

/*
int main()
{
  char a[40]="abcdefghijklmnopqrstuvwxyz1234567890";
  char b[40]="abcdefghijklmnopqrstuvwxyz1234567890";

  int i;
  for (i=32; i>=0; i--) {
    a[i] = '$';
    b[i] = '%';
    printf("%ld,%ld\n",hash(a,i),hash(b,i));
    if (hash(a,i)!=hash(b,i)) {
      fprintf(stderr, "hash error\n");
      exit(1);
    }
  }
  return 0;
}
*/  
      
int main(int argc, char *argv[])
{
  struct block input;
  char *p, *nextp, *endp;
  unsigned long r;
  if (argc^3) {
    fprintf(stderr, "usage: %s <dict-file> <lookup-file>\n", argv[0]);
    exit(1);
  }

  input = slurp(argv[1]);
  endp=input.addr+input.len;
  *endp = '\n';
  p=input.addr;
  nextp = p;

  for (r=0; nextp<endp; r++) {

    for(;*nextp ^ '\n'; nextp++);
    insert(p, nextp-p, r);
    nextp++;
    p = nextp;
  }
#if 0 
 struct hashnode *n;
  unsigned long count, sumsq=0, sum=0;
  for (i=0; i<HASHSIZE; i++) {
    for (n=ht[i], count=0; n!=NULL; n=n->next)
      count++;
    sum += count;
    sumsq += count*count;
  }
  printf("sum=%ld, sumsq=%ld, hashlen=%ld, chisq=%f\n",
	 sum, sumsq, HASHSIZE, ((double)sumsq)*HASHSIZE/sum-sum);
  /* expected value for chisq is ~HASHSIZE */
#endif
  
  int size = input.len/6;
  int *cache = malloc(size*sizeof(int)), *startcache, *endcache;
  startcache = cache;
  endcache = startcache + size;
  input = slurp(argv[2]);
  r = 0;
  endp=input.addr+input.len;
    p=input.addr;
    nextp = p;

  //loop peeling with caching  
  while(nextp<endp) {

    if (cache >= endcache){
      size = size<<1;
      cache = realloc(cache, size*sizeof(int));
    }
    for(;*nextp ^ '\n'; nextp++);
    *cache = lookup(p, nextp-p);

    r = r * 2654435761L + *cache;
    r = r + (r>>32);
    cache++;

    nextp++;
    p = nextp;
  }
  endcache = cache;

  REPEAT9 (
    cache = startcache;
    while (cache < endcache) {
      r = r * 2654435761L + *cache;
      r = r + (r>>32);
      cache++;
    }
  );
  printf("%ld\n",r);
  return 0;
} 
