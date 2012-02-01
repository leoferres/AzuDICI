#ifndef _LITERAL_H_
#define _LITERAL_H_

typedef int Literal; /* A literal is just an int. */
unsigned int lit_as_uint(Literal l);

/* Return the 0 literal*/
inline Literal zero_lit(){
    return 0;
}

#endif /* _LITERAL_H_ */
