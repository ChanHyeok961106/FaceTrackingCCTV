#include <Servo.h>  // 서보모터 라이브러리
Servo servo_1; // 쓸 서보 모터 선언

int pos= 90; // 서보모터 중앙값
char buffer; // 시리얼 포트로 들어올 데이터 (바이트)     

void setup() {
  servo_1.attach(9); // 디지털 9번 핀에 PWM제어 핀 설정
  Serial.begin(9600); // 9600 포트레이트 오픈
  Serial.flush();             
}

void loop() {
  if (Serial.available() > 0) { // 데이터가 들어옴

       buffer = Serial.read();
       // 아두이노 의 경우 속도(연산)가 매우 느려 PC에서 연산 후 출력값만 입력 받아옴
       if (buffer == 'R') {
        pos += 1; // 각도 증가
        servo_1.write(pos); // 각도값 서보
        Serial.print(pos); // 체크용
       }
       else if(buffer == 'L'){
        pos -= 1; // 각도 감소
        servo_1.write(pos); // 각도값 서보
        Serial.print(pos); // 체크용
       }
       else { // PC 에서는 정지 상태 및 쓰레기 값이 들어와도 'S'를 보냈지만 아두이노경우 상관없어서 else 처리
        servo_1.write(pos); // 각도 그대로
        Serial.print(pos); // 체크용
       }
       
       buffer = NULL; // 버퍼 (여기선 바이트 데이터) 초기화
       delay(10); // 아두이노가 PC에서 들어오는 데이터양(속도)을 감당을 하지 못해 0.01s로 딜레이를 통일시켜 통신
  }
  else { Serial.print("not come");} // 데이터가 안 들어옴
}
