// consumer.cpp
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "protocol.h"

int main() {
    // 1. 이미 생성된 공유메모리 열기
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed (생산자가 아직 구동되지 않았을 수 있음)");
        return 1;
    }

    // 2. 메모리 매핑
    void* ptr = mmap(0, sizeof(SharedMemoryLayout), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    SharedMemoryLayout* shm = static_cast<SharedMemoryLayout*>(ptr);
    std::cout << "[Consumer] 공유메모리 연결 완료. 데이터 대기 중...\n";

    // pthread_mutex_lock(&shm->mutex);
    // shm->consumer_ready = 1;
    // pthread_mutex_unlock(&shm->mutex);
    // std::cout << "[Consumer] 생산자 출발 신호 완료. 수신 루프 진입.\n";

    // 3. 데이터 수신 대기 루프
    unsigned long total_processed = 0;
    while (total_processed <= 1000000) {
        pthread_mutex_lock(&(shm->mutex));

        // 데이터가 준비될 때까지 조건 변수로 대기 (커널 컨텍스트 스위칭 오버헤드 최소화)
        while (!shm->is_ready) {
            pthread_cond_wait(&(shm->cond), &(shm->mutex));
        }

        // 데이터 읽기 (복사)
        MarketData& local_data = shm->data;
        shm->is_ready = false; // 플래그 리셋

        pthread_mutex_unlock(&(shm->mutex));

        // 읽은 데이터 처리 (출력)
        // std::cout << "[Consumer] 수신 데이터 -> ID: " << local_data.code_id
        //           << " | Symbol: " << local_data.symbol
        //           << " | Price: " << local_data.price
        //           << " | Vol: " << local_data.volume << "\n";

        //if (local_data.code_id == 1005) break; // 마지막 데이터면 종료
        total_processed++;
    }

    // 자원 정리
    munmap(ptr, sizeof(SharedMemoryLayout));
    close(shm_fd);
    std::cout << "[Consumer] 종료.\n";
    return 0;
}
