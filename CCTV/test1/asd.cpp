#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv/cvaux.h>
#include <opencv/highgui.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include "serialcomm.h"
// 시리얼 포트 설정 라이브러리는 Kocoafab 에서 참조한 후 아두이노 속도 문제로
// 버퍼바이트통신을 버퍼 문제로 일반 단순 바이트 통신으로 개조함

using namespace cv;
using namespace std;

int face_detect(int, char**); // 실습 참조한 얼굴 찾는 함수
void fdad(IplImage *, CSerialComm serialComm); // 실습 참조한 얼굴 서클그려주는 함수 + 바이트 시리얼 통신 함수
void connect_Ard(int, CSerialComm serialComm); // 아두이노에 바이트값 보내주는 함수
// 좌 우 중앙 3가지 종류로 송신

static CvMemStorage *storage = 0; // 메모리 스토리지 할당 변수
static CvHaarClassifierCascade *cascade = 0; // 얼굴 데이터 xml 파일 저장될 변수

int main(int argc, char *argv[])
{
	face_detect(argc, argv);
}

int face_detect(int argc, char **argv) {
	CvCapture *capture = 0;
	IplImage *frame; // 메인 출력 창 이미지

	int optlen = strlen("cascade -> ");
	const char *input_name;

	const char *XML_name; // XML 파일 (얼굴 데이터) 파일 명 저장 임시 변수

	CSerialComm arduino_Port; // 아두이노 시리얼 포트용 serialComm 객체


	if (!arduino_Port.connect("COM3")) {  // 아두이노 포트 확인 제 컴퓨터의 경우 COM3 포트
		cout << "연결 실패!" << endl; // 실패시 -1 리턴 후 함수 탈출
		return -1;
	}
	else { cout << "연결 성공!" << endl; } // 연결 확인



	if (argc > 1 && strncmp(argv[1], "cascade -> ", optlen) == 0) {
		XML_name = argv[1] + optlen;
		input_name = argc > 2 ? argv[2] : 0;
	}
	else {
		XML_name = "C:/data/haarcascades/haarcascade_frontalface_alt2.xml"; // xml 파일 이름
		input_name = argc > 1 ? argv[1] : 0;
	}


	if (!(cascade = (CvHaarClassifierCascade *)cvLoad(XML_name, 0, 0, 0))) { // XML (실습시간에 받은 xml파일) 파일 로드
		fprintf(stderr, "ERROR : couldn't load classfier cascade\n");
		fprintf(stderr, "Usage: facedetect same\n");
		return -1;
	}

	storage = cvCreateMemStorage(0); // 메모리 스토리지 사용하기 위해

	if (!input_name || (isdigit(input_name[0]) && input_name[1] == '\0'))
		capture = cvCaptureFromCAM(!input_name ? 0 : input_name[0] - '\0');
	else
		capture = cvCaptureFromCAM(0);

	cvNamedWindow("chanhyeok_Project", 1); // 일반 영상 이미지 창 이름

	while (1) {
		if (!(frame = cvQueryFrame(capture))) // 순간 캡쳐
			break;

		fdad(frame, arduino_Port); // 순간 프레임(영상이미지), 아두이노 시리얼 포트
		if(cvWaitKey(10) >= 0)
		break;
	}

	cvReleaseCapture(&capture);
	cvDestroyWindow("CUT_Image_chanhyeok"); // 얼굴 화면 파괴
	cvDestroyWindow("chanhyeok_Project"); // 일반 영상이미지 파괴
	return 0;
}

void fdad(IplImage *img, CSerialComm arduino_port) {

	static CvScalar colors[] = {{{0,0,255}}, {{0,128,255}}, {{0,255,255}}, {{0,255,0}},{{255,128,0}}, {{255,255,0}}, {{255,0,0}}, {{255,0,255}}};
	double scale = 2.0;
	IplImage *gray = cvCreateImage(cvSize(img->width, img->height), 8, 1); // 흑백영상
	IplImage *small_img = cvCreateImage(cvSize(cvRound(img->width/scale), cvRound(img->height / scale)), 8, 1);
	IplImage *copy; // img 이미지를 임시로 저장한 후 얼굴 크기 사이즈로 수정 후 출력될 이미지

	cvCvtColor(img, gray, CV_BGR2GRAY); // 얼굴 찾기위해 흑백
	cvResize(gray, small_img, CV_INTER_LINEAR); // 얼굴 찾기용 이미지 리사이즈
	cvEqualizeHist(small_img, small_img); // 이미지 이퀄라이징 얼굴 찾기 도움
	cvClearMemStorage(storage); // 메모리 CLEAR 할당 해제아님!


	if (cascade) { // 얼굴 검출시 진입
		double t = (double)cvGetTickCount();
		CvSeq *faces = cvHaarDetectObjects(small_img, cascade, storage, 1.1, 2, 0, cvSize(30, 30), cvSize(300, 300));
		t = (double)cvGetTickCount() - t;
		printf("Detection time : %gms\n", t / ((double)cvGetTickFrequency() * 1000));
		for (int i = 0; i < (faces ? faces->total : 0); i++) {
			CvRect *r = (CvRect *)cvGetSeqElem(faces, i);
			CvPoint center;
			center.x = cvRound((r->x + r->width*0.5)*scale); // X 좌표
			center.y = cvRound((r->y + r->height*0.5)*scale); // Y 좌표


			connect_Ard(center.x, arduino_port); // 얼굴 중앙 X 좌표값과 시리얼 포트 아두이노 통신 함수로
			int radius = cvRound((r->width + r->height)*0.25*scale); // 얼굴크기 반지름

			cvCircle(img, center, radius, colors[i % 8], 3, 8, 0); // 얼굴 크기 만큼 서클 생성

			copy = cvCreateImage(cvSize(radius * 2, radius * 2), IPL_DEPTH_16U, 3); // 얼굴 크기 사이즈로 이미지 생성
			// IPL_DEPTH_16U == 16비트 unsigned integer | 3 == R,G,B
			copy = cvCloneImage(img); // RGB 이미지 클론 생성, 흑백의 경우 cvCopy 사용
			Rect temp_copy = cvRect(center.x-radius, center.y-radius, radius * 2, radius * 2); // 얼굴 사이즈 사각형 으로
			cvSetImageROI(copy, temp_copy); // 얼굴 이미지 ROI

			cvShowImage("CUT_Image_chanhyeok", copy); // 얼굴 검출시 얼굴 화면 이미지 창
		}

	}
	cvShowImage("chanhyeok_Project", img);
	cvReleaseImage(&gray);
	cvReleaseImage(&small_img);

	return;
}

void connect_Ard(int temp, CSerialComm arduino_port) { // temp == 얼굴 화면에서 X 좌표, arduino_port == 시리얼포트

	printf("Original Value : %d\n", temp); // 얼굴 X 좌표 값
	
	if (temp > 260 && temp < 360) { // 화면 중앙에 있을때 스테이 값
		if (!arduino_port.sendCommand('S')) { cout << "STAY 전송 실패" << endl; }
		else { cout << "STAY command success" << endl; }
	}
	else if(temp >= 360) { // 얼굴이 우측에서 확인 되었을 경우 좌측으로
		if (!arduino_port.sendCommand('L')) { cout << "LEFT 전송 실패" << endl; }
		else { cout << "LEFT command success" << endl; }
	}
	else if(temp <= 260) { // 얼굴이 좌측에서 확인 되었을 경우 우측으로
		if (!arduino_port.sendCommand('R')) { cout << "RIGHT 전송 실패" << endl; }
		else { cout << "RIGHT command success" << endl; }
	}
	else { // 얼굴 위치가 쓰레기값으로 검출 된다면 그냥 가만히 있도록 송신
		if (!arduino_port.sendCommand('S')) { cout << "TRASH 실패" << endl; }
		else { cout << "TRASH command success" << endl; }
	}
	
	Sleep(10); // 컴퓨터 속도를 아두이노가 따라가지 못해 아두이노에 버퍼가 쌓여 0.01s 로 서로 맞춰줌
	
}