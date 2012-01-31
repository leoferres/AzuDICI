/*
 * This is a thread safe version of vector implementation kvec.h, using read/write locks from pthread.h. 
 * 
 * The usage is as following (assuming we want a vector of integers):
 * 
 * Declare a vector of type int:
 *      ts_vec_t(int) myVector;
 * 
 * Initialize the vector:
 *      ts_vec_init(myVector);
 * 
 * Push an element at the end of the vector:
 *      ts_vec_push_back(int,myVector,5); //will append 5 at the end of the vector
 * 
 * Pop the last element in the vector:
 *      int number;
 *      ts_vec_pop(number,myVector); //Last item is stored in number, the vector size is decreased by 1
 * 
 * Get the ith element in the vector:
 *      int number;
 *      ts_vec_ith(number,myVector,i); //The value in the position i is stored in number
 * 
 * Get the size of the vector:
 *      int size;
 *      ts_vec_size(size,myVector); //The size is stored in variable size
 * 
 * Set the ith element in the vector to 20:
 *      ts_vec_set_ith(int,myVector,i,20);
 * 
 * Resize the vector to hold 50 elements:
 *      ts_vec_resize(int,myVector,50);
 *  
 */

#ifndef TS_VEC_H
#define	TS_VEC_H

#include <stdlib.h>
#include <pthread.h>

pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

#define ts_vec_roundup32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))

#define ts_vec_t(type) struct { size_t n, m; type *a; }

#define ts_vec_init(v) ((v).n = (v).m = 0, (v).a = 0)

#define ts_vec_destroy(v) do{ \
                        pthread_rwlock_wrlock(&rwlock); \
                        free((v).a) \
                        pthread_rwlock_unlock(&rwlock); \
                        } while(0)

#define ts_vec_ith(temp, v, i) do{ \
                               pthread_rwlock_rdlock(&rwlock); \
                               temp= ((v).a[(i)]); \
                               pthread_rwlock_unlock(&rwlock); \
                               } while(0)

#define ts_vec_pop(temp, v) do{ \
                               pthread_rwlock_wrlock(&rwlock); \
                               temp= ((v).a[--(v).n]); \
                               pthread_rwlock_unlock(&rwlock); \
                               } while(0)

#define ts_vec_size(temp, v) do{ \
                               pthread_rwlock_rdlock(&rwlock); \
                               temp= ((v).n); \
                               pthread_rwlock_unlock(&rwlock); \
                               } while(0)

#define ts_vec_max(temp, v) do{ \
                        pthread_rwlock_rdlock(&rwlock); \
                        temp= ((v).m); \
                        pthread_rwlock_unlock(&rwlock); \
                        } while(0)

#define ts_vec_resize(type, v, s) do{ \
                                        pthread_rwlock_wrlock(&rwlock); \
                                        ((v).m = (s), (v).a = (type*)realloc((v).a, sizeof(type) * (v).m)); \
                                        pthread_rwlock_unlock(&rwlock); \
                                    } while(0) 

#define ts_vec_push_back(type, v, x) do { \
                                        pthread_rwlock_wrlock(&rwlock); \
                                        if ((v).n == (v).m) { \
                                                (v).m = (v).m? (v).m<<1 : 2; \
                                                (v).a = (type*)realloc((v).a, sizeof(type) * (v).m); \
                                        } \
                                        (v).a[(v).n++] = (x); \
                                        pthread_rwlock_unlock(&rwlock); \
                                   } while (0)

#define ts_vec_pushp(type, v) (((v).n == (v).m)? \
						   ((v).m = ((v).m? (v).m<<1 : 2), \
							(v).a = (type*)realloc((v).a, sizeof(type) * (v).m), 0)	\
						   : 0), ((v).a + ((v).n++))

#define ts_vec_set_ith(type, v, i, temp)              do{ \
                                                pthread_rwlock_wrlock(&rwlock); \
                                                ((v).m <= (size_t)(i)? \
						  ((v).m = (v).n = (i) + 1, ts_vec_roundup32((v).m), \
						   (v).a = (type*)realloc((v).a, sizeof(type) * (v).m), 0) \
						  : (v).n <= (size_t)(i)? (v).n = (i) \
						  : 0), (v).a[(i)] = temp; \
                                                pthread_rwlock_unlock(&rwlock); \
                                              } while(0)

#endif	/* TS_VEC_H */

