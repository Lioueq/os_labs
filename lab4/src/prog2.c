#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

int (*GCF)(int a, int b);
char* (*translation)(long x);

void load_library(int mode) {
    void *handle;
    char *error;
    if (mode == 1) {
        handle = dlopen("./libprogv1.so", RTLD_LAZY);
    }
    else if (mode == 2) {
        handle = dlopen("./libprogv2.so", RTLD_LAZY);
    }
    if (!handle) {
        perror(dlerror());
        exit(EXIT_FAILURE);
    }

    dlerror();
    *(void **) (&GCF) = dlsym(handle, "GCF");
    if ((error = dlerror()) != NULL)  {
        perror(error);
        exit(EXIT_FAILURE);
    }

    dlerror();
    *(void **) (&translation) = dlsym(handle, "translation");
    if ((error = dlerror()) != NULL)  {
        perror(error);
        exit(EXIT_FAILURE);
    }
}

int main() {
    int mode = 1;
    load_library(mode);

    while (1) {
        printf("Текущая реализация: %dv\n0 - Переключить реализацию\n1 - GCF\n2 - Translate number\n3 - Exit\n", mode);
        int n;
        scanf("%d", &n);
        switch (n) {
            case 0:
                mode = mode % 2 + 1;
                load_library(mode);
                break;
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
                scanf("%ld", &x);
                printf("%s\n", translation(x));
                break;
            case 3:
                exit(0);

            default:
                printf("Неверный пункт\n");
                break;
        }
    }
    return 0;
}