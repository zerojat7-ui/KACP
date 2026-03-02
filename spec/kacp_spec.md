# KACP — Kcode Artery Connection Protocol
## 외부 구현 가이드 v1.0

> KACP는 Kcode 생태계의 온톨로지 엔진과 통신하기 위한
> 바이너리 프로토콜입니다.
> 이 규약을 구현하면 어떤 언어에서도 온톨로지 엔진에 접속할 수 있습니다.

---

## 1. 기본 원칙

```
전송 방식:  TCP 바이너리 소켓
정렬 단위:  32바이트 (모든 블록)
헤더 크기:  64바이트 고정 (2블록)
포트:       7070 (단일 포트)
magic:      "KEXT" (외부 언어 전용)
```

---

## 2. 패킷 구조

```
┌──────────────────────────────┐
│   HEADER 블록1  (32바이트)    │  필수
│   HEADER 블록2  (32바이트)    │  필수
├──────────────────────────────┤
│   AUTH 블록     (32바이트)    │  KEXT 필수
├──────────────────────────────┤
│   데이터 블록   (32바이트×N)  │  가변
└──────────────────────────────┘

최소 패킷: 96바이트 (헤더 64 + AUTH 32)
```

---

## 3. 헤더 구조 (64바이트)

### 블록1 (32바이트) — 기본 정보

| 오프셋 | 크기 | 필드 | 설명 |
|--------|------|------|------|
| 0 | 4 | magic | "KEXT" 고정 |
| 4 | 1 | version | 프로토콜 버전 (현재 1) |
| 5 | 1 | flags | 비트 플래그 (아래 참조) |
| 6 | 1 | chunk_index | 청크 순서 번호 (0부터) |
| 7 | 1 | chunk_total | 전체 청크 수 |
| 8 | 2 | data_blocks | 데이터 블록 수 |
| 10 | 2 | ont_blocks | ONT 응답 블록 수 |
| 12 | 4 | scene_id | 씬 ID (없으면 0) |
| 16 | 4 | sequence | 패킷 순서 번호 |
| 20 | 4 | timestamp | Unix 타임스탬프 (초) |
| 24 | 8 | checksum | SHA-256 앞 8바이트 |

### 블록2 (32바이트) — 연결 + 권한

| 오프셋 | 크기 | 필드 | 설명 |
|--------|------|------|------|
| 32 | 16 | link_id | ONT 발급 연결고리 ID (신규: 0x00) |
| 48 | 1 | access_level | 0=비공개 / 1=지정 / 2=공개 / 0xFF=없음 |
| 49 | 1 | render_allow | 렌더링 허용 (0=불가 / 1=가능 / 0xFF=없음) |
| 50 | 1 | copy_allow | 복제 허용 (0=불가 / 1=가능 / 0xFF=없음) |
| 51 | 1 | learn_allow | 학습 허용 (0=불가 / 1=가능 / 0xFF=없음) |
| 52 | 12 | reserved | 0x00 으로 채움 |

---

## 4. flags 비트 정의

```
bit 0:    direction    0=요청 / 1=응답
bit 1:    transfer     0=참조 전송 / 1=청크 분할
bit 2:    is_last      1=마지막 청크
bit 3~4:  priority     00=일반 / 01=소유자 / 10=긴급
bit 5~7:  packet_type  000=쿼리 / 001=응답
                       010=푸시 / 011=ACK
```

---

## 5. AUTH 블록 (32바이트) — KEXT 필수

```
외부 언어(KEXT)는 반드시 헤더 뒤에
AUTH 블록을 추가해야 합니다.
```

| 오프셋 | 크기 | 필드 | 설명 |
|--------|------|------|------|
| 0 | 24 | auth_token | 발급받은 인증 토큰 |
| 24 | 1 | token_type | 0=API키 / 1=JWT |
| 25 | 1 | rate_limit | 요청 등급 (0=일반 / 1=프리미엄) |
| 26 | 6 | reserved | 0x00 으로 채움 |

---

## 6. 데이터 블록 구조

### 블록 타입 1 — 개념 식별 (32바이트)

```
객체나 개념을 식별할 때 사용
```

| 오프셋 | 크기 | 필드 | 설명 |
|--------|------|------|------|
| 0 | 32 | concept_name | 개념명 (UTF-8, null 패딩) |

### 블록 타입 2 — 속성 데이터 (32바이트)

| 오프셋 | 크기 | 필드 | 설명 |
|--------|------|------|------|
| 0 | 1 | attr_type | 속성 유형 |
| 1 | 1 | attr_count | 속성 개수 |
| 2 | 2 | reserved | 패딩 |
| 4 | 28 | attr_values | float × 7 (속성 값) |

### 블록 타입 3 — 공간 좌표 (32바이트)

| 오프셋 | 크기 | 필드 | 설명 |
|--------|------|------|------|
| 0 | 12 | xyz | float × 3 (공간 좌표) |
| 12 | 4 | zone_id | Zone ID |
| 16 | 1 | obj_type | 0=정적 / 1=동적 |
| 17 | 1 | channel_flag | 변화 채널 비트 플래그 |
| 18 | 14 | reserved | 패딩 |

### 블록 타입 4 — Delta 값 (32바이트)

```
변화량만 전송할 때 사용
Delta = 0 이면 전송하지 않음
```

| 오프셋 | 크기 | 필드 | 설명 |
|--------|------|------|------|
| 0 | 32 | delta | float × 8 (변화량) |

---

## 7. 전송 방식

### 신규 데이터 — 청크 분할

```
처음 등록하는 개념/객체:
    flags bit1 = 1 (청크 분할)
    chunk_total = 전체 청크 수
    chunk_index = 0, 1, 2 ... 순서대로

수신 측이 모든 청크 받은 후 재조립
마지막 청크: flags bit2 = 1 (is_last)
```

### 기존 데이터 — 참조 전송

```
이미 등록된 개념/객체 업데이트:
    flags bit1 = 0 (참조 전송)
    link_id 에 기존 발급값 입력
    Delta > 0 인 블록만 전송
    Delta = 0 이면 전송 안 함
```

---

## 8. link_id 발급 절차

### link_id 구조 (16바이트)

```
[오프셋 0~3]  4바이트  온톨로지 엔진 ID    발급 엔진 식별자
[오프셋 4~7]  4바이트  개념 노드 해시값    순차 번호 대신 해시 사용 (예측 방지)
[오프셋 8~11] 4바이트  발급 타임스탬프     Unix 타임스탬프 (초)
[오프셋 12~15]4바이트  HMAC 축약값        HMAC-SHA256 앞 4바이트
                                          온톨로지 엔진 비밀키 사용
                                          → 역산 불가 / 위조 방지
```

> 개념 노드 번호를 해시값으로 저장 — 순차 증가 방식 미사용
> 공격자가 다른 노드 번호 대입 탐색 불가

### 발급 흐름

```
1단계: 신규 등록 요청 (DNA 엔진 → 온톨로지 엔진)
    헤더 link_id = 0x00 (16바이트 전부 0)
    데이터 블록에 개념명 포함
    전송

2단계: 온톨로지 엔진 처리
    개념 노드 매핑
    link_id 16바이트 생성:
        엔진 ID + 노드 해시 + 타임스탬프 + HMAC 축약
    → DNA 엔진에 응답 패킷 전송

3단계: DNA 엔진 수신
    link_id 수신
    → .edna PASSPORT 블록에 저장
    → 이후 모든 패킷에 포함

4단계: 수신 시 재검증 (온톨로지 엔진)
    link_id 수신마다 HMAC 축약값 재계산
    불일치 → 즉시 폐기 + 0x05 checksum 오류 반환
    일치   → 정상 처리
```

### 보안 등급별 TLS 적용

```
내부 엔진 간 (KDNA / KLNG / KONT):
    internal_tls = 0 (개발 환경 — 속도 우선)
    internal_tls = 1 (운영 환경 — 권장)
    → TLS off 시 HMAC 체크섬이 1차 방어선

외부 통신 (KEXT):
    external_tls = 1 (항상 강제)
    → 절대 해제 불가
```

---

## 9. 인증 토큰 발급

```
온톨로지 엔진 관리자에게 요청:
    API키 발급 → token_type = 0
    JWT 발급   → token_type = 1

AUTH 블록 auth_token 필드에 삽입
모든 KEXT 패킷에 반드시 포함
```

---

## 10. checksum 계산

### 패킷 checksum (헤더 오프셋 24)

```
계산 대상:
    헤더 블록1 (오프셋 0~23, 24바이트)
    + AUTH 블록 (32바이트)
    + 모든 데이터 블록

계산 방법:
    SHA-256 해시 계산
    → 앞 8바이트만 사용
    → 헤더 오프셋 24에 삽입

검증:
    수신 측이 동일하게 계산 후 비교
    불일치 시 패킷 폐기 + 0x05 오류 반환
```

### link_id 내부 HMAC (오프셋 12~15)

```
계산 대상:
    link_id 오프셋 0~11 (12바이트)
    엔진 ID + 노드 해시 + 타임스탬프

계산 방법:
    HMAC-SHA256 (온톨로지 엔진 비밀키)
    → 앞 4바이트만 사용
    → link_id 오프셋 12~15 에 삽입

목적:
    패킷 checksum 과 별개
    link_id 자체 위조 방지
    온톨로지 엔진 비밀키 없이는 생성 불가
    internal_tls = 0 환경에서도 위조 방어

검증:
    온톨로지 엔진이 link_id 수신 시마다 재계산
    불일치 → 위조 패킷으로 즉시 폐기
```

---

## 11. 패킷 예시 (개념 조회)

```
목표: "온도센서" 개념 조회

헤더 블록1:
    magic        = "KEXT"
    version      = 1
    flags        = 0b00000000  (요청/참조/일반/쿼리)
    chunk_index  = 0
    chunk_total  = 1
    data_blocks  = 1
    ont_blocks   = 0
    scene_id     = 0
    sequence     = 1
    timestamp    = [현재 Unix 시간]
    checksum     = [계산값]

헤더 블록2:
    link_id      = [발급받은 link_id]
    access_level = 0xFF  (없음)
    나머지       = 0xFF

AUTH 블록:
    auth_token   = [발급받은 토큰]
    token_type   = 0  (API키)
    rate_limit   = 0  (일반)
    reserved     = 0x00

데이터 블록1 (개념 식별):
    concept_name = "온도센서\0\0\0..."  (32바이트)
```

---

## 12. 응답 처리

```
온톨로지 엔진 응답:
    헤더 flags bit0 = 1  (응답)
    ont_blocks = 응답 블록 수

응답 블록 타입:
    ONT_BLOCK_CONCEPT  개념 노드 정보
    ONT_BLOCK_RESULT   제약/권고 결과
    ONT_BLOCK_ERROR    오류 정보
```

### 오류 코드

| 코드 | 설명 |
|------|------|
| 0x00 | 성공 |
| 0x01 | 인증 실패 |
| 0x02 | 권한 없음 |
| 0x03 | Rate Limit 초과 |
| 0x04 | 개념 없음 |
| 0x05 | checksum 오류 |
| 0x06 | 잘못된 블록 구조 |
| 0xFF | 서버 오류 |

---

## 13. 구현 체크리스트

```
□ magic[4] = "KEXT" 설정
□ KACPHeader 64바이트 빅엔디안/리틀엔디안 확인
□ AUTH 블록 항상 포함
□ checksum SHA-256 앞 8바이트 계산
□ link_id 발급 절차 구현
□ 청크 분할 / 참조 전송 분기
□ Delta = 0 전송 차단
□ 응답 오류 코드 처리
□ TCP 연결 유지 / 재연결 처리
```

---

## 15. 보완 설계

### 15-1. link_id 생명주기 관리 (GC)

```
현재 문제:
    세션 종료 / 객체 소멸 시
    link_id 회수 규칙 없음
    → 메모리 누수 발생 가능

해결 — Lease Time (임대 시간):
    link_id 발급 시 유효 시간 설정
    일정 시간 KACP 패킷 없으면 만료
    만료된 ID → 메모리 반환

재연결 로직:
    클라이언트가 만료된 link_id 전송 시
    0x04 (개념 없음) 에러 대신
    응답 헤더에 Re-registration 플래그 포함
    → 클라이언트가 자동으로 재등록 절차 수행
```

```c
#define KACP_LINK_LEASE_DEFAULT  300    // 기본 임대 시간 (초)
#define KACP_FLAG_REREG          0x80   // 재등록 유도 플래그 (flags 예비 비트)
```

### 15-2. Artery Scheduling (우선순위 큐)

```
문제:
    포트 7070 단일 포트
    KDNA (대용량 실시간) + KLNG (긴급 명령) 동시 수신
    → 병목 발생 시 긴급 명령 지연

해결 — 수신 큐 3단계:

    우선순위 2 (긴급):
        KLNG 긴급 명령 (렌더링 중지 등)
        ROI 객체 E-Channel
        즉시 처리

    우선순위 1 (소유자):
        소유자 권한 패킷
        일반보다 먼저 처리

    우선순위 0 (일반):
        KDNA 데이터 스트림
        대역폭 여유 시 처리

동작:
    수신 시 flags bit3~4 확인
    → 우선순위별 큐에 삽입
    → 높은 우선순위부터 처리
```

### 15-3. KGET/KPST Strict Mode

```
개발 환경 (strict_mode = false):
    KGET/KPST 일반 Rate Limit 적용
    하위 호환 허용

운영 환경 (strict_mode = true):
    KGET/KPST Rate Limit 엄격 적용
    보안 등급 HIGH 데이터
    → KEXT (TLS) 로만 접근 가능
    → KGET/KPST 요청 시 0x02 (권한 없음) 반환

설정:
    kacp.conf 에서 strict_mode 전환
    데이터별 보안 등급 설정 가능
```

### 15-4. 가변 길이 쿼리 청크 처리

```
쿼리가 32바이트 초과 시:

    flags bit1 = 1 (청크 분할)
    chunk_total = 전체 청크 수
    chunk_index = 0, 1, 2 ... 순서

    마지막 청크:
        flags bit2 = 1 (is_last)
        남은 공간 0x00 패딩

수신 측:
    is_last 수신 전까지 버퍼에 누적
    is_last 수신 후 재조립 → 실행
    순서 어긋나면 sequence 기준 정렬 후 처리
```

---

## 16. TLS 보안 설정

### 15-1. TLS 분리 설정 원칙

```
internal_tls  내부 엔진 (KDNA/KLNG/KONT)
              개발 환경: off 가능
              운영 환경: on 권장

external_tls  외부 언어 (KEXT)
              항상 on (강제)
              off 불가 — 외부 침입 방어
```

### 15-2. 설정 구조

```c
typedef struct {
    uint8_t  internal_tls;     // 0=off / 1=on
                               // 개발 시 off 가능
    uint8_t  external_tls;     // 0=off / 1=on
                               // 항상 1 강제
                               // 0 설정 시 경고 + 강제 종료
    char     cert_path[256];   // 인증서 경로
    char     key_path[256];    // 개인키 경로
    char     ca_path[256];     // CA 인증서
    uint32_t tls_timeout_ms;   // 핸드셰이크 타임아웃
    uint8_t  verify_peer;      // 상대방 인증서 검증
                               // 0=off / 1=on
} KACPTLSConfig;
```

### 15-3. 환경별 동작

```
운영 환경:
    internal_tls = 1   모든 엔진 암호화
    external_tls = 1   외부 암호화
    → 전체 암호화 / 바이러스 차단

개발 환경:
    internal_tls = 0   내부 빠른 개발
    external_tls = 1   외부 항상 암호화
    → 내부 속도 유지
    → 외부 보안 유지

external_tls = 0 시도 시:
    → "KEXT TLS는 해제할 수 없습니다" 경고
    → 서버 강제 종료
    → 보안 절대 타협 없음
```

### 15-4. TLS 오버헤드

```
최초 연결 핸드셰이크:
    50~100ms (1회만 발생)

이후 통신:
    AES-NI 지원 CPU 기준
    패킷당 ~0.01ms 미만
    → 네트워크 지연(10~50ms) 대비 무시 가능
    → 사실상 속도 차이 없음
```

### 15-5. checksum + TLS 조합

```
TLS:
    전송 중 변조 / 도청 차단

checksum (AUTH 블록 포함):
    패킷 구조 무결성 확인
    손상 데이터 감지

sequence 검증:
    패킷 손실 감지
    순서 이상 감지

timestamp 윈도우:
    재전송 공격 차단
    허용 범위 ±30초

→ 4중 보안 구조
```

```
엔디안:
    모든 다중 바이트 정수 → 리틀엔디안
    float → IEEE 754 단정도

문자열:
    UTF-8 인코딩
    남은 공간 0x00 으로 패딩
    null 종료 불필요

정렬:
    모든 블록 32바이트 단위
    패딩 필드는 0x00 으로 채움
```

---

*KACP — Kcode Artery Connection Protocol Spec v1.3*
*외부 구현 가이드 / 내부 엔진 전용 magic: KDNA / KLNG / KONT*
*외부 언어 전용 magic: KEXT / external_tls 항상 강제*
