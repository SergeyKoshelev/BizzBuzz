#include <stdio.h>

enum Choice {
    cat = 1,
    dog = 2,
    snake = 3
};

int main()
{
    int my_choice;
    scanf("%d", &my_choice);

    switch (my_choice){
        case cat:
        {
            printf("cat\n");
            break;
        }
        default:
            printf("dog or snake left\n");
        case dog:
        {
            printf("dog\n");
            break;
        }
    }
}