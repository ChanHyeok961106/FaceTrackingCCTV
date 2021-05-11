#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv/cvaux.h>
#include <opencv/highgui.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include "serialcomm.h"
// �ø��� ��Ʈ ���� ���̺귯���� Kocoafab ���� ������ �� �Ƶ��̳� �ӵ� ������
// ���۹���Ʈ����� ���� ������ �Ϲ� �ܼ� ����Ʈ ������� ������

using namespace cv;
using namespace std;

int face_detect(int, char**); // �ǽ� ������ �� ã�� �Լ�
void fdad(IplImage *, CSerialComm serialComm); // �ǽ� ������ �� ��Ŭ�׷��ִ� �Լ� + ����Ʈ �ø��� ��� �Լ�
void connect_Ard(int, CSerialComm serialComm); // �Ƶ��̳뿡 ����Ʈ�� �����ִ� �Լ�
// �� �� �߾� 3���� ������ �۽�

static CvMemStorage *storage = 0; // �޸� ���丮�� �Ҵ� ����
static CvHaarClassifierCascade *cascade = 0; // �� ������ xml ���� ����� ����

int main(int argc, char *argv[])
{
	face_detect(argc, argv);
}

int face_detect(int argc, char **argv) {
	CvCapture *capture = 0;
	IplImage *frame; // ���� ��� â �̹���

	int optlen = strlen("cascade -> ");
	const char *input_name;

	const char *XML_name; // XML ���� (�� ������) ���� �� ���� �ӽ� ����

	CSerialComm arduino_Port; // �Ƶ��̳� �ø��� ��Ʈ�� serialComm ��ü


	if (!arduino_Port.connect("COM3")) {  // �Ƶ��̳� ��Ʈ Ȯ�� �� ��ǻ���� ��� COM3 ��Ʈ
		cout << "���� ����!" << endl; // ���н� -1 ���� �� �Լ� Ż��
		return -1;
	}
	else { cout << "���� ����!" << endl; } // ���� Ȯ��



	if (argc > 1 && strncmp(argv[1], "cascade -> ", optlen) == 0) {
		XML_name = argv[1] + optlen;
		input_name = argc > 2 ? argv[2] : 0;
	}
	else {
		XML_name = "C:/data/haarcascades/haarcascade_frontalface_alt2.xml"; // xml ���� �̸�
		input_name = argc > 1 ? argv[1] : 0;
	}


	if (!(cascade = (CvHaarClassifierCascade *)cvLoad(XML_name, 0, 0, 0))) { // XML (�ǽ��ð��� ���� xml����) ���� �ε�
		fprintf(stderr, "ERROR : couldn't load classfier cascade\n");
		fprintf(stderr, "Usage: facedetect same\n");
		return -1;
	}

	storage = cvCreateMemStorage(0); // �޸� ���丮�� ����ϱ� ����

	if (!input_name || (isdigit(input_name[0]) && input_name[1] == '\0'))
		capture = cvCaptureFromCAM(!input_name ? 0 : input_name[0] - '\0');
	else
		capture = cvCaptureFromCAM(0);

	cvNamedWindow("chanhyeok_Project", 1); // �Ϲ� ���� �̹��� â �̸�

	while (1) {
		if (!(frame = cvQueryFrame(capture))) // ���� ĸ��
			break;

		fdad(frame, arduino_Port); // ���� ������(�����̹���), �Ƶ��̳� �ø��� ��Ʈ
		if(cvWaitKey(10) >= 0)
		break;
	}

	cvReleaseCapture(&capture);
	cvDestroyWindow("CUT_Image_chanhyeok"); // �� ȭ�� �ı�
	cvDestroyWindow("chanhyeok_Project"); // �Ϲ� �����̹��� �ı�
	return 0;
}

void fdad(IplImage *img, CSerialComm arduino_port) {

	static CvScalar colors[] = {{{0,0,255}}, {{0,128,255}}, {{0,255,255}}, {{0,255,0}},{{255,128,0}}, {{255,255,0}}, {{255,0,0}}, {{255,0,255}}};
	double scale = 2.0;
	IplImage *gray = cvCreateImage(cvSize(img->width, img->height), 8, 1); // ��鿵��
	IplImage *small_img = cvCreateImage(cvSize(cvRound(img->width/scale), cvRound(img->height / scale)), 8, 1);
	IplImage *copy; // img �̹����� �ӽ÷� ������ �� �� ũ�� ������� ���� �� ��µ� �̹���

	cvCvtColor(img, gray, CV_BGR2GRAY); // �� ã������ ���
	cvResize(gray, small_img, CV_INTER_LINEAR); // �� ã��� �̹��� ��������
	cvEqualizeHist(small_img, small_img); // �̹��� ��������¡ �� ã�� ����
	cvClearMemStorage(storage); // �޸� CLEAR �Ҵ� �����ƴ�!


	if (cascade) { // �� ����� ����
		double t = (double)cvGetTickCount();
		CvSeq *faces = cvHaarDetectObjects(small_img, cascade, storage, 1.1, 2, 0, cvSize(30, 30), cvSize(300, 300));
		t = (double)cvGetTickCount() - t;
		printf("Detection time : %gms\n", t / ((double)cvGetTickFrequency() * 1000));
		for (int i = 0; i < (faces ? faces->total : 0); i++) {
			CvRect *r = (CvRect *)cvGetSeqElem(faces, i);
			CvPoint center;
			center.x = cvRound((r->x + r->width*0.5)*scale); // X ��ǥ
			center.y = cvRound((r->y + r->height*0.5)*scale); // Y ��ǥ


			connect_Ard(center.x, arduino_port); // �� �߾� X ��ǥ���� �ø��� ��Ʈ �Ƶ��̳� ��� �Լ���
			int radius = cvRound((r->width + r->height)*0.25*scale); // ��ũ�� ������

			cvCircle(img, center, radius, colors[i % 8], 3, 8, 0); // �� ũ�� ��ŭ ��Ŭ ����

			copy = cvCreateImage(cvSize(radius * 2, radius * 2), IPL_DEPTH_16U, 3); // �� ũ�� ������� �̹��� ����
			// IPL_DEPTH_16U == 16��Ʈ unsigned integer | 3 == R,G,B
			copy = cvCloneImage(img); // RGB �̹��� Ŭ�� ����, ����� ��� cvCopy ���
			Rect temp_copy = cvRect(center.x-radius, center.y-radius, radius * 2, radius * 2); // �� ������ �簢�� ����
			cvSetImageROI(copy, temp_copy); // �� �̹��� ROI

			cvShowImage("CUT_Image_chanhyeok", copy); // �� ����� �� ȭ�� �̹��� â
		}

	}
	cvShowImage("chanhyeok_Project", img);
	cvReleaseImage(&gray);
	cvReleaseImage(&small_img);

	return;
}

void connect_Ard(int temp, CSerialComm arduino_port) { // temp == �� ȭ�鿡�� X ��ǥ, arduino_port == �ø�����Ʈ

	printf("Original Value : %d\n", temp); // �� X ��ǥ ��
	
	if (temp > 260 && temp < 360) { // ȭ�� �߾ӿ� ������ ������ ��
		if (!arduino_port.sendCommand('S')) { cout << "STAY ���� ����" << endl; }
		else { cout << "STAY command success" << endl; }
	}
	else if(temp >= 360) { // ���� �������� Ȯ�� �Ǿ��� ��� ��������
		if (!arduino_port.sendCommand('L')) { cout << "LEFT ���� ����" << endl; }
		else { cout << "LEFT command success" << endl; }
	}
	else if(temp <= 260) { // ���� �������� Ȯ�� �Ǿ��� ��� ��������
		if (!arduino_port.sendCommand('R')) { cout << "RIGHT ���� ����" << endl; }
		else { cout << "RIGHT command success" << endl; }
	}
	else { // �� ��ġ�� �����Ⱚ���� ���� �ȴٸ� �׳� ������ �ֵ��� �۽�
		if (!arduino_port.sendCommand('S')) { cout << "TRASH ����" << endl; }
		else { cout << "TRASH command success" << endl; }
	}
	
	Sleep(10); // ��ǻ�� �ӵ��� �Ƶ��̳밡 ������ ���� �Ƶ��̳뿡 ���۰� �׿� 0.01s �� ���� ������
	
}