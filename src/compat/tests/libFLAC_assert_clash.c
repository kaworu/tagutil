/*
 * tests/libFLAC_assert_clash.c
 *
 * test if libFLAC <assert.h> will clash with the standard one.
 * see http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=705601
 */
#include <assert.h>


int
main(int argc, char *argv[])
{

       assert(1);
       return (0);
}
