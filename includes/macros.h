//
//  macros.h
//  myfs
//
//  Created by Oliver Waldhorst on 09.10.17.
//  Copyright © 2017 Oliver Waldhorst. All rights reserved.
//

#ifndef mymacros_h
#define mymacros_h

#define error(str)                \
do {                        \
fprintf(stderr, str "\n");\
exit(-1);\
} while(0)

#define seekto(dev, pos)                        \
do {                                    \
off_t __pos = (pos) * this->blockSize;                        \
if (lseek (dev, __pos, SEEK_SET) != __pos)                \
error ("seekto failed");    \
} while(0)

#define writebuf(dev,buf,size)            \
do {                            \
int __size = (size);                \
if (::write (dev, buf, __size) != __size)        \
error ("writebuf failed");    \
} while(0)

#define readbuf(dev,buf,size)            \
do {                            \
int __size = (size);                \
if (::read (dev, buf, __size) != __size)        \
error ("readbuf failed");    \
} while(0)

#endif /* mymacros_h */
