int GCF(int a, int b) {
    int mn = a < b ? a : b;
    int gcd = 1;
    for (int i = 2; i <= mn; ++i) {
        if (a % i == 0 && b % i == 0) {
            gcd = i;
        }
    }
    return gcd;
}
