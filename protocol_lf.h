// queue_defs.h
#ifndef QUEUE_DEFS_H
#define QUEUE_DEFS_H

#include <cstdint>

#pragma pack(push, 1)

// 큐에 담길 원시 데이터 구조체 (주문/시세 패킷)
struct alignas(64) MarketData {
    long long timestamp ;    // 나노초 단위 타임스탬프
    char      symbol[12];    // 종목 코드 (예: "KR7005930003")
    int       price     ;    // 현재가
    int       quantity ;     // 잔량
};

// 공유메모리에 매핑할 초경량 락프리 링버퍼 구조체 (std::atomic 배제)
struct SharedRingBufferQueue {
    // [제어 영역] 컴파일러 오버헤드 없이 volatile 변수와 64바이트 캐시라인 정렬만 사용
    alignas(64) volatile uint64_t tail; // 생산자가 쓰는 가상 카운터
    alignas(64) volatile uint64_t head; // 소비자가 읽는 가상 카운터

    alignas(64) volatile uint64_t consumer_ready;

    // [데이터 영역] 배열 크기는 2의 거듭제곱으로 고정
    static constexpr uint64_t CAPACITY = 2048;
    static constexpr uint64_t MASK = CAPACITY - 1; // 2047 (0x7FF)
    
    MarketData buffer[CAPACITY];
};

#pragma pack(pop)

// 공유메모리 파일 시스템 고유 식별 명칭
const char* SHM_NAME = "/shm_ultra_fast_market_queue";

#endif // QUEUE_DEFS_H
