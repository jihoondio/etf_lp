// consumer.cpp
#include <iostream>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include "protocol_lf.h"

void process_order_logic(const MarketData& data) {
    if (data.price > 72050) {
        // 공유메모리 주소의 필드를 직접 찔러 복사 오버헤드 완벽 차단
    }
}

int main() {
    int shm_fd = -1;
    std::cout << "[Consumer] 공유메모리 대기 중... (PID: " << getpid() << ")\n";
    while ((shm_fd = shm_open(SHM_NAME, O_RDWR, 0666)) < 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    void* ptr = mmap(nullptr, sizeof(SharedRingBufferQueue), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    auto* q = static_cast<SharedRingBufferQueue*>(ptr);

    // 🚀 생산자의 대기 빗장을 풀어주는 코드
    asm volatile("" ::: "memory"); // 순서 보장 장벽
    q->consumer_ready = 1;         // 플래그를 1로 셋팅!

    std::cout << "[Consumer] 공유메모리 결합 성공. 무복사 제로 카피 수신 시작.\n";

    uint64_t current_head = 0;
    uint64_t total_processed = 0;

    while (total_processed < 1000000) {
        uint64_t current_tail = q->tail;

        // [Empty 판별] 생산자 뺄셈식과 대칭 구조로 실시간 가용 패킷 개수 확보
        uint64_t available_data = current_tail - current_head;

        // 데이터가 없으면 대기
        if (available_data == 0) {
            asm volatile("" ::: "memory"); // 수동 컴파일러 장벽 투입
#if defined(__x86_64__)
            __builtin_ia32_pause();
#endif
            continue;
        }

        // [배치 최적화 및 Zero-copy 실행]
        // 생산자는 Full 장벽 때문에 소비자가 확보한 이 영역을 절대 침범하지 못합니다.
        for (size_t i = 0; i < available_data; ++i) {
            uint64_t index = (current_head + i) & SharedRingBufferQueue::MASK;

            // 통짜 데이터 복사(=) 행위 없이 상수 참조자로 다이렉트 주소 매칭
            const MarketData& data = q->buffer[index];

            std::cout << "[Consumer] data = " << data.price << std::endl;
            //process_order_logic(data);
        }

        // 루프가 끝난 뒤 한방에 head 카운터 도약
        current_head += available_data;
        total_processed += available_data;

        // 데이터 처리가 완벽히 끝난 후 공간을 반납하도록 명령어 순서 잠금장치 설정
        asm volatile("" ::: "memory");
        q->head = current_head;
    }

    std::cout << "[Consumer] 총 " << total_processed << "개 패킷 수신 및 연산 종료.\n";

    munmap(ptr, sizeof(SharedRingBufferQueue));
    close(shm_fd);
    shm_unlink(SHM_NAME); // 커널 내 공유메모리 자원 파괴
    return 0;
}