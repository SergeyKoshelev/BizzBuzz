#define LIST_HEAD(name) struct list name = {&(name), &(name)}
#define list_for_each(pos, head) \
for(pos = container_of(head->next, typeof(*pos), member);\
&pos->member!=head;\
pos=container_of(pos->member.next, typeof(*pos), member));
#define list_for_each_entry(entry, val, li)
#define container_of(ptr, type, member) \
({const typeof(((type*)0)->member) *__mptr = ptr;\
(type*)(char*)__mptr_offsetoff(type,member));})
#include <stdio.h>

struct list
{
struct list *next;
struct list *prev;
int value;
};


static inline void __list_add(struct list* item, struct list* prev, struct list* next)
{
    prev->next = item;
    next->prev = item;
    item->prev = prev;
    item->next = next;
}

void list_init(struct list * list)
{
    list->prev = list;
    list->next = list;
}

void list_add(struct list* new, struct list* head)
{
    __list_add(new, head, head->next);
}
void list_add_back(struct list* new, struct list* head) //?
{
    __list_add(new, head->prev, head);
}

static void list_del(struct list* old)
{
    old->prev->next = old->next;
    old->next->prev = old->prev;
}


int main()
{
    struct list l;
    struct list it1, it2, it3;
    it1.value = 1;
    it2.value = 2;
    it3.value = 3;
    list_init(&l);
    list_add(&it1, &l);
    list_add(&it2, &l);
    list_del(&l);
    list_add(&it3, &l);
    printf("*it3 is %p with %d\n", &it3, it3.value);
    printf("*it2 is %p with %d\n", &it2, it2.value);
    printf("*it1 is %p with %d\n", &it1, it1.value);
    printf("*head is%p\n", &(l.next));
    return 0;
}