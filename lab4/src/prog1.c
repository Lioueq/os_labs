#include <stdio.h>
#include <stdlib.h>

int GCF(int a, int b);
char* translation(long x);

int main() {
    while (1) {
        printf("1 - GCF\n2 - Translate number\n3 - exit\n");
        int n;
        scanf("%d", &n);
        switch (n) {
            case 1:
                printf("Введите числа a b\n");
                int a, b;
                scanf("%d", &a);
                scanf("%d", &b);
                printf("%d\n", GCF(a, b));
                break;
            
            case 2:
                printf("Введите число\n");
                long x;
                scanf("%d", &x);
                printf("%s\n", translation(x));
                break;
            case 3:
                exit(0);

            default:
                break;

            }
        }
    return 0;
}