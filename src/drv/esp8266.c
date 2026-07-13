#include "drv/esp8266.h"
#include "bsp/uart.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "stm32f1xx_hal.h"
/* ===== 内部接收环形缓冲区 ===== */
#define RX_BUF_SIZE 1024U

static uint8_t  rx_ring[RX_BUF_SIZE];
static volatile uint32_t rx_head = 0; // 写指针（中断写入）
static volatile uint32_t rx_tail = 0; // 读指针（主循环读取）
static volatile uint8_t rx_byte_isr;  /* 中断写入，必须 volatile */

ESP_Stats g_esp_stats = {0};

/* ===== 中断接口 ===== */
void ESP_UART_IRQHandler(void)
{
    rx_ring[rx_head % RX_BUF_SIZE] = rx_byte_isr;
    rx_head++;
    HAL_UART_Receive_IT(&ESP_UART, (uint8_t *)&rx_byte_isr, 1);
}

/* ===== 内部工具函数 ===== */

static void rx_clear(void)
{
    rx_tail = rx_head;
}

static uint32_t rx_available(void)
{
    return rx_head - rx_tail;
}

static uint8_t rx_pop(void)
{
    return rx_ring[(rx_tail++) % RX_BUF_SIZE];
}

static void esp_send_raw(const char *s)
{
    HAL_UART_Transmit(&ESP_UART, (const uint8_t *)s, strlen(s), 2000);
}

/**
 * 等待 buf 中出现 expect 字符串，超时返回 ESP_ERR_TIMEOUT
 * 期间如出现 "ERROR" 或 "FAIL" 立即返回 ESP_ERR_AT
 */
static ESP_Result esp_wait_for(const char *expect, uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    char acc[256];
    uint32_t idx = 0;

    while (HAL_GetTick() - start < timeout_ms) {
        while (rx_available() > 0 && idx < sizeof(acc) - 1) {
            acc[idx++] = (char)rx_pop();
            acc[idx]   = '\0';
        }
        if (strstr(acc, expect))   return ESP_OK;
        if (strstr(acc, "ERROR")) {
            return ESP_ERR_AT;
        }
        if (strstr(acc, "FAIL")) {
            UART_Printf("[ESP] FAIL cmd=\"%s\" raw=[%.*s]\r\n", expect, (int)idx, acc);
            return ESP_ERR_AT;
        }
        /* 服务端主动断连：不必等满超时 */
        if (strstr(acc, "CLOSED"))  return ESP_ERR_TIMEOUT;
    }
    if (idx == 0)
        UART_Printf("[ESP] TIMEOUT waiting \"%s\" — 无响应（检查接线/波特率）\r\n", expect);
    else
        UART_Printf("[ESP] TIMEOUT waiting \"%s\" raw=[%.*s]\r\n", expect, (int)idx, acc);
    return ESP_ERR_TIMEOUT;
}

/* 关闭 TCP 连接——连接已被服务端关闭时 AT+CIPCLOSE 会返回 ERROR，属正常，静默处理 */
static void esp_cipclose(void)
{
    rx_clear();
    esp_send_raw("AT+CIPCLOSE\r\n");
    /* 等 OK 或 ERROR，都可以继续，不打印错误 */
    uint32_t start = HAL_GetTick();
    char acc[64]; uint32_t idx = 0;
    while (HAL_GetTick() - start < 1000) {
        while (rx_available() > 0 && idx < sizeof(acc) - 1) {
            acc[idx++] = (char)rx_pop();
            acc[idx]   = '\0';
        }
        if (strstr(acc, "OK") || strstr(acc, "ERROR")) break;
    }
    HAL_Delay(200); // 等模块处理完关闭指令
}

/**
 * 发送 AT 命令并等待期望响应
 * 自动补\r\n
 */
static ESP_Result esp_cmd(const char *cmd, const char *expect, uint32_t timeout_ms)
{
    rx_clear();
    esp_send_raw(cmd);
    esp_send_raw("\r\n");
    return esp_wait_for(expect, timeout_ms);
}

/**
 * 继续读取数据直到超过 idle_ms 没有新数据（或缓冲满）
 * 用于读取HTTP响应
 */
static void esp_read_until_idle(char *buf, uint32_t buf_size, uint32_t max_wait_ms, uint32_t idle_ms)
{
    uint32_t total_start = HAL_GetTick();
    uint32_t last_rx     = HAL_GetTick();
    uint32_t idx = 0;

    while (HAL_GetTick() - total_start < max_wait_ms && idx < buf_size - 1) {
        if (rx_available() > 0) {
            buf[idx++] = (char)rx_pop();
            buf[idx]   = '\0';
            last_rx = HAL_GetTick();
        } else if (idx > 0 && HAL_GetTick() - last_rx > idle_ms) {
            break; // 有数据且空闲超过idle_ms，认为接收完毕
        }
    }
}

/* ===== 公开接口实现 ===== */

ESP_Result ESP_Init(void)
{
    /* ---- 底层 UART 初始化（幂等，重复调用无副作用） ---- */
    MX_USART3_UART_Init();

    /* ---- 硬件初始化：CH_PD / RST 引脚 ---- */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = ESP_CH_PD_PIN | ESP_RST_PIN;  /* PB8, PB9 */
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gpio);

    /* 硬件复位：RST 拉低 200ms 再拉高，CH_PD 保持高电平 */
    ESP_CH_ENABLE();
    ESP_RST_LOW();
    HAL_Delay(200);
    ESP_RST_HIGH();

    /* 启动中断接收，等待模块上电稳定 */
    HAL_UART_Receive_IT(&ESP_UART, (uint8_t *)&rx_byte_isr, 1);
    /* 等 "ready" 字符串，最长 5s；旧固件不发此字符串则兜底等 3s */
    if (esp_wait_for("ready", 5000) != ESP_OK)
        HAL_Delay(3000);
    rx_clear();

    // 测试 AT 通信
    if (esp_cmd("AT", "OK", ESP_TIMEOUT_SHORT) != ESP_OK)
        return ESP_ERR_AT;

    // 关闭回显，减少噪声
    esp_cmd("ATE0", "OK", ESP_TIMEOUT_SHORT);

    // 设置AP模式
    if (esp_cmd("AT+CWMODE=2", "OK", ESP_TIMEOUT_SHORT) != ESP_OK)
        return ESP_ERR_AT;

    // 建立热点
    char cwsap_cmd[96];
    snprintf(cwsap_cmd, sizeof(cwsap_cmd),
             "AT+CWSAP=\"%s\",\"%s\",6,3", AP_SSID, AP_PASS);
    if (esp_cmd(cwsap_cmd, "OK", ESP_TIMEOUT_LONG) != ESP_OK)
        return ESP_ERR_AT;

    // 单连接模式（STM32做客户端，每次POST后关闭）
    if (esp_cmd("AT+CIPMUX=0", "OK", ESP_TIMEOUT_SHORT) != ESP_OK)
        return ESP_ERR_AT;

    g_esp_stats.ap_ready = true;
    return ESP_OK;
}

/* ================================================================
 *  esp_http_post —— 通用 HTTP POST 助手（内部）
 *
 *  流程（AT 命令 5 步）：
 *    ┌─ AT+CIPSTART → 等 "CONNECT"
 *    ├─ AT+CIPSEND=N → 等 ">"
 *    ├─ 发 HTTP 请求体 → 等 "+IPD"
 *    ├─ 读响应（可选）
 *    └─ AT+CIPCLOSE
 *
 *  参数：
 *    path          HTTP 路径 (e.g. "/thd")
 *    body          JSON 字符串 (不含 HTTP 头，本函数会加)
 *    response_buf  非 NULL 时把响应字节流写这里，供调用方自己解析
 *    response_size response_buf 大小；传 0 表示不关心响应
 *
 *  返回 ESP_OK 或错误码。会累加 g_esp_stats。
 * ================================================================ */
static ESP_Result esp_http_post(const char *path, const char *body,
                                char *response_buf, uint32_t response_size)
{
    static char request[512];
    static char cmd_buf[48];

    int body_len = (int)strlen(body);
    int req_len = snprintf(request, sizeof(request),
        "POST %s HTTP/1.0\r\n"
        "Host: %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s",
        path, PHONE_IP, body_len, body);
    if (req_len <= 0 || (size_t)req_len >= sizeof(request))
        return ESP_ERR_AT;

    /* 1. 建立 TCP 连接 */
    snprintf(cmd_buf, sizeof(cmd_buf),
             "AT+CIPSTART=\"TCP\",\"%s\",%d", PHONE_IP, PHONE_PORT);
    rx_clear();
    esp_send_raw(cmd_buf);
    esp_send_raw("\r\n");
    if (esp_wait_for("CONNECT", ESP_TIMEOUT_SHORT) != ESP_OK) {
        esp_cipclose();
        g_esp_stats.fail_count++;
        g_esp_stats.last_send_ok = false;
        return ESP_ERR_TIMEOUT;
    }

    /* 2. 告知发送长度 */
    rx_clear();
    snprintf(cmd_buf, sizeof(cmd_buf), "AT+CIPSEND=%d", req_len);
    esp_send_raw(cmd_buf);
    esp_send_raw("\r\n");
    if (esp_wait_for(">", ESP_TIMEOUT_SHORT) != ESP_OK) {
        esp_cipclose();
        g_esp_stats.fail_count++;
        g_esp_stats.last_send_ok = false;
        return ESP_ERR_TIMEOUT;
    }

    /* 3. 发送 HTTP 请求 */
    rx_clear();
    HAL_UART_Transmit(&ESP_UART, (const uint8_t *)request, req_len, 2000);

    /* 4. 等 +IPD */
    if (esp_wait_for("+IPD", 6000) != ESP_OK) {
        esp_cipclose();
        g_esp_stats.fail_count++;
        g_esp_stats.last_send_ok = false;
        return ESP_ERR_TIMEOUT;
    }

    /* 5. 读响应（可选） */
    if (response_buf && response_size > 0) {
        memset(response_buf, 0, response_size);
        esp_read_until_idle(response_buf, response_size, 3000, 200);
    }

    /* 6. 关闭连接 */
    esp_cipclose();

    g_esp_stats.send_count++;
    g_esp_stats.last_send_ok = true;
    return ESP_OK;
}

ESP_Result ESP_PostSensor(float voltage, bool led_on, uint32_t light,
                          uint8_t temp,
                          float *out_thr, uint32_t *out_light_thr,
                          uint8_t *out_temp_thr)
{
    static char body[72];
    static char response[768];

    snprintf(body, sizeof(body),
             "{\"v\":%.3f,\"led\":%d,\"light\":%lu,\"temp\":%d}",
             voltage, led_on ? 1 : 0, (unsigned long)light, (int)temp);

    ESP_Result r = esp_http_post("/sensor", body, response, sizeof(response));
    if (r != ESP_OK) return r;

    /* 从响应解析阈值（可选字段） */
    char *json_start = strchr(response, '{');
    if (json_start) {
        if (out_thr) {
            char *k = strstr(json_start, "\"thr\":");
            if (k) { float v = strtof(k + 6, NULL);
                     if (v > 0.0f && v <= 3.3f) *out_thr = v; }
        }
        if (out_light_thr) {
            char *k = strstr(json_start, "\"light_thr\":");
            if (k) { long v = strtol(k + 12, NULL, 10);
                     if (v >= 0 && v <= 4095) *out_light_thr = (uint32_t)v; }
        }
        if (out_temp_thr) {
            char *k = strstr(json_start, "\"temp_thr\":");
            if (k) { long v = strtol(k + 11, NULL, 10);
                     if (v >= 0 && v <= 50) *out_temp_thr = (uint8_t)v; }
        }
    }
    return ESP_OK;
}

/* ================================================================
 *  ESP_PostThd —— 新项目 (THD 测量) 数据上报
 *
 *  POST /thd  body:
 *    {"f0":1000.50,"thd":0.324,
 *     "h":[1.0000,0.0324,0.0152,0.0056,0.0031],
 *     "p":[0.00,1.57,3.14,-1.57,0.00]}
 *
 *  手机端返回空 {} 即可；本函数不解析响应体。
 * ================================================================ */
ESP_Result ESP_PostThd(float f0_hz, float thd_percent,
                       const float *h5, const float *p5)
{
    if (!h5 || !p5) return ESP_ERR_PARAM;

    static char body[220];
    int n = snprintf(body, sizeof(body),
             "{\"f0\":%.2f,\"thd\":%.3f,"
             "\"h\":[%.4f,%.4f,%.4f,%.4f,%.4f],"
             "\"p\":[%.3f,%.3f,%.3f,%.3f,%.3f]}",
             (double)f0_hz, (double)thd_percent,
             (double)h5[0], (double)h5[1], (double)h5[2],
             (double)h5[3], (double)h5[4],
             (double)p5[0], (double)p5[1], (double)p5[2],
             (double)p5[3], (double)p5[4]);
    if (n <= 0 || (size_t)n >= sizeof(body)) return ESP_ERR_AT;

    return esp_http_post("/thd", body, NULL, 0);
}
