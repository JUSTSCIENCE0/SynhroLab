#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <list>
#include <string>
#include <cstring>
#include <chrono>
#include <iostream>
#include <semaphore.h>

sem_t semaphore;

void* thread_copy_file(void* filenames)
{
    if(sem_wait(&semaphore))
    {
        printf("Failed wait sem with errno %d", errno);
        return NULL;
    }

    std::string* names = (std::string*)filenames;
    char command[1024];
    snprintf(command, 1024, "cp %s %s", names[0].c_str(), names[1].c_str());
    sleep(1);
    printf("Coping with %s\n", command);
    system(command);

    if(sem_post(&semaphore))
    {
        printf("Failed post sem with errno %d", errno);
        return NULL;
    }

    return NULL;
}

int main()
{
    std::string work_dir;
    std::string dest;

    printf("Enter work_dir:\n");
    std::cin >> work_dir;
    printf("Enter destination:\n");
    std::cin >> dest;

    std::list<std::string> files;
    DIR *dir = opendir(work_dir.c_str());
    if (!dir)
    {
        printf("Directory %s not exist\n", work_dir.c_str());
        return -1;
    }

    struct dirent *f_cur;
    while ((f_cur=readdir(dir))!=NULL)
    {
        if (strncmp(".", f_cur->d_name, 1))
        {
            printf("%s\n", f_cur->d_name);
            files.push_back(std::string(f_cur->d_name));
        }
    }

    size_t count = files.size();

    if(sem_init(&semaphore, 0, 1))
    {
        printf("Failed init sem with errno %d", errno);
        return -1;
    }

    const auto start = std::chrono::system_clock::now();

    pthread_t* thr_id = new pthread_t[count];

    auto it = files.begin();
    std::string** params = new std::string*[count];
    for (int i=0; it != files.end(); it++, i++)
    {
        std::string file = *it;
        printf("%s - Create thread\n", file.c_str());
        params[i] = new std::string[2];
        params[i][0] = work_dir + "/" + file;
        params[i][1] = dest + "/" + file;
        int status = pthread_create(&thr_id[i], NULL, thread_copy_file, (void*)params[i]);
        if (status)
        {
            perror("pthread error\n");
            exit(EXIT_FAILURE);
        }
    }

    for (int i=0; i<count; i++)
    {
        pthread_join(thr_id[i], NULL);
    }

    const auto end = std::chrono::system_clock::now();
    const auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    sem_destroy(&semaphore);
    printf("Execution time = %lld ms\n", delta);
    printf("Success!\n");
}
