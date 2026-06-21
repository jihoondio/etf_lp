// shm_protocol.h
#ifndef SHM_PROTOCOL_H
#define SHM_PROTOCOL_H

#include <pthread.h>

#define SHM_NAME "/etf_lp_shm"

#pragma pack(push, 1)

// 1. 공유메모리에 올릴 데이터 구조체 (포인터, 가상 함수 절대 금지)
struct MarketData {
    int       code_id   ;      // 종목 코드 인덱스
    char      symbol[12];      // 종목명 (std::string 대신 고정 크기 char 배열)
    long long price     ;      // 현재가
    int       volume    ;      // 거래량
};

// 2. 공유메모리 전체 레이아웃 (동기화 객체 + 데이터)
struct SharedMemoryLayout {
    pthread_mutex_t mutex;       // 프로세스 간 공유할 뮤텍스
    pthread_cond_t  cond;        // 프로세스 간 공유할 조건 변수
    MarketData      data;        // 실제 전송할 데이터
    bool            is_ready;    // 데이터 준비 플래그
    unsigned int    consumer_ready;
};

#pragma pack(pop)

#endif // SHM_PROTOCOL_H
