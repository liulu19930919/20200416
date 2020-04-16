#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
static int size = 0;
int *generateRandomArray(int maxSize, int maxValue)
{
    srand((unsigned int)time(NULL));        // 设置种子
    size = rand() % (maxSize + 1);      // 产生0-maxSize 之间的数
    int *arr =(int*)malloc(sizeof(int) * size);  // 分配数组
    for(int i = 0; i < size; ++i)
    {
        srand((unsigned int)time(NULL) + i);
        int tmp1 = rand() % (maxValue + 1);// 数组赋值，值在0-maxvalue之间。
        int tmp2 = rand() % (maxValue + 1);
        arr[i] = tmp1 -tmp2;

    }
    return arr;  //返回数组长度
}
void insertSort_as(int * arr, int size)
{
    int minIdex;
    for (int i = 0; i < size - 1; i++)
    {
        minIdex = i;
        for (int j = i + 1; j < size; ++j)
        {
            if (arr[minIdex] > arr[j])
            {
                minIdex = j;
            }
        }
        swaparr(arr, minIdex, i);
    }
}
void swaparr(int *arr, int i, int j)
{
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}
int main()
{
    for(int i = 0; i < 1; ++i)
    {

        int maxSize = 100;
        int maxValue = 100;
        int *num = generateRandomArray(maxSize, maxValue);
        printf("before:\n");
        for (int i = 1; i <= size; i++)
        {
            printf("%5d", num[i-1]);
            if ((i % 10) == 0)
                printf("\n");
        }
        putchar('\n');
        printf("after:\n");
        printf("size = %d\n",size);
        insertSort_as(num, size);
        for (int i = 1; i <= size; i++)
        {
            printf("%5d", num[i-1]);
            if ((i % 10) == 0)
                printf("\n");
        }
        printf("\n");
    }
    return 0;
}
