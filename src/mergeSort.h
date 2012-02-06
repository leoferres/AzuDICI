#ifndef MERGESORT_H
#define	MERGESORT_H

#include <stdio.h>
#include <stdlib.h>

struct sort_node {
 int number;
 struct sort_node *next;
};

/* add a sort_node to the linked list */
struct sort_node *addsort_node(int number, struct sort_node *next);
/* preform merge sort on the linked list */
struct sort_node *mergesort(struct sort_node *head);
/* merge the lists.. */
struct sort_node *merge(struct sort_node *head_one, struct sort_node *head_two);

#endif	/* MERGESORT_H */

