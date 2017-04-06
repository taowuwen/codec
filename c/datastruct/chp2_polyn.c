#include <ds.h>

#define MAX_EXP		0x7fffffff


typedef struct {
	float coef;
	int   expn;
} term, ElemType;

typedef struct _ds_list{
	ElemType elem;
	struct _ds_list *next;
}polynomial;


/*
 * init
 * destroy
 *
 * insert
 * delete
 * 
 * find
 * get
 *
 * empty
 * length
 *
 * print
 *
 * add
 * sub
 * mul
 * div
 *
 * */

static polynomial *polynomial_new()
{
	polynomial *p = NULL;

	p = (polynomial *)malloc(sizeof(polynomial));

	if (p)
		polynomial_init(p);

	return p;
}

static void polynoimal_destroy(polynomial *p)
{
	if (p)
		free(p);
}

static void polynomial_init(polynomial *p)
{
	assert(p != NULL);

	p->elem.coef = 0;
	p->elem.expn = 0;
	p->next = NULL;
}


static int get_expn_prev()
{
	return 0;
}

static int polynomial_insert(polynomial *lst, polynomial *p)
{
	assert(lst && p);

	return 0;
}

static polynomial *polynomial_find(polynomial *lst, int expn)
{
	polynomial *p = NULL;

	assert(lst != NULL);

	for (p = lst->next; p != NULL; p = p->next) {

		if (p->elem.expn == expn)
			return p;
	}

	return NULL;
}

static void polynomial_print(polynomial *lst)
{
	polynomial *p = lst->next;
	printf("\t[ ");

	if (p != NULL) {

		printf("f(x) = %fx^%d", p->elem.coef, p->expn);

		p = p->next;

		while (p != NULL) {
			printf(" + %fx^%d", p->elem.coef, p->expn);
			p = p->next;
		}
	}

	printf(" ]\n");
}

static polynomial *polynomail_locate(polynomial *lst, int expn, float coef)
{
	return NULL;
}


static polynomial *polynomial_add(polynomial *p1, polynomial *p2)
{
	return NULL;
}

static polynomial *polynomial_sub(polynomial *p1, polynomial *p2)
{
	return NULL;
}

static polynomial *polynomial_mul(polynomial *p1, polynomial *p2)
{
	return NULL;
}


int main(int argc, char *argv[])
{
	int n;
	float c;

	srandom((unsigned int)time(NULL));

	return 0;
}

