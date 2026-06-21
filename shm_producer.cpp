// producer.cpp
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include "protocol.h"

int main() {
    // 1. 공유메모리 객체 생성 (기존에 있으면 제거 후 생성)
    shm_unlink(SHM_NAME);
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        return 1;
    }

    // 2. 메모리 크기 설정
    if (ftruncate(shm_fd, sizeof(SharedMemoryLayout)) == -1) {
        perror("ftruncate failed");
        return 1;
    }

    // 3. 프로세스 가상 메모리에 매핑
    void* ptr = mmap(0, sizeof(SharedMemoryLayout), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    SharedMemoryLayout* shm = static_cast<SharedMemoryLayout*>(ptr);

    // 4. 뮤텍스 및 조건 변수 속성 설정 (PROCESS_SHARED)
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED); // 핵심 옵션
    pthread_mutex_init(&(shm->mutex), &mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);

    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);  // 핵심 옵션
    pthread_cond_init(&(shm->cond), &cond_attr);
    pthread_condattr_destroy(&cond_attr);

    shm->consumer_ready = 0;
    shm->is_ready = false;

    std::cout << "[Producer] 시스템 준비 완료. 데이터 송신을 시작합니다.\n";

    // bool wait_complete = false;
    // while (!wait_complete) {
    //     pthread_mutex_lock(&shm->mutex);   // 🔒 락 획득
    //     if (shm->consumer_ready == 1) {
    //         wait_complete = true;
    //     }
    //     pthread_mutex_unlock(&shm->mutex); // 🔓 락 해제
    //
    //     if (!wait_complete) {
    //         sleep(1); // 1ms 쉬면서 OS에 CPU 양보 (Spin-wait 부하 방지)
    //         std::cout << "[Producer] Waiting Consumer..\n";
    //     }
    // }
    // std::cout << "[Producer] 소비자가 확인되었습니다. 이제부터 락프리로 폭주합니다!\n";

    // 5. 데이터 시뮬레이션 (5회 데이터 갱신)
    for (int i = 1; i <= 1000000; ++i) {
        //sleep(2); // 2초 주기 시뮬레이션

        pthread_mutex_lock(&(shm->mutex));

        // 데이터 쓰기
        shm->data.code_id = 1000 + i;
        std::strncpy(shm->data.symbol, "KOSPI200", sizeof(shm->data.symbol));
        shm->data.price = 350000 + (i * 100);
        shm->data.volume = 10000 * i;
        shm->is_ready = true;

        std::cout << "[Producer] 데이터 갱신 완료 -> Price: " << shm->data.price << "\n";

        // 소비자에게 알림 및 락 해제
        pthread_cond_signal(&(shm->cond));
        pthread_mutex_unlock(&(shm->mutex));
    }

    // 자원 정리 (실무에서는 시그널 핸들러 등으로 안전하게 처리해야 함)
    munmap(ptr, sizeof(SharedMemoryLayout));
    close(shm_fd);
    return 0;
}
