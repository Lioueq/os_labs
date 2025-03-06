
v1:
	gcc -o prog1 prog1.c -L. -lprogv1

v2:
	gcc -o prog1 prog1.c -L. -lprogv2

prog2:
	gcc -o prog2 prog2.c -ldl

libs:
	gcc -fPIC -shared -o libprogv2.so GCF1.c translation2.c
	gcc -fPIC -shared -o libprogv1.so GCF1.c translation1.c

clean:
	-rm prog1
	-rm prog2
	-rm *.so
