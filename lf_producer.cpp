// producer.cpp
#include <iostream>
#include <chrono>
#include <cstring>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "protocol_lf.h"

int main() {
    // producer.cpp 수정 코드
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0) {
        std::cerr << "[Producer] 공유메모리 오픈 실패\n";
        return 1;
    }

    // ftruncate 반환값을 체크하여 에러 처리 추가
    if (ftruncate(shm_fd, sizeof(SharedRingBufferQueue)) != 0) {
        std::cerr << "[Producer] 공유메모리 크기 할당 실패\n";
        close(shm_fd);
        return 1;
    }

    // 2. 가상 주소 매핑 및 구조체 바인딩
    auto ptr = mmap(nullptr,
                      sizeof(SharedRingBufferQueue),
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED, shm_fd, 0);
    auto q = static_cast<SharedRingBufferQueue*>(ptr);

    // 구조체 내부 volatile 변수 명시적 초기화
    q->tail = 0;
    q->head = 0;

    std::cout << "[Producer] 수동 최적화 큐 생성 완료. (PID: " << getpid() << ")\n";

    while (q->consumer_ready == 0) {
        asm volatile("" ::: "memory"); // 컴파일러 장벽
        #if defined(__x86_64__)
        __builtin_ia32_pause();
        #endif
    }

    uint64_t current_tail = 0;

    // 100만 개 데이터 스트리밍 시작
    for (int i = 1; i <= 1000000; ++i) {
        uint64_t current_head = q->head;

        // [Full 판별] 두 일반 카운터의 뺄셈 거리가 2047개에 도달하면 가득 찬 상태
        while ((current_tail - current_head) == SharedRingBufferQueue::MASK) {
            // 컴파일러 장벽: 대기 루프 도는 동안 head 값을 계속 메모리에서 새로 읽어오도록 강제
            asm volatile("" ::: "memory");
            #if defined(__x86_64__)
            __builtin_ia32_pause();
            #endif
            current_head = q->head;
        }

        // 안전 구역 확보 완료: 비트 마스킹 물리 인덱스 추출
        uint64_t index = current_tail & SharedRingBufferQueue::MASK;

        // Zero-copy 직접 쓰기
        q->buffer[index].timestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        std::strncpy(q->buffer[index].symbol, "KR7005930003", 12);
        q->buffer[index].price = 72000 + (i % 100);
        q->buffer[index].quantity = i;

        // [컴파일러 메모리 장벽]
        // "데이터 다 쓰기 전에는 절대로 아래의 tail 전진 명령을 먼저 수행하지 마라"고 컴파일러에게 지시
        asm volatile("" ::: "memory");
        current_tail++;
        q->tail = current_tail; // 8바이트 정렬 변수이므로 하드웨어가 통째로 단 한 번에 씀
    }

    std::cout << "[Producer] 1,000,000개 데이터 이송 완료.\n";
    munmap(ptr, sizeof(SharedRingBufferQueue));
    close(shm_fd);
    return 0;
}
