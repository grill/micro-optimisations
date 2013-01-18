#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>

/* gcc specific */
typedef unsigned int uint128_t __attribute__((__mode__(TI)));

#define hashmult 13493690561280548289ULL
/* #define hashmult 2654435761 */

struct block {
  char *addr;
  size_t len;
};

struct block slurp(char *filename)
{
  int fd=open(filename,O_RDONLY);
  char *s;
  struct stat statbuf;
  struct block r;
  if (fd==-1)
    err(1, "%s", filename);
  if (fstat(fd, &statbuf)==-1)
    err(1, "%s", filename);
  /* allocates an extra 7 bytes to allow full-word access to the last byte */
  s = mmap(NULL,statbuf.st_size+7, PROT_READ|PROT_WRITE,MAP_FILE|MAP_PRIVATE,fd, 0);
  if (s==MAP_FAILED)
    err(1, "%s", filename);
  r.addr = s;
  r.len = statbuf.st_size;
  return r;
}

#define HASHSIZE (1<<20)

struct hashnode {
  struct hashnode *next; /* link in external chaining */
  char *keyaddr;
  size_t keylen;
  int value;
};

struct hashnode *ht[HASHSIZE];

unsigned long hash(char *addr, size_t len)
{
  /* assumptions: 1) unaligned accesses work 2) little-endian 3) 7 bytes
     beyond last byte can be accessed */
  uint128_t x=0, w;
  size_t i, shift;
  for (i=0; i+7<len; i+=8) {
    w = *(unsigned long *)(addr+i);
    x = (x + w)*hashmult;
  }
  if (i<len) {
    shift = (i+8-len)*8;
    /* printf("len=%d, shift=%d\n",len, shift);*/
    w = (*(unsigned long *)(addr+i))<<shift;
    x = (x + w)*hashmult;
  }
  return x+(x>>64);
}

void insert(char *keyaddr, size_t keylen, int value)
{
  struct hashnode **l=&ht[hash(keyaddr, keylen) & (HASHSIZE-1)];
  struct hashnode *n = malloc(sizeof(struct hashnode));
  n->next = *l;
  n->keyaddr = keyaddr;
  n->keylen = keylen;
  n->value = value;
  *l = n;
}

int lookup(char *keyaddr, size_t keylen)
{
  struct hashnode *l=ht[hash(keyaddr, keylen) & (HASHSIZE-1)];
  while (l!=NULL) {
    if (keylen == l->keylen && memcmp(keyaddr, l->keyaddr, keylen)==0)
      return l->value;
    l = l->next;
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
  struct block input1, input2;
  char *p, *nextp, *endp;
  unsigned int i;
  unsigned long r=0;
  if (argc!=3) {
    fprintf(stderr, "usage: %s <dict-file> <lookup-file>\n", argv[0]);
    exit(1);
  }
  input1 = slurp(argv[1]);
  input2 = slurp(argv[2]);
  for (p=input1.addr, endp=input1.addr+input1.len, i=0; p<endp; i++) {
    nextp=memchr(p, '\n', endp-p);
    if (nextp == NULL)
      break;
    insert(p, nextp-p, i);
    p = nextp+1;
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
  for (i=0; i<10; i++) {
    for (p=input2.addr, endp=input2.addr+input2.len; p<endp; ) {
      nextp=memchr(p, '\n', endp-p);
      if (nextp == NULL)
        break;
      r = ((unsigned long)r) * 2654435761L + lookup(p, nextp-p);
      r = r + (r>>32);
      p = nextp+1;
    }
  }
  printf("%ld\n",r);
  return 0;
} 
