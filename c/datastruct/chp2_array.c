#include "ds.h"

typedef int	elemtype;

typedef struct _ds_list {
	elemtype *elem;
	int  len;
	int  size;
}ds_list;

static int list_init(ds_list *lst)
{
	lst->size = 0;
	lst->len = 0;
	memset(lst, 0, sizeof(ds_list));

	lst->elem = (elemtype *)malloc(sizeof(elemtype) * INIT_LIST_SIZE);

	if (!lst->elem)
		return ERR;

	lst->size = INIT_LIST_SIZE;
	lst->len  = 0;

	return OK;
}

static void list_destroy(ds_list *lst)
{
	if (lst->elem) {
		free(lst->elem);
		lst->elem = NULL;
	}

	lst->size = 0;
	lst->len = 0;
}


static void list_clear(ds_list *lst)
{
	lst->len = 0;
}

static int list_empty(ds_list *lst)
{
	return (lst->len == 0)?TRUE:FALSE;
}

static int list_length(ds_list *lst)
{
	return lst->len;
}

static int list_get_elem(ds_list *lst, int i, elemtype *elem)
{
	assert(elem != NULL);


	if (i < 0 || i >= lst->len)
		return ERR;

	*elem = lst->elem[i];

	return *elem;
}

static int list_locate_elem(ds_list *lst, elemtype e)
{
	int i = 0;

	for (i = 0; i < lst->len; i++) {
		if (lst->elem[i] == e)
			return i;
	}

	return -1;
}

static int list_prior_elem(ds_list *lst, elemtype e)
{
	int i = 0;

	for (i = 1; i < lst->len; i++) {
		if (lst->elem[i] == e)
			return lst->elem[i - 1];
	}

	return -1;
}

static int list_next_elem(ds_list *lst, elemtype e)
{
	int i = 0;

	for (i = lst->len - 2 ; i >= 0; i--) {
		if (lst->elem[i] == e)
			return lst->elem[i + 1];
	}

	return -1;
}


static int list_insert(ds_list *lst, int i, elemtype e)
{
	int n;
	elemtype *tmp = NULL;

	if (i < 0 || i > lst->len)
		return ERR;

	if (lst->len >= lst->size) {

		tmp = (elemtype *)realloc(lst->elem, 
				(lst->size + INCREASE_SIZE) * sizeof(elemtype));

		if (!tmp)
			return ERR;
		lst->elem = tmp;
		lst->size += INCREASE_SIZE;
	}

	for (n = lst->len - 1; n >= i; n--) {
		lst->elem[n + 1] = lst->elem[n];
	}

	lst->elem[i] = e;
	lst->len++;

	return OK;
}

static int list_delete(ds_list *lst, int i, elemtype *e)
{
	if (i < 0 || i > lst->len)
		return ERR;

	*e = lst->elem[i];

	lst->len -= 1;

	for (; i < lst->len; i++) {
		lst->elem[i] = lst->elem[i+1];
	}

	return *e;
}


static int list_print(ds_list *lst)
{
	int i;

	printf("[ ");

	for (i = 0; i < lst->len; i++) {
		printf("%d, ", lst->elem[i]);
	}

	printf("]\n");

	return 0;
}

int main(int argc, char *argv[])
{
	int n = 0, i, num;
	ds_list lst;
	elemtype  elem;

	srand((unsigned int )time(NULL));

	list_init(&lst);
	printf("list information size: %d, length: %d\n", lst.size, list_length(&lst));

	list_clear(&lst);
	list_print(&lst);

	n = random() % 15;

	while (n-- > 0) {
		i = random() % (list_length(&lst) + 1);
		num = random() % 100;

		printf("n = %d insert (%d, %d) --- list(%d, %d)\n", 
				n + 1, i, num, lst.size, list_length(&lst));

		list_insert(&lst, i, num);
		list_print(&lst);
	}

	printf("list_empty ? %s\n", list_empty(&lst)?"yes":"no");

	while(list_length(&lst) > 0) {

		elem = -1;

		i = random() % (list_length(&lst) + 1);
		
		if (list_get_elem(&lst, i, &elem) == ERR) {
			continue;
		}

		printf("%d, elem (%d-> %d) \t\t ", list_length(&lst), i, elem);
		printf("localte, prior, next: %d-> %d, %d\n", 
				list_locate_elem(&lst, elem),
				list_prior_elem(&lst, elem), 
				list_next_elem(&lst, elem));

		list_delete(&lst, i, &elem);

		list_print(&lst);
	}

	list_print(&lst);
	printf("list_empty ? %s\n", list_empty(&lst)?"yes":"no");

	list_destroy(&lst);

	return 0;
}

