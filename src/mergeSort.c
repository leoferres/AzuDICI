#ifndef MERGESORT_C
#define MERGESORT_C

#include "mergeSort.h"


/* add a sort_node to the linked list */
struct sort_node *addsort_node(int number, struct sort_node *next) {
 struct sort_node *tsort_node;

 tsort_node = (struct sort_node*)malloc(sizeof(*tsort_node));

 if(tsort_node != NULL) {
  tsort_node->number = number;
  tsort_node->next = next;
 }

 return tsort_node;
}

/* preform merge sort on the linked list */
struct sort_node *mergesort(struct sort_node *head) {
 struct sort_node *head_one;
 struct sort_node *head_two;

 if((head == NULL) || (head->next == NULL)) 
  return head;

 head_one = head;
 head_two = head->next;
 while((head_two != NULL) && (head_two->next != NULL)) {
  head = head->next;
  head_two = head->next->next;
 }
 head_two = head->next;
 head->next = NULL;

 return merge(mergesort(head_one), mergesort(head_two));
}

/* merge the lists.. */
struct sort_node *merge(struct sort_node *head_one, struct sort_node *head_two) {
 struct sort_node *head_three;

 if(head_one == NULL) 
  return head_two;

 if(head_two == NULL) 
  return head_one;

 if(head_one->number < head_two->number) {
  head_three = head_one;
  head_three->next = merge(head_one->next, head_two);
 } else {
  head_three = head_two;
  head_three->next = merge(head_one, head_two->next);
 }

 return head_three;
}

#endif