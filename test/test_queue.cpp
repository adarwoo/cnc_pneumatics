#include "queue.h"
#include "assert.h"

int main()
{
   queue_t queue1;

   assert( queue_init( &queue1, 4) );
   assert( queue_is_empty(&queue1) );

   assert( queue_push(&queue1, (void *)1) );
   assert( queue_push(&queue1, (void *)2) );
   assert( queue_push(&queue1, (void *)3) );
   assert( queue_push(&queue1, (void *)4) );

   assert( ! queue_push(&queue1, (void *)5) );
   assert( ! queue_push(&queue1, (void *)6) );
   assert( ! queue_push(&queue1, (void *)7) );
   assert( ! queue_push(&queue1, (void *)8) );
   assert( ! queue_push(&queue1, (void *)5) );
   assert( ! queue_push(&queue1, (void *)6) );
   assert( ! queue_push(&queue1, (void *)7) );
   assert( ! queue_push(&queue1, (void *)8) );

   long res;

   queue_pop(&queue1, (void **)&res);
   assert( res == 1 );
   queue_pop(&queue1, (void **)&res);
   assert( res == 2 );
   queue_pop(&queue1, (void **)&res);
   assert( res == 3 );
   queue_pop(&queue1, (void **)&res);
   assert( res == 4 );

   assert( queue_push(&queue1, (void *)5) );
   assert( queue_push(&queue1, (void *)6) );
   assert( queue_push(&queue1, (void *)7) );
   assert( queue_push(&queue1, (void *)8) );

   assert( ! queue_push(&queue1, (void *)9) );

   queue_pop(&queue1, (void **)&res);
   assert( res == 5 );

   assert( queue_push(&queue1, (void *)9) );

   queue_push_ring(&queue1, (void *)10);

   queue_pop(&queue1, (void **)&res);
   assert( res == 7 );
   queue_pop(&queue1, (void **)&res);
   assert( res == 8 );
   queue_pop(&queue1, (void **)&res);
   assert( res == 9 );
   queue_pop(&queue1, (void **)&res);
   assert( res == 10 );

   queue_push_ring(&queue1, (void *)20);
   queue_push_ring(&queue1, (void *)21);
   queue_push_ring(&queue1, (void *)22);
   queue_push_ring(&queue1, (void *)23);

   queue_pop(&queue1, (void **)&res);
   assert( res == 20 );
   queue_pop(&queue1, (void **)&res);
   assert( res == 21 );
   queue_pop(&queue1, (void **)&res);
   assert( res == 22 );
   queue_pop(&queue1, (void **)&res);
   assert( res == 23 );


}