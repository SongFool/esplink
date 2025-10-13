#include <stdint.h>
#include <stdbool.h>

// 状态类型定义
typedef enum {
    SWD_IDLE,
    SWD_SENDING_DATA,
    SWD_READING_DATA,
    SWD_TURNAROUND,
} SWD_State_t;

// 状态处理函数类型定义
typedef void (*SWD_StateHandler)(void);

#define HIGH 1
#define LOW  0


// 精简的SWD上下文结构体
typedef struct {
    bool done;
    SWD_State_t state;
    // 复用计数器
    uint8_t phase_counter;
    
    // 数据存储
    uint32_t    tx_data;        // 待发送数据
    uint32_t    rx_data;        // 接收数据
    uint8_t     need_send_len;
    uint8_t     need_recv_len;
    
    void (* write)(uint8_t bit);
    uint8_t (* read)(void);

} SWD_Context_t;


void _send_data(SWD_Context_t * p_ctx)
{
    if(p_ctx->need_send_len == 0) return;
    if(p_ctx->phase_counter < p_ctx->need_send_len){
        uint8_t bit = (p_ctx->tx_data >> p_ctx->phase_counter) & 0x1;
        p_ctx->write(bit);
    }
    p_ctx->phase_counter++;
    if(p_ctx->phase_counter >= p_ctx->need_send_len){
       p_ctx->phase_counter = 0; 
       p_ctx->done = true;
    }
    
}

void _recv_data(SWD_Context_t * p_ctx)
{
    if(p_ctx->need_recv_len == 0) return;
    if(p_ctx->phase_counter < p_ctx->need_recv_len){
        uint8_t bit =  p_ctx->read();
        p_ctx->rx_data |= ((uint32_t)bit << p_ctx->phase_counter); 
    }
    p_ctx->phase_counter++;
    if(p_ctx->phase_counter >= p_ctx->need_recv_len){
       p_ctx->phase_counter = 0; 
       p_ctx->done = true;
    }
}
void _wait_turnaround(SWD_Context_t * p_ctx)
{
    p_ctx->done = true;
}
void  SWD_RisingEdge_Handler(SWD_Context_t * p_ctx)
{
    switch (p_ctx->state) {
        case SWD_SENDING_DATA:
            _send_data(p_ctx);
            if(p_ctx->done == true){
                p_ctx->state = SWD_IDLE;
            }
            break;
        case SWD_TURNAROUND:
            p_ctx->state = SWD_IDLE;
            _wait_turnaround(p_ctx);
            break;
        case SWD_READING_DATA:
            _recv_data(p_ctx);
            if(p_ctx->done == true){
                p_ctx->state = SWD_IDLE;
            }
            break;
        case SWD_IDLE:
            break;
    }
    //协议函数切换
}


void SWD_send_data(SWD_Context_t * p_ctx,uint8_t data,uint8_t bit_len)
{
    p_ctx->state = SWD_SENDING_DATA;
    p_ctx->tx_data = data;
    p_ctx->need_send_len = bit_len;
    p_ctx->done = false;
}
void SWD_wait_turnaround(SWD_Context_t * p_ctx)
{
    p_ctx->state = SWD_TURNAROUND;
    p_ctx->done = false;
}
void SWD_recv_data(SWD_Context_t * p_ctx,uint8_t bit_len)
{
    p_ctx->state = SWD_READING_DATA;
    p_ctx->rx_data = 0;
    p_ctx->need_send_len = bit_len;
    p_ctx->done = false;
}
