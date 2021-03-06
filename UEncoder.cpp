/*
 * UEncoder.cpp
 *
 *  Created on: 2017年11月1日
 *      Author: Romeli
 */

#include <UEncoder.h>
#include <Misc/UDebug.h>

UEncoder* UEncoder::_Pool[4];
uint8_t UEncoder::_PoolSp = 0;

UEncoder::UEncoder(TIM_TypeDef* TIMx, UIT_Typedef& it) {
	_TIMx = TIMx;
	_IT = it;

	_ExCNT = 0;
	_RelativeDir = Dir_Positive;

	//自动将对象指针加入资源池
	_Pool[_PoolSp++] = this;
}

UEncoder::~UEncoder() {
}

/*
 * author Romeli
 * param void
 * return void
 */
void UEncoder::Init() {
	GPIOInit();
	TIMInit();
	ITInit();
	//顺序很重要
	TIM_Clear_Update_Flag(_TIMx);
	TIM_Enable_IT_Update(_TIMx);
	TIM_Enable(_TIMx);
}

/*
 * author Romeli
 * explain 初始化所有编码器模块
 * return void
 */
void UEncoder::InitAll() {
	//初始化池内所有单元
	for (uint8_t i = 0; i < _PoolSp; ++i) {
		_Pool[i]->Init();
	}
	if (_PoolSp == 0) {
		//Error @Romeli 无编码器模块
		UDebugOut("There have encoder module exsit");
	}
}

/*
 * author Romeli
 * explain 设置编码器方向，可以把脉冲反向
 * param dir 编码器方向
 * return void
 */
void UEncoder::SetRelativeDir(Dir_Typedef dir) {
	_RelativeDir = dir;
}

/*
 * author Romeli
 * explain 设置编码器位置
 * param pos 编码器位置
 * return void
 */
void UEncoder::SetPos(int32_t pos) {
	if (_RelativeDir == Dir_Negtive) {
		pos = -pos;
	}
	if (pos >= 0) {
		_ExCNT = uint16_t(pos / 0x10000);
		_TIMx->CNT = uint16_t(pos - (_ExCNT * 0x10000));
	} else {
		pos = -pos;
		_ExCNT = uint16_t(pos / 0x10000 + 1);
		_TIMx->CNT = uint16_t((_ExCNT * 0x10000) - pos);
	}
}

/*
 * author Romeli
 * explain 读取编码器当前位置
 * return int32_t 当前位置
 */
int32_t UEncoder::GetPos() const {
	int32_t pos = int32_t(_ExCNT) * 0x10000 + _TIMx->CNT;
	return _RelativeDir == Dir_Negtive ? -pos : pos;
}

/*
 * author Romeli
 * explain 中断处理函数，当定时器溢出时累加额外计数
 * return void
 */
void UEncoder::IRQ() {
	if (_TIMx->CNT <= 0x7fff) {
		++_ExCNT;
	} else {
		--_ExCNT;
	}
	TIM_Clear_Update_Flag(_TIMx);
}

void UEncoder::GPIOInit() {
	UDebugOut("This function should be override");
	/*	GPIO_InitTypeDef GPIO_InitStructure;

	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_9;
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	 GPIO_Init(GPIOA, &GPIO_InitStructure);*/
}

/*
 * author Romeli
 * explain 初始化定时器为编码器模式，4分频采样
 * return void
 */
void UEncoder::TIMInit() {
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

	TIMRCCInit();

	TIM_DeInit(_TIMx);

	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_Prescaler = 0;
	TIM_TimeBaseInitStructure.TIM_Period = 0xffff;
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(_TIMx, &TIM_TimeBaseInitStructure);

	TIM_EncoderInterfaceConfig(_TIMx, TIM_EncoderMode_TI12,
	TIM_ICPolarity_Falling, TIM_ICPolarity_Falling);

	TIM_ICInitTypeDef TIM_ICInitStructure;

	TIM_ICStructInit(&TIM_ICInitStructure);
	TIM_ICInitStructure.TIM_ICFilter = 1;
	TIM_ICInit(_TIMx, &TIM_ICInitStructure);
	_TIMx->CNT = 0;
}

void UEncoder::ITInit() {
	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = _IT.NVIC_IRQChannel;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPriority = _IT.PreemptionPriority;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
//			_IT.PreemptionPriority;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority =
//			_IT.SubPriority;
	NVIC_Init(&NVIC_InitStructure);

	TIM_ClearITPendingBit(_TIMx, TIM_IT_Update);
	TIM_ITConfig(_TIMx, TIM_IT_Update, ENABLE);
}

