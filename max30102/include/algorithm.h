#ifndef __ALGORITHM_H
#define __ALGORITHM_H

#define FFT_N 				512    //���帵��Ҷ�任�ĵ���
#define START_INDEX 	4  //��Ƶ������ֵ

struct compx     //����һ�������ṹ
{
	float real;
	float imag;
};  

//����ȡ��
double my_floor(double x);
//��������
double my_fmod(double x, double y);
//���Һ���
double XSin( double x );
//���Һ��� 
double XCos( double x );
//��ƽ��
int qsqrt(int a);

/*******************************************************************
����ԭ�ͣ�struct compx EE(struct compx b1,struct compx b2)  
�������ܣ��������������г˷�����
��������������������嶨��ĸ���a,b
���������a��b�ĳ˻��������������ʽ���
*******************************************************************/
struct compx EE(struct compx a,struct compx b);
/*****************************************************************
����ԭ�ͣ�void FFT(struct compx *xin,int N)
�������ܣ�������ĸ�������п��ٸ���Ҷ�任��FFT��
���������*xin�����ṹ������׵�ַָ�룬struct��
*****************************************************************/
void FFT(struct compx *xin);

//��ȡ��ֵ
int find_max_num_index(struct compx *data,int count);

typedef struct
{
	float w;
	int init;
	float a;

}DC_FilterData;

//ֱ���˲���
int dc_filter(int input,DC_FilterData * df);

typedef struct
{
	float v0;
	float v1;
}BW_FilterData;

int bw_filter(int input,BW_FilterData * bw);


#endif  /*__ALGORITHM_H*/

