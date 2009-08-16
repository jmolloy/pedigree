#include <stdio.h>

class Anus
{
public:
    Anus(int a) : anus(6) {anus = a;}
    ~Anus() {}
    void print() {printf("%d", anus);}
private:
    int anus;
};

Anus myanus(42);

int main(int argc, char **argv)
{
    printf("My anus says: ");
    myanus.print();
    printf("\n");
    return 0;
}
