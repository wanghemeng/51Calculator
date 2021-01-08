/**************************************************************************************
	*			              Calculator				   						  	 *
	*																				 *
	*   	  Jp8->JP5   	Jp9->Jp4	insert 1602 display			P2^4->J7         *
	*		  J27->J28		J32->J34	P2^0->RC	P2^1->SC		P2^2->SE         *
***************************************************************************************/

#include <reg51.h> //此文件中定义了51的一些特殊功能寄存器
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
#define NUM_STACK 10 //括号的级数

sbit EN = P2 ^ 7; //LCD的使能引脚
sbit RS = P2 ^ 6; //LCD数据命令选择端
sbit RW = P2 ^ 5; //LCD的读写选择端
sbit K1 = P1 ^ 0;
sbit K2 = P1 ^ 1;
sbit K3 = P1 ^ 2;
sbit K4 = P1 ^ 3;
sbit K5 = P1 ^ 4;
sbit K6 = P1 ^ 5;
sbit K7 = P1 ^ 6;
sbit K8 = P1 ^ 7;
sbit BEEP = P2 ^ 4; //将蜂鸣器定义在 P2.4 端口上

uchar KEY_CODE[] = {0xee, 0xde, 0xbe, 0x7e, //4X4矩阵键盘键值表
					0xed, 0xdd, 0xbd, 0x7d,
					0xeb, 0xdb, 0xbb, 0x7b,
					0xe7, 0xd7, 0xb7, 0x77};
uchar i = 0;	  //统计操作数的位数
uchar count = 0;  //计数变量，与LCD的换行相关
uchar num = 0xff; //输入变量
uchar bef = 0xff; //记录上一个num的值，判断是否为）或S，算法逻辑相关
uchar state = 1;  //状态标识符,表示12个状态机的状态
uchar flash[NUM_STACK];
uchar operator[NUM_STACK][2];
double operand[NUM_STACK][3];
uchar stack_idx = 0;  //当前已读入括号数
uchar QUEUE[MAX_LEN]; //定义一个数组存储操作数中每一位
uchar LCD_CACHE[17];  //LCD屏幕Cache
uchar f_clean = 0;	  //是否清屏标识位（用于括号）

void scanf_mat();											//从矩阵键盘中获取值
void delay_short();											//短延时函数
void delay_long();											//较长延时函数
void writeCMD(uchar com);									//写命令子程序
void showData(uchar dat);									//写数据子程序
void init();												//初始化子程序，初始化液晶显示屏
void clear();												//清除显示屏上的显示
void stateChange();											//状态转换函数，表示状态转换
void PushOperArray(uchar *arr, int i, int locationOperand); //操作数转换函数
void firstCaculate();										//operand0=operand0+operand1;
void lastCaculate();										//operand1=operand2+operand1;
void beep();												//蜂鸣器函数
void showAnswer();											//展示结果函数
void wirteCache();											//循环屏幕函数
void Hc595SendByte(u8 dat1, u8 dat2, u8 dat3, u8 dat4);		//LED显示函数

void main()
{
	// Timer0Init(); //定时器0初始化
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
		stateChange(); //主功能实现
	}
}

/**********状态转换函数**************/
void stateChange()
{
	switch (state)
	{
	case 1: //初始状态
		if (f_clean == 1)
		{
			clear();
			f_clean = 0;
		}

		if ((num >= '0' && num <= '9') && (i < MAX_LEN)) //输入合法的0到9数字，进入状态2
		{
			state = 2;
			QUEUE[i] = num; //数字进栈
			showData(num);
			i++;
		}
		else if (num == '(')
		{
			XCH_LEFT;
			// operand1[0] = operand[stack_idx][0];
			// operand1[1] = operand[stack_idx][1];
			// operand1[2] = operand[stack_idx][2]; //交接数据完成，开始()内的运算
			state = 1; //回到起点
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
		else if (num == '+' || num == '-') //进入状态3 A+
		{
			state = 3;
			PushOperArray(QUEUE, i, 0); //放置操作数于数组operand,第一个操作数
			i = 0;
			operator[stack_idx][0] = num;
			showData(num);
		}
		else if (num == '*') //进入状态4  A*
		{
			state = 4;
			PushOperArray(QUEUE, i, 0); //放置操作数于数组operand,第一个操作数
			i = 0;
			operator[stack_idx][0] = num;
			showData(num);
		}
		else if (num == '/') //进入状态5  A/
		{
			state = 5;
			PushOperArray(QUEUE, i, 0); //放置操作数于数组operand,第一个操作数
			i = 0;
			operator[stack_idx][0] = num;
			showData(num);
		}
		else if (num == '=') //进入状态1  执行运算数=数
		{
			state = 1;
			PushOperArray(QUEUE, i, 0); //放置操作数于数组operand,第一个操作数
			i = 0;
			showData(num);
			showAnswer();
			f_clean = 1;
		}
		else if (num == 'S')
		{
			PushOperArray(QUEUE, i, 0); //放置操作数于数组operand,第一个操作数
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
		{ //A+B+ 计算前两位操作数
			if (BEF_NOT_OPERATOR)
			{
				//caculate();
				state = 3;
				PushOperArray(QUEUE, i, 1); //放置操作数于数组operand,第二个操作数
				i = 0;
				firstCaculate();			  //在第一个操作符+-，被+-覆盖之前，计算A+/-B，并把结果放在第一个操作数。
				operator[stack_idx][0] = num; //放置第一位操作符
				showData(num);
			}
		}
		else if (num == '*') //A+B*
		{
			if (BEF_NOT_OPERATOR)
			{
				state = 6;
				PushOperArray(QUEUE, i, 1); //放置操作数于数组operand,第二个操作数
				i = 0;
				operator[stack_idx][1] = num; //放置第二位操作符
				showData(num);
			}
		}
		else if (num == '/') //进入状态7  A+B/
		{
			if (BEF_NOT_OPERATOR)
			{
				state = 7;
				PushOperArray(QUEUE, i, 1); //放置操作数于数组operand,第二个操作数
				i = 0;
				operator[stack_idx][1] = num; //放置第二位操作符
				showData(num);
			}
		}
		else if (num == '=') //A+B=
		{
			if (BEF_NOT_OPERATOR)
			{
				state = 1;
				PushOperArray(QUEUE, i, 1); //放置操作数于数组operand,第二个操作数
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
			state = 1; //回到起点
			showData(num);
		}
		else if (num == ')') //归还数据，写入第二个操作数，回到状态flash
		{
			state = flash[stack_idx - 1];
			PushOperArray(QUEUE, i, 1); //放置操作数于数组operand,第二个操作数
			i = 0;
			firstCaculate(); //ans is operand[stack_idx][0]，开始归还数据
			XCH_RIGHT;
			showData(num);
		}
		else if (num == 'S')
		{
			PushOperArray(QUEUE, i, 1); //放置操作数于数组operand,第一个操作数
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
		else if (num == '+' || num == '-') //A*B+ B后面是+的时候转移到状态3 A*B+ -->A+
		{
			if (BEF_NOT_OPERATOR)
			{
				state = 3;
				PushOperArray(QUEUE, i, 1); //放置操作数于数组operand,第二个操作数
				i = 0;
				firstCaculate(); //在第一个操作符*，被+-覆盖之前，计算A*B，并把结果放在第一个操作数。
				operator[stack_idx][0] = num;
				showData(num);
			}
		}
		else if (num == '*')
		{ //A*B* -->A*
			if (BEF_NOT_OPERATOR)
			{
				state = 4;
				PushOperArray(QUEUE, i, 1); //放置操作数于数组operand,第二个操作数
				i = 0;
				firstCaculate(); //在第一个操作符*，被*覆盖之前，计算A*B，并把结果放在第一个操作数。
				operator[stack_idx][0] = num;
				showData(num);
			}
		}
		else if (num == '/')
		{ //A*B/-->A/
			if (BEF_NOT_OPERATOR)
			{
				state = 5;
				PushOperArray(QUEUE, i, 1); //放置操作数于数组operand,第二个操作数
				i = 0;
				firstCaculate(); //在第一个操作符*，被/覆盖之前，计算A*B，并把结果放在第一个操作数。
				operator[stack_idx][0] = num;
				showData(num);
			}
		}
		else if (num == '=') // A*B=
		{
			if (BEF_NOT_OPERATOR)
			{
				state = 1;
				PushOperArray(QUEUE, i, 1); //放置操作数于数组operand,第二个操作数
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
		else if (num == ')') //归还数据，写入第二个操作数，回到状态flash
		{
			state = flash[stack_idx - 1];
			PushOperArray(QUEUE, i, 1); //放置操作数于数组operand,第二个操作数
			i = 0;
			firstCaculate(); //ans is operand[stack_idx][0]，开始归还数据
			XCH_RIGHT;
			showData(num);
		}
		else if (num == 'S')
		{
			PushOperArray(QUEUE, i, 1); //放置操作数于数组operand,第一个操作数
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
		if ((num >= '0' && num <= '9') && (i < MAX_LEN) || num == '.') //进入状态5  数/(不为0的数)
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
				PushOperArray(QUEUE, i, 1); //放置操作数于数组operand,第二个操作数
				i = 0;
				firstCaculate(); //在第一个操作符/，被+-覆盖之前，计算A/B，并把结果放在第一个操作数。
				operator[stack_idx][0] = num;
				showData(num);
			}
		}
		else if (num == '*')
		{ //进入状态4  A/B*-->A*
			if (BEF_NOT_OPERATOR)
			{
				state = 4;
				PushOperArray(QUEUE, i, 1); //放置操作数于数组operand,第二个操作数
				i = 0;
				firstCaculate(); //在第一个操作符/，被*覆盖之前，计算A/B，并把结果放在第一个操作数。
				operator[stack_idx][0] = num;
				showData(num);
			}
		}
		else if (num == '/')
		{ //进入状态5  A/B/ --> A/
			if (BEF_NOT_OPERATOR)
			{
				state = 5;
				PushOperArray(QUEUE, i, 1); //放置操作数于数组operand,第二个操作数
				i = 0;
				firstCaculate(); //在第一个操作符/，被/覆盖之前，计算A/B，并把结果放在第一个操作数。
				operator[stack_idx][0] = num;
				showData(num);
			}
		}
		else if (num == '=') //A/B = A/
		{
			if (BEF_NOT_OPERATOR)
			{
				state = 1;
				PushOperArray(QUEUE, i, 1); //放置操作数于数组operand,第二个操作数
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
			state = 1; //回到起点
			showData(num);
		}
		else if (num == ')') //归还数据，写入第二个操作数，回到状态flash
		{
			state = flash[stack_idx - 1];
			PushOperArray(QUEUE, i, 1); //放置操作数于数组operand,第二个操作数
			i = 0;
			firstCaculate(); //ans is operand[stack_idx][0]，开始归还数据
			XCH_RIGHT;
			showData(num);
		}
		else if (num == 'S')
		{
			PushOperArray(QUEUE, i, 1); //放置操作数于数组operand,第一个操作数
			operand[stack_idx][1] = sqrt(operand[stack_idx][1]);
			showData(num);
			state = 5;
		}
		else
		{
			beep();
		}
		break;

	case 6:															   //状态6--A+B* 因为要先算B*再算A+ 所以出现了状态6
		if ((num >= '0' && num <= '9') && (i < MAX_LEN) || num == '.') //进入状态11  数+/-数*数
		{
			state = 6;
			QUEUE[i] = num;
			showData(num);
			i++;
		}
		else if (num == '+' || num == '-')
		{ //进入状态3  数+/-数*数+/-=数+/-数+/-=数+/-（3+4*5+）
			if (BEF_NOT_OPERATOR)
			{
				state = 3;
				PushOperArray(QUEUE, i, 2); //放置操作数于数组operand,第三个操作数
				i = 0;
				lastCaculate();	 //计算A+B*C中的B*C，并将结果至于第二个操作数。
				firstCaculate(); //计算A+(B*C的结果)，并将结果至于第一个操作数。
				operator[stack_idx][0] = num;
				showData(num);
			}
		}
		else if (num == '*')
		{ //进入状态6  A+B*C*=A+B*
			if (BEF_NOT_OPERATOR)
			{
				state = 6;
				PushOperArray(QUEUE, i, 2); //放置操作数于数组operand,第三个操作数
				i = 0;
				lastCaculate();				  //计算A+B*C中的B*C，并将结果至于第二个操作数。
				operator[stack_idx][1] = num; //A+-（B*C的结果）中后的*存于第二个操作符。
				showData(num);
			}
		}
		else if (num == '/')
		{ //进入状态7  A+B*C/=A+B/
			if (BEF_NOT_OPERATOR)
			{
				state = 7;
				PushOperArray(QUEUE, i, 2); //放置操作数于数组operand,第三个操作数
				i = 0;
				lastCaculate();				  //计算A+B*C中的B*C，并将结果至于第二个操作数。
				operator[stack_idx][1] = num; //A+-（B*C的结果）中后的/存于第二个操作符。
				showData(num);
			}
		}
		else if (num == '=') //进入状态1  A+B*C= --> A+B=
		{
			if (BEF_NOT_OPERATOR)
			{
				state = 1;
				PushOperArray(QUEUE, i, 2); //放置操作数于数组operand,第三个操作数
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
		else if (num == ')') //归还数据，写入第二个操作数，回到状态3
		{
			state = flash[stack_idx - 1];
			PushOperArray(QUEUE, i, 1); //放置操作数于数组operand,第二个操作数
			i = 0;
			lastCaculate();
			firstCaculate(); //ans is operand[stack_idx][0]，开始归还数据
			XCH_RIGHT;
			showData(num);
		}
		else if (num == 'S')
		{
			PushOperArray(QUEUE, i, 2); //放置操作数于数组operand,第一个操作数
			operand[stack_idx][2] = sqrt(operand[stack_idx][2]);
			showData(num);
			state = 6;
		}
		else
		{
			beep();
		}
		break;

	case 7:															   //状态7--A+B
		if ((num >= '0' && num <= '9') && (i < MAX_LEN) || num == '.') //由于是除法运算，第一位不为零才可以。
		{
			state = 7;
			QUEUE[i] = num;
			showData(num);
			i++;
		}
		else if (num == '+' || num == '-')
		{ //进入状态3  A+B/C+ -->A+
			if (BEF_NOT_OPERATOR)
			{
				state = 3;
				PushOperArray(QUEUE, i, 2); //放置操作数于数组operand,第三个操作数
				i = 0;
				lastCaculate();	 //计算A+B/C中的B/C，并将结果至于第二个操作数。
				firstCaculate(); //计算A+(B/C的结果)，并将结果至于第一个操作数。
				operator[stack_idx][0] = num;
				showData(num);
			}
		}
		else if (num == '*')
		{ //进入状态6  A+B/C* --> A+B*
			if (BEF_NOT_OPERATOR)
			{
				state = 6;
				PushOperArray(QUEUE, i, 2); //放置操作数于数组operand,第三个操作数
				i = 0;
				lastCaculate();				  //计算A+B/C中的B/C，并将结果至于第二个操作数。
				operator[stack_idx][1] = num; //A+-（B/C的结果）中后的*存于第二个操作符。
				showData(num);
			}
		}
		else if (num == '/')
		{ //进入状态7  A+B/C/ --> A+B/
			if (BEF_NOT_OPERATOR)
			{
				state = 7;
				PushOperArray(QUEUE, i, 2); //放置操作数于数组operand,第三个操作数
				i = 0;
				lastCaculate();				  //计算A+B/C中的B/C，并将结果至于第二个操作数。
				operator[stack_idx][1] = num; //A+-（B/C的结果）中后的/存于第二个操作符。
				showData(num);
			}
		}
		else if (num == '=') //进入状态1  A+B/C= --> A+B=
		{
			if (BEF_NOT_OPERATOR)
			{
				state = 1;
				PushOperArray(QUEUE, i, 2); //放置操作数于数组operand,第三个操作数
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
		else if (num == ')') //归还数据，写入第二个操作数，回到状态flash
		{
			state = flash[stack_idx - 1];
			PushOperArray(QUEUE, i, 1); //放置操作数于数组operand,第二个操作数
			i = 0;
			lastCaculate();
			firstCaculate(); //ans is operand[stack_idx][0]，开始归还数据
			XCH_RIGHT;
			showData(num);
		}
		else if (num == 'S')
		{
			PushOperArray(QUEUE, i, 2); //放置操作数于数组operand,第一个操作数
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
/**********从键盘获取值得函数类似于C语言的scanf()函数**************/
void scanf_mat()
{
	uchar temp;
	int q = 1;
	int a = 1; // 用于检测按键是否抬起
	u8 j;
	temp = q;
	while (1)
	{
		P3 = 0xff;
		//k1代表-，k2代表*，k3代表/
		if (K1 == 0) //增加独立按键的判断，独立按键连p0
		{
			delay_long(); //延时，软件消除抖动。
			if (K1 == 0)
			{
				while (!K1)
					;
				num = '(';
				break;
			}
		}
		if (K2 == 0) //增加独立按键的判断，独立按键连p0
		{
			delay_long(); //延时，软件消除抖动。
			if (K2 == 0)
			{
				while (!K2)
					;
				num = ')';
				break;
			}
		}
		if (K3 == 0) //增加独立按键的判断，独立按键连p0
		{
			delay_long(); //延时，软件消除抖动。
			if (K3 == 0)
			{
				while (!K3)
					;
				num = 'S';
				break;
			}
		}
		if (K4 == 0) //增加独立按键的判断，独立按键连p0
		{
			delay_long(); //延时，软件消除抖动。
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
		// 	delay_long(); //延时，软件消除抖动。
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
		// 	delay_long(); //延时，软件消除抖动。
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
		// 	delay_long(); //延时，软件消除抖动。此为根号运算
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
			delay_long(); //延时，软件消除抖动。
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

		P3 = 0x0f; //置行为高电平，列为低电平。这样用于检测行值。
		if (P3 != 0x0f)
		{
			delay_long(); //延时，软件消除抖动。
			temp = P3;	  //保存行值
			P3 = 0xf0;	  //置行为低电平，列为高电平，获取列
			if (P3 != 0xf0)
			{
				num = temp | P3; //获取了按键位置
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
						// 	num = '+'; //获取等号的值
						// else if (q == 11)
						// 	num = '='; //获取加号的值
						// else
						// 	num = q + 0x30; //获取数值
					}
				while (P3 != 0xf0)
					;  // 检测按键是否抬起
				break; //跳出循环，为了只获取一个值
			}
		}
	}
}
/**********从queue中读取操作数压入栈中**************/

void PushOperArray(uchar *arr, int i, int locationOperand) //得到操作数
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
/*********************短延时函数*************************/
void delay_short()
{
	int n = 3000;
	while (n--)
		;
}

/*****************定义长点的延时程序**********************/
void delay_long()
{
	uint n = 8000;
	while (n--)
		;
}

/*******************写命令子程序**************************/
void writeCMD(uchar com)
{
	P0 = com; //com为输入的命令码。通过P2送给LCD
	RS = 0;	  //RS=0 写命令
	RW = 0;
	delay_short();
	EN = 1; //LCD的使能端E置高电平
	delay_short();
	EN = 0; //LCD的使能端E置低电平
}
/*******************写数据子程序**************************/
void showData(uchar dat)
{
	count++;
	if (count == 17) //换行代码，输到8个自动换行
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
	P0 = dat; //写入数据
	RS = 1;	  //RS=1写命令
	RW = 0;
	EN = 1;
	delay_short();
	EN = 0;

	LCD_CACHE[(count - 1) % 16] = dat;
}
/*******************写Cache子程序**************************/
void wirteCache()
{
	int writeIdx = 0;
	while (writeIdx < 16)
	{
		if (P0 == 0x11)
		{
			writeCMD(0xC0);
		}
		P0 = LCD_CACHE[writeIdx]; //写入数据
		RS = 1;					  //RS=1写命令
		RW = 0;
		EN = 1;
		delay_short();
		EN = 0;
		writeIdx++;
	}
	writeCMD(0x02);
	writeCMD(0x80 + 0x40);
}
/*******************初始化函数**************************/
void init()
{
	count = 0;
	EN = 0;
	writeCMD(0x38); //设置显示模式
	writeCMD(0x0d); //光标打开，闪烁
	writeCMD(0x06); //写入一个字符后指针地址+1，写一个字符时整屏不移动
	writeCMD(0x01); //清屏显示，数据指针清0，所以显示清0
	writeCMD(0x80); //设置字符显示的首地址
}
/*********************清屏子程序**********************/
void clear()
{
	count = 0;
	EN = 0;
	writeCMD(0x01);
}
/************************前两个数运算**********************************/
void firstCaculate()
{
	double num1, num2, ans;
	char str[] = "ERROR";
	int m = 0;
	unsigned char op;
	num1 = operand[stack_idx][0];
	num2 = operand[stack_idx][1]; //取后两个操作数
	op = operator[stack_idx][0];  //取后两个运算符
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
			//sprintf(str,"%g",ans);    //将结果存入字符数组，用于输出
			while (1)
			{
				if (str[m] == '\0')
					break;
				else
				{
					showData(str[m]); //显示错误信息ERROR
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

/************************后两个数运算**********************************/
void lastCaculate()
{
	double num1, num2, ans;
	unsigned char op;
	char str[] = "ERROR";
	int m = 0;
	num1 = operand[stack_idx][1];
	num2 = operand[stack_idx][2]; //取后两个操作数
	op = operator[stack_idx][1];  //取后两个运算符
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
			//sprintf(str,"%g",ans);    //将结果存入字符数组，用于输出
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

/***************************显示结果*******************************/
void showAnswer()
{
	double ans = operand[stack_idx][0];
	char str[MAX_LEN * 2];
	uchar m;
	sprintf(str, "%g", ans); //将结果存入字符数组，用于输出
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

		BEEP = !BEEP;			  //BEEP取反
		for (j = 0; j < 250; j++) //需要产生方波
			_nop_();
	}
	BEEP = 1; //关闭蜂鸣器
}
