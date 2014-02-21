#define LEN_BUF 256
#define LEN_MSG 5000

struct dequeue
{
  char msg[LEN_BUF];
  struct dequeue *next;
  struct dequeue *prev;
};
