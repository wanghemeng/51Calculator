/**************************************************************************************
	*			              Calculator				   						  	 *
	*																				 *
	*   	  Jp8->JP5   	Jp9->Jp4	insert 1602 display			P2^4->J7         *
	*		  J27->J28		J32->J34	P2^0->RC	P2^1->SC		P2^2->SE         *
***************************************************************************************/

#include <reg51.h> //���ļ��ж�����51��һЩ���⹦�ܼĴ���
#include "stdio.h"
#include <intrins.h>
#include <math.h>
#include "LED.h"

#define uchar unsigned char
#define uint unsigned int
#define MAX_LEN 10
#define XCH_LEFT                                             \
	{                                                        \
		flash[stack_idx] = state;                            \
		operator[stack_idx + 1][0] = operator[stack_idx][0]; \
		operator[stack_idx + 1][1] = operator[stack_idx][1]; \
		operand[stack_idx + 1][0] = operand[stack_idx][0];   \
		operand[stack_idx + 1][1] = operand[stack_idx][1];   \
		operand[stack_idx + 1][2] = operand[stack_idx][2];   \
		stack_idx++;                                         \
	}

#define XCH_RIGHT                                              \
	{                                                          \
		if (state == 3 || state == 4 || state == 5)            \
			operand[stack_idx - 1][1] = operand[stack_idx][0]; \
		else if (state == 6 || state == 7)                     \
			operand[stack_idx - 1][2] = operand[stack_idx][0]; \
		else if (state == 1)                                   \
		{                                                      \
			state = 2;                                         \
			operand[stack_idx - 1][0] = operand[stack_idx][0]; \
		}                                                      \
		stack_idx--;                                           \
	}

#define BEF_NOT_OPERATOR (bef != '+' && bef != '-' && bef != '*' && bef != '/' && bef != '=' && bef != '.')
#define NUM_STACK 10 //���ŵļ���

sbit EN = P2 ^ 7; //LCD��ʹ������
sbit RS = P2 ^ 6; //LCD��������ѡ���
sbit RW = P2 ^ 5; //LCD�Ķ�дѡ���
sbit K1 = P1 ^ 0;
sbit K2 = P1 ^ 1;
sbit K3 = P1 ^ 2;
sbit K4 = P1 ^ 3;
sbit K5 = P1 ^ 4;
sbit K6 = P1 ^ 5;
sbit K7 = P1 ^ 6;
sbit K8 = P1 ^ 7;
sbit BEEP = P2 ^ 4; //�������������� P2.4 �˿���

uchar KEY_CODE[] = {0xee, 0xde, 0xbe, 0x7e, //4X4������̼�ֵ��
					0xed, 0xdd, 0xbd, 0x7d,
					0xeb, 0xdb, 0xbb, 0x7b,
					0xe7, 0xd7, 0xb7, 0x77};
uchar i = 0;	  //ͳ�Ʋ�������λ��
uchar count = 0;  //������������LCD�Ļ������
uchar num = 0xff; //�������
uchar bef = 0xff; //��¼��һ��num��ֵ���ж��Ƿ�Ϊ����S���㷨�߼����
uchar state = 1;  //״̬��ʶ��,��ʾ12��״̬����״̬
uchar flash[NUM_STACK];
uchar operator[NUM_STACK][2];
double operand[NUM_STACK][3];
uchar stack_idx = 0;  //��ǰ�Ѷ���������
uchar QUEUE[MAX_LEN]; //����һ������洢��������ÿһλ
uchar LCD_CACHE[17];  //LCD��ĻCache
uchar f_clean = 0;	  //�Ƿ�������ʶλ���������ţ�

void scanf_mat();											//�Ӿ�������л�ȡֵ
void delay_short();											//����ʱ����
void delay_long();											//�ϳ���ʱ����
void writeCMD(uchar com);									//д�����ӳ���
void showData(uchar dat);									//д�����ӳ���
void init();												//��ʼ���ӳ��򣬳�ʼ��Һ����ʾ��
void clear();												//�����ʾ���ϵ���ʾ
void stateChange();											//״̬ת����������ʾ״̬ת��
void PushOperArray(uchar *arr, int i, int locationOperand); //������ת������
void firstCaculate();										//operand0=operand0+operand1;
void lastCaculate();										//operand1=operand2+operand1;
void beep();												//����������
void showAnswer();											//չʾ�������
void wirteCache();											//ѭ����Ļ����
void Hc595SendByte(u8 dat1, u8 dat2, u8 dat3, u8 dat4);		//LED��ʾ����

void main()
{
	// Timer0Init(); //��ʱ��0��ʼ��
	u8 j, k = 200;
	while (k-- > 0)
	{
		for (j = 0; j < 16; j++)
		{
			Hc595SendByte(~ledwei[j + 16], ~ledwei[j], ledmeng[16 + j], ledmeng[j]);
			delay(10);
		}
	}

	init();
	while (1)
	{
		bef = num;
		scanf_mat();
		stateChange(); //������ʵ��
	}
}

/**********״̬ת������**************/
void stateChange()
{
	switch (state)
	{
	case 1: //��ʼ״̬
		if (f_clean == 1)
		{
			clear();
			f_clean = 0;
		}

		if ((num >= '0' && num <= '9') && (i < MAX_LEN)) //����Ϸ���0��9���֣�����״̬2
		{
			state = 2;
			QUEUE[i] = num; //���ֽ�ջ
			showData(num);
			i++;
		}
		else if (num == '(')
		{
			XCH_LEFT;
			// operand1[0] = operand[stack_idx][0];
			// operand1[1] = operand[stack_idx][1];
			// operand1[2] = operand[stack_idx][2]; //����������ɣ���ʼ()�ڵ�����
			state = 1; //�ص����
			showData(num);
		}
		else
		{
			beep();
		}
		break;

	case 2: //A
		if ((num >= '0' && num <= '9') && (i < MAX_LEN) || num == '.')
		{
			state = 2;
			QUEUE[i] = num;
			showData(num);
			i++;
		}
		else if (num == '+' || num == '-') //����״̬3 A+
		{
			state = 3;
			PushOperArray(QUEUE, i, 0); //���ò�����������operand,��һ��������
			i = 0;
			operator[stack_idx][0] = num;
			showData(num);
		}
		else if (num == '*') //����״̬4  A*
		{
			state = 4;
			PushOperArray(QUEUE, i, 0); //���ò�����������operand,��һ��������
			i = 0;
			operator[stack_idx][0] = num;
			showData(num);
		}
		else if (num == '/') //����״̬5  A/
		{
			state = 5;
			PushOperArray(QUEUE, i, 0); //���ò�����������operand,��һ��������
			i = 0;
			operator[stack_idx][0] = num;
			showData(num);
		}
		else if (num == '=') //����״̬1  ִ��������=��
		{
			state = 1;
			PushOperArray(QUEUE, i, 0); //���ò�����������operand,��һ��������
			i = 0;
			showData(num);
			showAnswer();
			f_clean = 1;
		}
		else if (num == 'S')
		{
			PushOperArray(QUEUE, i, 0); //���ò�����������operand,��һ��������
			operand[stack_idx][0] = sqrt(operand[stack_idx][0]);
			showData(num);
			state = 2;
		}
		else
		{
			beep();
		}
		break;

	case 3: //A+
			//A+B
		if ((num >= '0' && num <= '9') && (i < MAX_LEN) || num == '.')
		{
			state = 3;
			QUEUE[i] = num;
			showData(num);
			i++;
		}
		else if (num == '+' || num == '-')
		{ //A+B+ ����ǰ��λ������
			if (BEF_NOT_OPERATOR)
			{
				//caculate();
				state = 3;
				PushOperArray(QUEUE, i, 1); //���ò�����������operand,�ڶ���������
				i = 0;
				firstCaculate();			  //�ڵ�һ��������+-����+-����֮ǰ������A+/-B�����ѽ�����ڵ�һ����������
				operator[stack_idx][0] = num; //���õ�һλ������
				showData(num);
			}
		}
		else if (num == '*') //A+B*
		{
			if (BEF_NOT_OPERATOR)
			{
				state = 6;
				PushOperArray(QUEUE, i, 1); //���ò�����������operand,�ڶ���������
				i = 0;
				operator[stack_idx][1] = num; //���õڶ�λ������
				showData(num);
			}
		}
		else if (num == '/') //����״̬7  A+B/
		{
			if (BEF_NOT_OPERATOR)
			{
				state = 7;
				PushOperArray(QUEUE, i, 1); //���ò�����������operand,�ڶ���������
				i = 0;
				operator[stack_idx][1] = num; //���õڶ�λ������
				showData(num);
			}
		}
		else if (num == '=') //A+B=
		{
			if (BEF_NOT_OPERATOR)
			{
				state = 1;
				PushOperArray(QUEUE, i, 1); //���ò�����������operand,�ڶ���������
				i = 0;
				showData(num);
				firstCaculate();
				showAnswer();
				f_clean = 1;
			}
		}
		else if (num == '(')
		{
			XCH_LEFT;
			state = 1; //�ص����
			showData(num);
		}
		else if (num == ')') //�黹���ݣ�д��ڶ������������ص�״̬flash
		{
			state = flash[stack_idx - 1];
			PushOperArray(QUEUE, i, 1); //���ò�����������operand,�ڶ���������
			i = 0;
			firstCaculate(); //ans is operand[stack_idx][0]����ʼ�黹����
			XCH_RIGHT;
			showData(num);
		}
		else if (num == 'S')
		{
			PushOperArray(QUEUE, i, 1); //���ò�����������operand,��һ��������
			operand[stack_idx][1] = sqrt(operand[stack_idx][1]);
			showData(num);
			state = 3;
		}
		else
		{
			beep();
		}
		break;

	case 4:															   //A*
		if ((num >= '0' && num <= '9') && (i < MAX_LEN) || num == '.') //A*B
		{
			state = 4;
			QUEUE[i] = num;
			showData(num);
			i++;
		}
		else if (num == '+' || num == '-') //A*B+ B������+��ʱ��ת�Ƶ�״̬3 A*B+ -->A+
		{
			if (BEF_NOT_OPERATOR)
			{
				state = 3;
				PushOperArray(QUEUE, i, 1); //���ò�����������operand,�ڶ���������
				i = 0;
				firstCaculate(); //�ڵ�һ��������*����+-����֮ǰ������A*B�����ѽ�����ڵ�һ����������
				operator[stack_idx][0] = num;
				showData(num);
			}
		}
		else if (num == '*')
		{ //A*B* -->A*
			if (BEF_NOT_OPERATOR)
			{
				state = 4;
				PushOperArray(QUEUE, i, 1); //���ò�����������operand,�ڶ���������
				i = 0;
				firstCaculate(); //�ڵ�һ��������*����*����֮ǰ������A*B�����ѽ�����ڵ�һ����������
				operator[stack_idx][0] = num;
				showData(num);
			}
		}
		else if (num == '/')
		{ //A*B/-->A/
			if (BEF_NOT_OPERATOR)
			{
				state = 5;
				PushOperArray(QUEUE, i, 1); //���ò�����������operand,�ڶ���������
				i = 0;
				firstCaculate(); //�ڵ�һ��������*����/����֮ǰ������A*B�����ѽ�����ڵ�һ����������
				operator[stack_idx][0] = num;
				showData(num);
			}
		}
		else if (num == '=') // A*B=
		{
			if (BEF_NOT_OPERATOR)
			{
				state = 1;
				PushOperArray(QUEUE, i, 1); //���ò�����������operand,�ڶ���������
				i = 0;
				showData(num);
				firstCaculate();
				showAnswer();
				f_clean = 1;
				//clear();
			}
		}
		else if (num == '(')
		{
			XCH_LEFT;
			state = 1;
			showData(num);
		}
		else if (num == ')') //�黹���ݣ�д��ڶ������������ص�״̬flash
		{
			state = flash[stack_idx - 1];
			PushOperArray(QUEUE, i, 1); //���ò�����������operand,�ڶ���������
			i = 0;
			firstCaculate(); //ans is operand[stack_idx][0]����ʼ�黹����
			XCH_RIGHT;
			showData(num);
		}
		else if (num == 'S')
		{
			PushOperArray(QUEUE, i, 1); //���ò�����������operand,��һ��������
			operand[stack_idx][1] = sqrt(operand[stack_idx][1]);
			showData(num);
			state = 4;
		}
		else
		{
			beep();
		}

		break;

	case 5:															   // A
		if ((num >= '0' && num <= '9') && (i < MAX_LEN) || num == '.') //����״̬5  ��/(��Ϊ0����)
		{
			state = 5;
			QUEUE[i] = num;
			showData(num);
			i++;
		}
		else if (num == '+' || num == '-')
		{ //  A/B+ --> A+
			if (BEF_NOT_OPERATOR)
			{
				state = 3;
				PushOperArray(QUEUE, i, 1); //���ò�����������operand,�ڶ���������
				i = 0;
				firstCaculate(); //�ڵ�һ��������/����+-����֮ǰ������A/B�����ѽ�����ڵ�һ����������
				operator[stack_idx][0] = num;
				showData(num);
			}
		}
		else if (num == '*')
		{ //����״̬4  A/B*-->A*
			if (BEF_NOT_OPERATOR)
			{
				state = 4;
				PushOperArray(QUEUE, i, 1); //���ò�����������operand,�ڶ���������
				i = 0;
				firstCaculate(); //�ڵ�һ��������/����*����֮ǰ������A/B�����ѽ�����ڵ�һ����������
				operator[stack_idx][0] = num;
				showData(num);
			}
		}
		else if (num == '/')
		{ //����״̬5  A/B/ --> A/
			if (BEF_NOT_OPERATOR)
			{
				state = 5;
				PushOperArray(QUEUE, i, 1); //���ò�����������operand,�ڶ���������
				i = 0;
				firstCaculate(); //�ڵ�һ��������/����/����֮ǰ������A/B�����ѽ�����ڵ�һ����������
				operator[stack_idx][0] = num;
				showData(num);
			}
		}
		else if (num == '=') //A/B = A/
		{
			if (BEF_NOT_OPERATOR)
			{
				state = 1;
				PushOperArray(QUEUE, i, 1); //���ò�����������operand,�ڶ���������
				i = 0;
				showData(num);
				firstCaculate();
				showAnswer();
				f_clean = 1;
			}
		}
		else if (num == '(')
		{
			XCH_LEFT;
			state = 1; //�ص����
			showData(num);
		}
		else if (num == ')') //�黹���ݣ�д��ڶ������������ص�״̬flash
		{
			state = flash[stack_idx - 1];
			PushOperArray(QUEUE, i, 1); //���ò�����������operand,�ڶ���������
			i = 0;
			firstCaculate(); //ans is operand[stack_idx][0]����ʼ�黹����
			XCH_RIGHT;
			showData(num);
		}
		else if (num == 'S')
		{
			PushOperArray(QUEUE, i, 1); //���ò�����������operand,��һ��������
			operand[stack_idx][1] = sqrt(operand[stack_idx][1]);
			showData(num);
			state = 5;
		}
		else
		{
			beep();
		}
		break;

	case 6:															   //״̬6--A+B* ��ΪҪ����B*����A+ ���Գ�����״̬6
		if ((num >= '0' && num <= '9') && (i < MAX_LEN) || num == '.') //����״̬11  ��+/-��*��
		{
			state = 6;
			QUEUE[i] = num;
			showData(num);
			i++;
		}
		else if (num == '+' || num == '-')
		{ //����״̬3  ��+/-��*��+/-=��+/-��+/-=��+/-��3+4*5+��
			if (BEF_NOT_OPERATOR)
			{
				state = 3;
				PushOperArray(QUEUE, i, 2); //���ò�����������operand,������������
				i = 0;
				lastCaculate();	 //����A+B*C�е�B*C������������ڵڶ�����������
				firstCaculate(); //����A+(B*C�Ľ��)������������ڵ�һ����������
				operator[stack_idx][0] = num;
				showData(num);
			}
		}
		else if (num == '*')
		{ //����״̬6  A+B*C*=A+B*
			if (BEF_NOT_OPERATOR)
			{
				state = 6;
				PushOperArray(QUEUE, i, 2); //���ò�����������operand,������������
				i = 0;
				lastCaculate();				  //����A+B*C�е�B*C������������ڵڶ�����������
				operator[stack_idx][1] = num; //A+-��B*C�Ľ�����к��*���ڵڶ�����������
				showData(num);
			}
		}
		else if (num == '/')
		{ //����״̬7  A+B*C/=A+B/
			if (BEF_NOT_OPERATOR)
			{
				state = 7;
				PushOperArray(QUEUE, i, 2); //���ò�����������operand,������������
				i = 0;
				lastCaculate();				  //����A+B*C�е�B*C������������ڵڶ�����������
				operator[stack_idx][1] = num; //A+-��B*C�Ľ�����к��/���ڵڶ�����������
				showData(num);
			}
		}
		else if (num == '=') //����״̬1  A+B*C= --> A+B=
		{
			if (BEF_NOT_OPERATOR)
			{
				state = 1;
				PushOperArray(QUEUE, i, 2); //���ò�����������operand,������������
				i = 0;
				showData(num);
				lastCaculate();
				firstCaculate();
				showAnswer();
				f_clean = 1;
			}
		}
		else if (num == '(')
		{
			XCH_LEFT;
			state = 1;
			showData(num);
		}
		else if (num == ')') //�黹���ݣ�д��ڶ������������ص�״̬3
		{
			state = flash[stack_idx - 1];
			PushOperArray(QUEUE, i, 1); //���ò�����������operand,�ڶ���������
			i = 0;
			lastCaculate();
			firstCaculate(); //ans is operand[stack_idx][0]����ʼ�黹����
			XCH_RIGHT;
			showData(num);
		}
		else if (num == 'S')
		{
			PushOperArray(QUEUE, i, 2); //���ò�����������operand,��һ��������
			operand[stack_idx][2] = sqrt(operand[stack_idx][2]);
			showData(num);
			state = 6;
		}
		else
		{
			beep();
		}
		break;

	case 7:															   //״̬7--A+B
		if ((num >= '0' && num <= '9') && (i < MAX_LEN) || num == '.') //�����ǳ������㣬��һλ��Ϊ��ſ��ԡ�
		{
			state = 7;
			QUEUE[i] = num;
			showData(num);
			i++;
		}
		else if (num == '+' || num == '-')
		{ //����״̬3  A+B/C+ -->A+
			if (BEF_NOT_OPERATOR)
			{
				state = 3;
				PushOperArray(QUEUE, i, 2); //���ò�����������operand,������������
				i = 0;
				lastCaculate();	 //����A+B/C�е�B/C������������ڵڶ�����������
				firstCaculate(); //����A+(B/C�Ľ��)������������ڵ�һ����������
				operator[stack_idx][0] = num;
				showData(num);
			}
		}
		else if (num == '*')
		{ //����״̬6  A+B/C* --> A+B*
			if (BEF_NOT_OPERATOR)
			{
				state = 6;
				PushOperArray(QUEUE, i, 2); //���ò�����������operand,������������
				i = 0;
				lastCaculate();				  //����A+B/C�е�B/C������������ڵڶ�����������
				operator[stack_idx][1] = num; //A+-��B/C�Ľ�����к��*���ڵڶ�����������
				showData(num);
			}
		}
		else if (num == '/')
		{ //����״̬7  A+B/C/ --> A+B/
			if (BEF_NOT_OPERATOR)
			{
				state = 7;
				PushOperArray(QUEUE, i, 2); //���ò�����������operand,������������
				i = 0;
				lastCaculate();				  //����A+B/C�е�B/C������������ڵڶ�����������
				operator[stack_idx][1] = num; //A+-��B/C�Ľ�����к��/���ڵڶ�����������
				showData(num);
			}
		}
		else if (num == '=') //����״̬1  A+B/C= --> A+B=
		{
			if (BEF_NOT_OPERATOR)
			{
				state = 1;
				PushOperArray(QUEUE, i, 2); //���ò�����������operand,������������
				i = 0;
				showData(num);
				lastCaculate();
				firstCaculate();
				showAnswer();
				f_clean = 1;
			}
		}
		else if (num == '(')
		{
			XCH_LEFT;
			state = 1;
			showData(num);
		}
		else if (num == ')') //�黹���ݣ�д��ڶ������������ص�״̬flash
		{
			state = flash[stack_idx - 1];
			PushOperArray(QUEUE, i, 1); //���ò�����������operand,�ڶ���������
			i = 0;
			lastCaculate();
			firstCaculate(); //ans is operand[stack_idx][0]����ʼ�黹����
			XCH_RIGHT;
			showData(num);
		}
		else if (num == 'S')
		{
			PushOperArray(QUEUE, i, 2); //���ò�����������operand,��һ��������
			operand[stack_idx][2] = sqrt(operand[stack_idx][2]);
			showData(num);
			state = 7;
		}
		else
		{
			beep();
		}
		break;
	}
}
/**********�Ӽ��̻�ȡֵ�ú���������C���Ե�scanf()����**************/
void scanf_mat()
{
	uchar temp;
	int q = 1;
	int a = 1; // ���ڼ�ⰴ���Ƿ�̧��
	u8 j;
	temp = q;
	while (1)
	{
		P3 = 0xff;
		//k1����-��k2����*��k3����/
		if (K1 == 0) //���Ӷ����������жϣ�����������p0
		{
			delay_long(); //��ʱ���������������
			if (K1 == 0)
			{
				while (!K1)
					;
				num = '(';
				break;
			}
		}
		if (K2 == 0) //���Ӷ����������жϣ�����������p0
		{
			delay_long(); //��ʱ���������������
			if (K2 == 0)
			{
				while (!K2)
					;
				num = ')';
				break;
			}
		}
		if (K3 == 0) //���Ӷ����������жϣ�����������p0
		{
			delay_long(); //��ʱ���������������
			if (K3 == 0)
			{
				while (!K3)
					;
				num = 'S';
				break;
			}
		}
		if (K4 == 0) //���Ӷ����������жϣ�����������p0
		{
			delay_long(); //��ʱ���������������
			if (K4 == 0)
			{
				while (!K4)
				{
					for (j = 0; j < 16; j++)
					{
						Hc595SendByte(~ledwei[j + 16], ~ledwei[j], ledmeng[16 + j], ledmeng[j]);
						delay(10);
					}
				}
				clear();
				state = 1;
				i = 0;
				num = '+';
				break;
			}
		}
		// if (K5 == 0)
		// {
		// 	delay_long(); //��ʱ���������������
		// 	if (K5 == 0)
		// 	{
		// 		while (!K5)
		// 			;
		// 		num = '(';

		// 		break;
		// 	}
		// }
		// if (K6 == 0)
		// {
		// 	delay_long(); //��ʱ���������������
		// 	if (K6 == 0)
		// 	{
		// 		while (!K6)
		// 			;
		// 		num = ')';

		// 		break;
		// 	}
		// }
		// if (K7 == 0)
		// {
		// 	delay_long(); //��ʱ�����������������Ϊ��������
		// 	if (K7 == 0)
		// 	{
		// 		while (!K7)
		// 			;
		// 		num = 'S';
		// 		break;
		// 	}
		// }
		if (K8 == 0)
		{
			delay_long(); //��ʱ���������������
			if (K8 == 0)
			{
				while (!K8)
				{
					for (j = 0; j < 16; j++)
					{
						Hc595SendByte(~ledwei[j + 16], ~ledwei[j], ledmeng[16 + j], ledmeng[j]);
						delay(10);
					}
				}
				num = 'M';
				break;
			}
		}

		P3 = 0x0f; //����Ϊ�ߵ�ƽ����Ϊ�͵�ƽ���������ڼ����ֵ��
		if (P3 != 0x0f)
		{
			delay_long(); //��ʱ���������������
			temp = P3;	  //������ֵ
			P3 = 0xf0;	  //����Ϊ�͵�ƽ����Ϊ�ߵ�ƽ����ȡ��
			if (P3 != 0xf0)
			{
				num = temp | P3; //��ȡ�˰���λ��
				//P2=1;
				for (q = 0; q < 16; q++)
					if (num == KEY_CODE[q])
					{
						switch (q + 1)
						{
						case 1:
							num = '7';
							break;
						case 2:
							num = '8';
							break;
						case 3:
							num = '9';
							break;
						case 4:
							num = '+';
							break;
						case 5:
							num = '4';
							break;
						case 6:
							num = '5';
							break;
						case 7:
							num = '6';
							break;
						case 8:
							num = '-';
							break;
						case 9:
							num = '1';
							break;
						case 10:
							num = '2';
							break;
						case 11:
							num = '3';
							break;
						case 12:
							num = '*';
							break;
						case 13:
							num = '0';
							break;
						case 14:
							num = '.';
							break;
						case 15:
							num = '=';
							break;
						case 16:
							num = '/';
							break;
						default:
							break;
						}
						// if (q == 10)
						// 	num = '+'; //��ȡ�Ⱥŵ�ֵ
						// else if (q == 11)
						// 	num = '='; //��ȡ�Ӻŵ�ֵ
						// else
						// 	num = q + 0x30; //��ȡ��ֵ
					}
				while (P3 != 0xf0)
					;  // ��ⰴ���Ƿ�̧��
				break; //����ѭ����Ϊ��ֻ��ȡһ��ֵ
			}
		}
	}
}
/**********��queue�ж�ȡ������ѹ��ջ��**************/

void PushOperArray(uchar *arr, int i, int locationOperand) //�õ�������
{
	if (bef != ')' && bef != 'S')
	{
		double total = 0;
		double ptotal = 0;
		uchar j = 0;

		for (j = 0; j < i; j++)
		{
			if (arr[j] == '.')
			{
				for (j = j + 1, i = i - 1; j <= i; i--)
				{
					ptotal = (ptotal + (arr[i] - 0x30)) / 10;
				}
				break;
			}
			total = total * 10 + (arr[j] - 0x30);
		}
		operand[stack_idx][locationOperand] = total + ptotal;
	}
}
/*********************����ʱ����*************************/
void delay_short()
{
	int n = 3000;
	while (n--)
		;
}

/*****************���峤�����ʱ����**********************/
void delay_long()
{
	uint n = 8000;
	while (n--)
		;
}

/*******************д�����ӳ���**************************/
void writeCMD(uchar com)
{
	P0 = com; //comΪ����������롣ͨ��P2�͸�LCD
	RS = 0;	  //RS=0 д����
	RW = 0;
	delay_short();
	EN = 1; //LCD��ʹ�ܶ�E�øߵ�ƽ
	delay_short();
	EN = 0; //LCD��ʹ�ܶ�E�õ͵�ƽ
}
/*******************д�����ӳ���**************************/
void showData(uchar dat)
{
	count++;
	if (count == 17) //���д��룬�䵽8���Զ�����
	{
		writeCMD(0x02);
		writeCMD(0x80 + 0x40);
		// count = 0;
	}
	else if (count == 33)
	{
		clear();
		wirteCache();
		count = 17;
	}
	//EN=1;
	//RW=1;
	//RS=0;
	if (P0 == 0x11)
	{
		writeCMD(0xC0);
	}
	P0 = dat; //д������
	RS = 1;	  //RS=1д����
	RW = 0;
	EN = 1;
	delay_short();
	EN = 0;

	LCD_CACHE[(count - 1) % 16] = dat;
}
/*******************дCache�ӳ���**************************/
void wirteCache()
{
	int writeIdx = 0;
	while (writeIdx < 16)
	{
		if (P0 == 0x11)
		{
			writeCMD(0xC0);
		}
		P0 = LCD_CACHE[writeIdx]; //д������
		RS = 1;					  //RS=1д����
		RW = 0;
		EN = 1;
		delay_short();
		EN = 0;
		writeIdx++;
	}
	writeCMD(0x02);
	writeCMD(0x80 + 0x40);
}
/*******************��ʼ������**************************/
void init()
{
	count = 0;
	EN = 0;
	writeCMD(0x38); //������ʾģʽ
	writeCMD(0x0d); //���򿪣���˸
	writeCMD(0x06); //д��һ���ַ���ָ���ַ+1��дһ���ַ�ʱ�������ƶ�
	writeCMD(0x01); //������ʾ������ָ����0��������ʾ��0
	writeCMD(0x80); //�����ַ���ʾ���׵�ַ
}
/*********************�����ӳ���**********************/
void clear()
{
	count = 0;
	EN = 0;
	writeCMD(0x01);
}
/************************ǰ����������**********************************/
void firstCaculate()
{
	double num1, num2, ans;
	char str[] = "ERROR";
	int m = 0;
	unsigned char op;
	num1 = operand[stack_idx][0];
	num2 = operand[stack_idx][1]; //ȡ������������
	op = operator[stack_idx][0];  //ȡ�����������
	switch (op)
	{
	case '+':
		ans = num1 + num2;
		break;
	case '-':
		ans = num1 - num2;
		break;
	case '*':
		ans = num1 * num2;
		break;
	case '/':
		if (operand[stack_idx][1] != 0)
		{
			ans = num1 / num2;
		}
		else
		{
			clear();
			//sprintf(str,"%g",ans);    //����������ַ����飬�������
			while (1)
			{
				if (str[m] == '\0')
					break;
				else
				{
					showData(str[m]); //��ʾ������ϢERROR
					m++;
				}
			}
			state = 1;
			beep();
		}
		break;
	}
	operand[stack_idx][0] = ans;
}

/************************������������**********************************/
void lastCaculate()
{
	double num1, num2, ans;
	unsigned char op;
	char str[] = "ERROR";
	int m = 0;
	num1 = operand[stack_idx][1];
	num2 = operand[stack_idx][2]; //ȡ������������
	op = operator[stack_idx][1];  //ȡ�����������
	switch (op)
	{
	case '+':
		ans = num1 + num2;
		break;
	case '-':
		ans = num1 - num2;
		break;
	case '*':
		ans = num1 * num2;
		break;
	case '/':
		if (operand[stack_idx][2] != 0)
		{
			ans = num1 / num2;
		}
		else
		{
			clear();
			//sprintf(str,"%g",ans);    //����������ַ����飬�������
			while (1)
			{
				if (str[m] == '\0')
					break;
				else
				{
					showData(str[m]);
					m++;
				}
			}
			state = 1;
			beep();
		}
		break;
	}
	operand[stack_idx][1] = ans;
}

/***************************��ʾ���*******************************/
void showAnswer()
{
	double ans = operand[stack_idx][0];
	char str[MAX_LEN * 2];
	uchar m;
	sprintf(str, "%g", ans); //����������ַ����飬�������
	m = 0;
	while (1)
	{
		if (str[m] == '\0')
			break;
		else
		{
			showData(str[m]);
			m++;
		}
	}
}

/**********************************************************/
void beep()
{
	unsigned char q, j;
	for (q = 0; q < 100; q++)
	{

		BEEP = !BEEP;			  //BEEPȡ��
		for (j = 0; j < 250; j++) //��Ҫ��������
			_nop_();
	}
	BEEP = 1; //�رշ�����
}
