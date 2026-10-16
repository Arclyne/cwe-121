#include <stdio.h>

int main()
{
    int cookie;
    char buf[10];

    printf("buf: %08x cookie: %08x\n", &buf, &cookie);
    gets(buf);

    if (cookie == 0x000d0a00)
        printf("ganaste angel!\n");
    return 0;
}