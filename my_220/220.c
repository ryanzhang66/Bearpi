#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
int my220() {
    int uart_fd = open("/dev/ttyS0", O_RDWR);
    if (uart_fd < 0) {
        perror("Failed to open UART device");
        return -1;
    }
    
    struct termios uart_options;
    tcgetattr(uart_fd, &uart_options);
    cfsetispeed(&uart_options, B9600); // 设置波特率为9600
    cfsetospeed(&uart_options, B9600); // 设置波特率为9600
    uart_options.c_cflag &= ~PARENB;   // 禁用奇偶校验
    uart_options.c_cflag &= ~CSTOPB;   // 设置停止位为1
    uart_options.c_cflag &= ~CSIZE;    // 清除数据位相关的标志位
    uart_options.c_cflag |= CS8;       // 设置数据位为8位
    uart_options.c_cflag &= ~CRTSCTS;  // 禁用硬件流控制
    uart_options.c_cflag |= CREAD | CLOCAL; // 启用接收，忽略调制解调器状态
    tcsetattr(uart_fd, TCSANOW, &uart_options);
    
    // 读取电流参数寄存器的数值
    unsigned char current_param_command[] = {0x01, 0x04, 0x00, 0x01, 0x00, 0x02, 0x71, 0xCA};
    ssize_t bytes_written = write(uart_fd, current_param_command, sizeof(current_param_command));
    if (bytes_written != sizeof(current_param_command)) {
        perror("Failed to write UART");
        close(uart_fd);
        return -1;
    }
    
    unsigned char current_param_buffer[32];
    ssize_t current_param_bytes_read = read(uart_fd, current_param_buffer, sizeof(current_param_buffer));
    if (current_param_bytes_read < 0) {
        perror("Failed to read UART");
        close(uart_fd);
        return -1;
    }
    
    // 解析数据
    short current_param_value = (current_param_buffer[3] << 8) | current_param_buffer[4];
    int CurrentParameterREG = current_param_value;
    
    // 读取电流寄存器的数值
    unsigned char current_reg_command[] = {0x01, 0x04, 0x00, 0x02, 0x00, 0x02, 0xB0, 0x0B};
    bytes_written = write(uart_fd, current_reg_command, sizeof(current_reg_command));
    if (bytes_written != sizeof(current_reg_command)) {
        perror("Failed to write UART");
        close(uart_fd);
        return -1;
    }
    
    unsigned char current_reg_buffer[32];
    ssize_t current_reg_bytes_read = read(uart_fd, current_reg_buffer, sizeof(current_reg_buffer));
    if (current_reg_bytes_read < 0) {
        perror("Failed to read UART");
        close(uart_fd);
        return -1;
    }
    
    // 解析数据
    short current_reg_value = (current_reg_buffer[3] << 8) | current_reg_buffer[4];
    int CurrentREG = current_reg_value;
    
    // 读取电压参数寄存器的数值
    unsigned char voltage_param_command[] = {0x01, 0x04, 0x00, 0x03, 0x00, 0x02, 0x20, 0x0C};
    bytes_written = write(uart_fd, voltage_param_command, sizeof(voltage_param_command));
    if (bytes_written != sizeof(voltage_param_command)) {
        perror("Failed to write UART");
        close(uart_fd);
        return -1;
    }
    
    unsigned char voltage_param_buffer[32];
    ssize_t voltage_param_bytes_read = read(uart_fd, voltage_param_buffer, sizeof(voltage_param_buffer));
    if (voltage_param_bytes_read < 0) {
        perror("Failed to read UART");
        close(uart_fd);
        return -1;
    }
    
    // 解析数据
    short voltage_param_value = (voltage_param_buffer[3] << 8) | voltage_param_buffer[4];
    int VoltageParameterREG = voltage_param_value;
    
    // 读取电压寄存器的数值
    unsigned char voltage_reg_command[] = {0x01, 0x04, 0x00, 0x04, 0x00, 0x02, 0x51, 0xCA};
    bytes_written = write(uart_fd, voltage_reg_command, sizeof(voltage_reg_command));
    if (bytes_written != sizeof(voltage_reg_command)) {
        perror("Failed to write UART");
        close(uart_fd);
        return -1;
    }
    
    unsigned char voltage_reg_buffer[32];
    ssize_t voltage_reg_bytes_read = read(uart_fd, voltage_reg_buffer, sizeof(voltage_reg_buffer));
    if (voltage_reg_bytes_read < 0) {
        perror("Failed to read UART");
        close(uart_fd);
        return -1;
    }
    
    // 解析数据
    short voltage_reg_value = (voltage_reg_buffer[3] << 8) | voltage_reg_buffer[4];
    int VoltageREG = voltage_reg_value;
    

    /*                         */
    // 读取功率参数寄存器的数值

      unsigned char power_param_command[] = {0x01, 0x04, 0x00, 0x03, 0x00, 0x02, 0x30, 0x0A};
    bytes_written = write(uart_fd, power_param_command, sizeof(power_param_command));
    if (bytes_written != sizeof(power_param_command)) {
        perror("Failed to write UART");
        close(uart_fd);
        return -1;
    }
    
    unsigned char power_param_buffer[32];
    ssize_t power_param_bytes_read = read(uart_fd, power_param_buffer, sizeof(power_param_buffer));
    if (power_param_bytes_read < 0) {
        perror("Failed to read UART");
        close(uart_fd);
        return -1;
    }
    
    // 解析数据
    short power_param_value = (power_param_buffer[3] << 8) | power_param_buffer[4];
    int VPowerParameterREG = power_param_value;
    
     // 读取功率寄存器存器的数值
    unsigned char power_reg_command[] = {0x01, 0x04, 0x00, 0x04, 0x00, 0x02, 0x91, 0xCA};
    bytes_written = write(uart_fd, power_reg_command, sizeof(power_reg_command));
    if (bytes_written != sizeof(power_reg_command)) {
        perror("Failed to write UART");
        close(uart_fd);
        return -1;
    }
    
    unsigned char power_reg_buffer[32];
    ssize_t power_reg_bytes_read = read(uart_fd, power_reg_buffer, sizeof(power_reg_buffer));
    if (power_reg_bytes_read < 0) {
        perror("Failed to read UART");
        close(uart_fd);
        return -1;
    }
    // 解析数据
    short power_reg_value = (power_reg_buffer[3] << 8) | power_reg_buffer[4];
    int PowerREG = power_reg_value;
    
    // 打印读取的值
    printf("VPower Parameter Register: %d\n", VPowerParameterREG);
    printf("Power Register: %d\n", PowerREG);
    
    close(uart_fd);
    //计算有效电流电压和功率
    int voltage = (VoltageParameterREG/VoltageREG)*1.88;
    int current = CurrentParameterREG/CurrentREG;
    int power   = (VPowerParameterREG/PowerREG)*1.88*1;

    // 输出结果
    printf("Current Parameter: %d\n", CurrentParameterREG);
    printf("Current: %d\n", CurrentREG);
    printf("Voltage Parameter: %d\n", VoltageParameterREG);
    printf("Voltage: %d\n", VoltageREG);
    
    close(uart_fd);
    return 0;
}
