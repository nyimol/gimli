#include <stdio.h>
#include <unistd.h>

int main () {
    int pid = getpid ();
    printf ("pid is %d\n", pid);
    return 0;
}
