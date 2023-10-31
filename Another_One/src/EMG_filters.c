#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "EMG_filters.h"


SAMPLE_FREQUENCY m_sampleFreq;
NOTCH_FREQUENCY  m_notchFreq;
bool             m_bypassEnabled;
bool             m_notchFilterEnabled;
bool             m_lowpassFilterEnabled;
bool             m_highpassFilterEnabled;
float            states_2nd_private[2];
float            num_2nd_private[3];
float            den_2nd_private[3];
float            states_4th_private[4];
float            num_4th_private[6];
float            den_4th_private[6];
float            gain;
//lpf_numerator_coef：低通滤波器（LPF）的系数。
// lpf_denominator_coef：LPF传递函数的分母系数。
// hpf_numerator_coef：高通滤波器（HPF）的系数。
// hpf_denominator_coef：HPF传递函数的分母系数。
// ahf_numerator_coef_50Hz：针对50Hz噪声设计的抗杂波滤波器（AHF）系数。
// ahf_denominator_coef_50Hz：针对50Hz噪声的AHF传递函数的分母系数。
// ahf_output_gain_coef_50Hz：50Hz AHF的输出增益系数。
// ahf_numerator_coef_60Hz：针对60Hz噪声设计的AHF系数。
// ahf_denominator_coef_60Hz：针对60Hz噪声的AHF传递函数的分母系数。
// ahf_output_gain_coef_60Hz：60Hz AHF的输出增益系数。
static float lpf_numerator_coef[2][3] = {{0.3913, 0.7827, 0.3913},
                                         {0.1311, 0.2622, 0.1311}};
static float lpf_denominator_coef[2][3] = {{1.0000, 0.3695, 0.1958},
                                           {1.0000, -0.7478, 0.2722}};
// coefficients of transfer function of HPF
static float hpf_numerator_coef[2][3] = {{0.8371, -1.6742, 0.8371},
                                         {0.9150, -1.8299, 0.9150}};
static float hpf_denominator_coef[2][3] = {{1.0000, -1.6475, 0.7009},
                                           {1.0000, -1.8227, 0.8372}};
// coefficients of transfer function of anti-hum filter
// coef[sampleFreqInd][order] for 50Hz
static float ahf_numerator_coef_50Hz[2][6] = {
    {0.9522, -1.5407, 0.9522, 0.8158, -0.8045, 0.0855},
    {0.5869, -1.1146, 0.5869, 1.0499, -2.0000, 1.0499}};
static float ahf_denominator_coef_50Hz[2][6] = {
    {1.0000, -1.5395, 0.9056, 1.0000 - 1.1187, 0.3129},
    {1.0000, -1.8844, 0.9893, 1.0000, -1.8991, 0.9892}};
static float ahf_output_gain_coef_50Hz[2] = {1.3422, 1.4399};
// coef[sampleFreqInd][order] for 60Hz
static float ahf_numerator_coef_60Hz[2][6] = {
    {0.9528, -1.3891, 0.9528, 0.8272, -0.7225, 0.0264},
    {0.5824, -1.0810, 0.5824, 1.0736, -2.0000, 1.0736}};
static float ahf_denominator_coef_60Hz[2][6] = {
    {1.0000, -1.3880, 0.9066, 1.0000, -0.9739, 0.2371},
    {1.0000, -1.8407, 0.9894, 1.0000, -1.8584, 0.9891}};
static float ahf_output_gain_coef_60Hz[2] = {1.3430, 1.4206};

typedef enum {
    FILTER_TYPE_LOWPASS = 0,
    FILTER_TYPE_HIGHPASS = 1
} FILTER_TYPE;

void init_2nd(FILTER_TYPE ftype, int sampleFreq) {
    states_2nd_private[0] = 0;
    states_2nd_private[1] = 0;
    if (ftype == FILTER_TYPE_LOWPASS) {
        // 2th order butterworth lowpass filter
        // cutoff frequency 150Hz
        if (sampleFreq == SAMPLE_FREQ_500HZ) {
            for (int i = 0; i < 3; i++) {
                num_2nd_private[i] = lpf_numerator_coef[0][i];
                den_2nd_private[i] = lpf_denominator_coef[0][i];
            }
        } else if (sampleFreq == SAMPLE_FREQ_1000HZ) {
            for (int i = 0; i < 3; i++) {
                num_2nd_private[i] = lpf_numerator_coef[1][i];
                den_2nd_private[i] = lpf_denominator_coef[1][i];
            }
        }
    } else if (ftype == FILTER_TYPE_HIGHPASS) {
        // 2th order butterworth
        // cutoff frequency 20Hz
        if (sampleFreq == SAMPLE_FREQ_500HZ) {
            for (int i = 0; i < 3; i++) {
                num_2nd_private[i] = hpf_numerator_coef[0][i];
                den_2nd_private[i] = hpf_denominator_coef[0][i];
            }
        } else if (sampleFreq == SAMPLE_FREQ_1000HZ) {
            for (int i = 0; i < 3; i++) {
                num_2nd_private[i] = hpf_numerator_coef[1][i];
                den_2nd_private[i] = hpf_denominator_coef[1][i];
            }
        }
    }
}

int update_2nd(float input) {
    float tmp = (input - den_2nd_private[1] * states_2nd_private[0] - den_2nd_private[2] * states_2nd_private[1]) / den_2nd_private[0];
    float output = num_2nd_private[0] * tmp + num_2nd_private[1] * states_2nd_private[0] + num_2nd_private[2] * states_2nd_private[1];
    // save last states
    states_2nd_private[1] = states_2nd_private[0];
    states_2nd_private[0] = tmp;
    return output;
}


void init_4th(int sampleFreq, int humFreq) {
    gain = 0;
    for (int i = 0; i < 4; i++) {
        states_4th_private[i] = 0;
    }
    if (humFreq == NOTCH_FREQ_50HZ) {
        if (sampleFreq == SAMPLE_FREQ_500HZ) {
            for (int i = 0; i < 6; i++) {
                num_4th_private[i] = ahf_numerator_coef_50Hz[0][i];
                den_4th_private[i] = ahf_denominator_coef_50Hz[0][i];
            }
            gain = ahf_output_gain_coef_50Hz[0];
        } else if (sampleFreq == SAMPLE_FREQ_1000HZ) {
            for (int i = 0; i < 6; i++) {
                num_4th_private[i] = ahf_numerator_coef_50Hz[1][i];
                den_4th_private[i] = ahf_denominator_coef_50Hz[1][i];
            }
            gain = ahf_output_gain_coef_50Hz[1];
        }
    } else if (humFreq == NOTCH_FREQ_60HZ) {
        if (sampleFreq == SAMPLE_FREQ_500HZ) {
            for (int i = 0; i < 6; i++) {
                num_4th_private[i] = ahf_numerator_coef_60Hz[0][i];
                den_4th_private[i] = ahf_denominator_coef_60Hz[0][i];
            }
            gain = ahf_output_gain_coef_60Hz[0];
        } else if (sampleFreq == SAMPLE_FREQ_1000HZ) {
            for (int i = 0; i < 6; i++) {
                num_4th_private[i] = ahf_numerator_coef_60Hz[1][i];
                den_4th_private[i] = ahf_denominator_coef_60Hz[1][i];
            }
            gain = ahf_output_gain_coef_60Hz[1];
        }
    }
}

float update_4th(float input) {
    float output;
    float stageIn;
    float stageOut;

    stageOut  = num_4th_private[0] * input + states_4th_private[0];
    states_4th_private[0] = (num_4th_private[1] * input + states_4th_private[1]) - den_4th_private[1] * stageOut;
    states_4th_private[1] = num_4th_private[2] * input - den_4th_private[2] * stageOut;
    stageIn   = stageOut;
    stageOut  = num_4th_private[3] * stageOut + states_4th_private[2];
    states_4th_private[2] = (num_4th_private[4] * stageIn + states_4th_private[3]) - den_4th_private[4] * stageOut;
    states_4th_private[3] = num_4th_private[5] * stageIn - den_4th_private[5] * stageOut;

    output = gain * stageOut;

    return output;
}


/**
 * 该函数用于初始化EMG处理系统。它接收多个参数，例如采样频率、陷波频率以及启用/禁用各种滤波器。
 * 它调用init_2nd和init_4th函数来分别初始化二阶和四阶滤波器
*/
void EMG_init(SAMPLE_FREQUENCY sampleFreq,NOTCH_FREQUENCY  notchFreq,
                     bool             enableNotchFilter,
                     bool             enableLowpassFilter,
                     bool             enableHighpassFilter) {
    m_sampleFreq   = sampleFreq;
    m_notchFreq    = notchFreq;
    m_bypassEnabled = true;
    if (((sampleFreq == SAMPLE_FREQ_500HZ) || (sampleFreq == SAMPLE_FREQ_1000HZ)) &&
        ((notchFreq == NOTCH_FREQ_50HZ) || (notchFreq == NOTCH_FREQ_60HZ))) {
            m_bypassEnabled = false;
    }

    init_2nd(FILTER_TYPE_LOWPASS, m_sampleFreq);
    init_2nd(FILTER_TYPE_HIGHPASS, m_sampleFreq);
    init_4th(m_sampleFreq, m_notchFreq);

    m_notchFilterEnabled    = enableNotchFilter;
    m_lowpassFilterEnabled  = enableLowpassFilter;
    m_highpassFilterEnabled = enableHighpassFilter;
}

/**
 * 该函数处理输入的EMG信号并返回处理后的输出。它根据启用/禁用的标志应用滤波器。
 * 如果启用了陷波滤波器，则首先将信号通过陷波滤波器（update_4th函数）处理。
 * 然后，如果启用了低通滤波器，则通过低通滤波器（update_2nd函数）处理。
 * 最后，如果启用了高通滤波器，则通过高通滤波器（update_2nd函数）处理。
*/
int EMG_update(int inputValue) {
    int output = 0;
    if (m_bypassEnabled) {
        return output = inputValue;
    }

    // first notch filter
    if (m_notchFilterEnabled) {
        // output = NTF.update(inputValue);
        output = update_4th(inputValue);
    } else {
        // notch filter bypass
        output = inputValue;
    }

    // second low pass filter
    if (m_lowpassFilterEnabled) {
        output = update_2nd(output);
    }

    // third high pass filter
    if (m_highpassFilterEnabled) {
        output = update_2nd(output);
    }

    return output;
}

/**
 * 该函数根据提供的g-force包络值计算EMG计数。
 * 它维护一些静态变量，如integralData，用于随时间累积信号值。
 * 它检查积分值是否恒定，并在其变为恒定时记录时间。
 * 如果积分值再次开始变化，则将remainFlag设置为true，表示时间记录将在下一次迭代中重新进入。
 * 如果积分值超过一定阈值（200ms），则清除积分值并返回获取的最大包络值。
*/
int getEMGCount(int gforce_envelope)
    {
      static long integralData = 0;
      static long integralDataEve = 0;
      static bool remainFlag = false;
      static unsigned long timeMillis = 0;
      static unsigned long timeBeginzero = 0;
      static long fistNum = 0;
      static int  TimeStandard = 200;
      static int Max_envelope = 0;
      /*
        The integral is processed to continuously add the signal value
        and compare the integral value of the previous sampling to determine whether the signal is continuous
       */
      integralDataEve = integralData;
      integralData += gforce_envelope;
      /*
        If the integral is constant, and it doesn't equal 0, then the time is recorded;
        If the value of the integral starts to change again, the remainflag is true, and the time record will be re-entered next time
      */
      if (Max_envelope <= gforce_envelope) Max_envelope = gforce_envelope;

      if ((integralDataEve == integralData) && (integralDataEve != 0))
      {
        
        // timeMillis = millis();

        if (remainFlag)
        {
          remainFlag = false;
          return 0;
        }
        /* If the integral value exceeds 200 ms, the integral value is clear 0,return that get EMG signal */
        osDelay(10U);
        if (integralDataEve == integralData)
        {
          integralDataEve = integralData = 0;
          return Max_envelope;
        }
        return 0;
      }
      else {
        remainFlag = true;
        return 0;
       }
    }