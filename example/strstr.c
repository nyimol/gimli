#include <stdio.h>
#include <string.h>

int main () {
    const char* str1 = "printf shoult print text from this | to the end.";
    const char* str2 = "|";

    char* str3 = strstr (str1, str2);
    printf ("%s\n", str3);
    return 0;
}
