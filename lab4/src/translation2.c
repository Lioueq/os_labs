#include <stdlib.h>
#include <string.h>

char* translation(long x) {
    char* ans = (char*)malloc(64 * sizeof(char));
    int i = 0;
    if (x == 0) {
        ans[i++] = '0';
    } 
    else {
        while (x > 0) {
            ans[i++] = (x % 3) + '0';
            x = x / 3;
        }
    }
    ans[i] = '\0';
    int len = strlen(ans);
    for (int j = 0; j < len / 2; j++) {
        char temp = ans[j];
        ans[j] = ans[len - j - 1];
        ans[len - j - 1] = temp;
    }
    return ans;
}
