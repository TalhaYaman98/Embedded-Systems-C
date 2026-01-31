/*

Gömülü sistemler için C dili pratikleri. Stm32 mikrodenetleyicileri odaklý.

*/


// STM32’ye Özgü Ama Standart Gibi Kullanýlan Baþlýklar. Bunlar C standardý deðil, STM32 ekosisteminin temelidir.
#include "stm32f4xx.h"        // CMSIS, register tanýmlarý
#include "stm32f4xx_hal.h"    // HAL üst seviye sürücüler


#include <stdint.h>           // Sabit bit geniþliðinde veri tipleri
uint8_t  rxData;             // 8-bit veri, UART/SPI için
uint16_t adcValue;           // 12-bit ADC için güvenli taþýyýcý
uint32_t tick;               // SysTick / timer sayaçlarý


#include <stdbool.h>         // bool, true, false tanýmlar. C dilinde bool tipi yoktur, bu kütüphane saðlar. Bayrak mantýðýnda okunabilirliði ciddi artýrýr.
bool systemReady;            // Sistem hazýr mý bayraðý


#include <stddef.h>          // NULL, size_t gibi temel tanýmlar. Pointer tabanlý kodlarda gereklidir. Driver yazarken null kontrolü için sýk kullanýlýr.
uint8_t *pBuffer = NULL;     // Pointer baþlangýçta boþ

#include <string.h>          // memcpy, memset, strlen vb. DMA, buffer, frame yönetiminde sýk kullanýlýr. STM32’de stack ve performans etkisi olabilir. strcpy gibi fonksiyonlar genelde önerilmez.
uint8_t txBuffer[10];
memset(txBuffer, 0, sizeof(txBuffer));   // Buffer sýfýrlama


#include <stdio.h>           // printf, sprintf vb. <stdio.h> — Standart G/Ç (Sýnýrlý Kullaným). Genellikle debug amaçlý kullanýlýr. RAM ve Flash tüketimi yüksektir. Üretim kodunda çoðu zaman kapatýlýr.
printf("ADC: %d\n", adcValue);   // SWO veya UART debug çýktýsý


#include <stdlib.h>          // atoi, malloc, free vb. Genel Amaçlý Yardýmcý Fonksiyonlar. Embedded projelerde çok sýnýrlý kullanýlýr. malloc / free çoðu STM32 projesinde önerilmez Fragmentation riski vardýr.
int value = atoi("123");     // String › int dönüþümü

#include <math.h>            // sqrt, sin, cos vb. Matematik Fonksiyonlarý. Sensör ve kontrol algoritmalarýnda kullanýlýr. FPU yoksa ciddi performans maliyeti vardýr. Çoðu projede sabit nokta tercih edilir.
float rms = sqrt(25.0f);     // Karekök hesaplama


#include <assert.h>          // assert makrosu. Geliþtirme Aþamasý Kontroller. Hata ayýklamada faydalýdýr. Debug build’te açýk Release build’te kapatýlýr
assert(adcValue <= 4095);    // ADC sýnýr kontrolü


#include <limits.h>          // Veri tipi min/max deðerleri. Taþma (overflow) kontrolü için kullanýlýr.
uint8_t value;
if (value == UCHAR_MAX)      // 8-bit maksimum deðere ulaþýldý mý
{
    value = 0;               // Sayaç sýfýrlanýr
}


#include <ctype.h>           // isdigit, isalpha vb. Karakter Kontrolleri. UART üzerinden gelen ASCII veriler için faydalýdýr.
char rx;
if (isdigit(rx))             // Gelen karakter rakam mý
{
    /* iþlem */
}


#include <time.h>            // time_t tanýmý. Zaman Fonksiyonlarý (Sýnýrlý). Bare-metal STM32’de genelde kullanýlmaz. RTC + RTOS olmayan sistemlerde pratik karþýlýðý yoktur
time_t t;                    // RTOS varsa anlamlý olabilir


#include <errno.h>           // Hata kodlarý. POSIX benzeri katmanlarda veya middleware’de görülür.
int err = errno;             // Son hata kodu


#include <stdarg.h>          // va_list tanýmý. Deðiþken Argümanlý Fonksiyonlar. printf benzeri fonksiyon yazarken kullanýlýr. Debug log sistemlerinde kullanýlýr
void Log(const char *fmt, ...)
{
    /* deðiþken argüman iþleme */
}


#include <signal.h>          // Sinyal tanýmlarý. Sinyaller (Nadiren). Bare-metal STM32’de neredeyse hiç kullanýlmaz.


#include <float.h>           // Float sýnýrlarý. Sayýsal sýnýr kontrolü gereken durumlarda.
float v;
if (v > FLT_MAX)             // Float taþma kontrolü
{
    v = 0.0f;
}


#include <setjmp.h>          // setjmp / longjmp. Stack Atlama (Genelde Kaçýnýlýr). Gömülü sistemlerde önerilmez.


#include <stdatomic.h>       // Atomik tipler. Atomik Ýþlemler (Modern C). RTOS veya çok çekirdekli sistemlerde anlamlýdýr.
atomic_uint flag;            // Interrupt-safe bayrak


int main(int argc, char *argv[]) {
	
	printf("C dili pratikleri");
	return 0;
}
