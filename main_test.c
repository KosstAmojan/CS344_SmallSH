//
// Created by Matthew Norwood on 2/3/22.
//
#include "main.h"

//
// Created by Matthew Norwood on 1/6/22.
//
#include "main.h"
void test_pidExpand01();

int main(int argc, char *argv[])
{
    /*if (argc < 2)
    {
        printf("You must provide the name of the file to process\n");
        printf("Example usage: ./movies movies_sample_1.csv\n");
        return EXIT_FAILURE;
    }

    struct movie *list = processFile(argv[1]);*/

    printf("\n");
    printf("Starting pidExpand Test1\n");
    printf("----------------------------------------------------------------------------------------------------------------------\n");
    test_pidExpand01();
    printf("\n\n");


    return EXIT_SUCCESS;

}

void test_pidExpand01() {
    char *inputString = calloc(32, sizeof(char));
    char *printString = "Please print the process id: $$";
    size_t alloc_mem = sizeof(char) * 32;
    strcpy(inputString, printString);

            //(*inputString), *printString);
    pid_t shPid = getpid();
    printf("The string before: %s, The alloc_mem before %zu, The pid %d.", inputString, alloc_mem, shPid);
    alloc_mem = pidExpand(inputString;
    printf("The string after: %s, The alloc_mem after %zu, The pid %d.", inputString, alloc_mem, shPid);
    free(inputString);
}

