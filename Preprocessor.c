#include <stdint.h>
#include <stdio.h>

/*
 * Onislemci
 * MISRA-C:2012 Uygulamalari
 *
 * Kullanilan kurallar:
 *   Rule  7.2  — unsigned sabit literaller U suffix tasimali
 *   Rule  8.1  — tipler acikca belirtilmeli
 *   Rule 20.1  — #include oncesinde sadece #ifndef olmali
 *   Rule 20.2  — standart baslik dosyasi isimleri kullanilmamali
 *   Rule 20.3  — #include dosya adi gecerli olmali
 *   Rule 20.4  — macro ile anahtar kelime tanimi yapilmamali
 *   Rule 20.5  — #undef kullanimi yasak
 *   Rule 20.7  — macro parametresi parantez icinde olmali
 *   Rule 20.10 — # ve ## operatoru kullanimi kisitli
 *   Rule 20.11 — macro parametresi # veya ## ile kullanilmamali
 *   Rule 20.13 — #else #elif #endif kendi satirinda olmali
 *   Rule 20.14 — #if #ifdef #ifndef eslesmeli olmali
 */

/* PROTOTIP BILDIRIMLERI */

static void makro_tanimlama(void);
static void kosullu_derleme(void);
static void header_guard(void);
static void platform_soyutlama(void);
static void yaygin_hatalar(void);

/* BASIT SABIT MAKROLAR */

/* Tip bilgisi olmayan sabit — sadece metin ikamesi */
#define VERSIYON_MAJOR    (1U)
#define VERSIYON_MINOR    (0U)
#define VERSIYON_PATCH    (2U)

/* Bit tanimlari */
#define BIT(n)            (1U << (n))  /* n. biti set et */
#define BIT_MASK(n)       (BIT(n) - 1U) /* n bitlik maske */

/* Sinir tanimlari */
#define ADC_MAX           (4095U)  /* 12-bit ADC maksimum */
#define ADC_MIN           (0U)
#define SICAKLIK_MAX      (125)    /* isaretli — U yok */
#define SICAKLIK_MIN      (-40)

/* Bellek boyutlari */
#define KB                (1024U)
#define FLASH_BOY         (64U * KB)   /* 64 KB */
#define RAM_BOY           (20U * KB)   /* 20 KB */

/* FONKSIYON BENZERI MAKROLAR */

/* Her parametre parantez icinde — Rule 20.7 */
#define MAX(a, b)         (((a) > (b)) ? (a) : (b))
#define MIN(a, b)         (((a) < (b)) ? (a) : (b))
#define SINIRLA(x, lo, hi) (MAX((lo), MIN((x), (hi))))

/* Bit islemleri */
#define BIT_SET(reg, bit)    ((reg) |=  (bit))
#define BIT_CLR(reg, bit)    ((reg) &= ~(bit))
#define BIT_TGL(reg, bit)    ((reg) ^=  (bit))
#define BIT_RD(reg,  bit)    (((reg) & (bit)) != 0U)

/* Birim donusumleri */
#define MS_TO_TICK(ms)    ((ms) * 1U)        /* HAL_GetTick() 1ms resolusyon */
#define TICK_TO_MS(tick)  ((tick) / 1U)
#define DEG_TO_RAD(deg)   ((deg) * 3.14159f / 180.0f)

/* Dizi boyutu — sizeof ile guvenlii */
#define ARRAY_SIZE(arr)   ((uint8_t)(sizeof(arr) / sizeof((arr)[0U])))

/* KOSULLU DERLEME */

/* Platform tanimlari — derleyici veya Makefile'dan gelir */
#define STM32F4XX         /* bu satir normalde Makefile'da tanimlanir */
#define DEBUG_AKTIF       /* debug build icin */

/* DEBUG makrosu — release buildde sifir maliyetli */
#ifdef DEBUG_AKTIF
    #define DBG_PRINT(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#else
    #define DBG_PRINT(fmt, ...)  /* bos — derleyici kaldirir */
#endif

/* Optimizasyon seviyesi */
#ifdef DEBUG_AKTIF
    #define OPTIMIZE_LEVEL  (0U)  /* debug: optimizasyon yok */
#else
    #define OPTIMIZE_LEVEL  (2U)  /* release: -O2 */
#endif

/* PLATFORM SOYUTLAMA */

/* Farkli STM32 serilerinde port isimlerini soyutla */
#ifdef STM32F4XX
    #define LED_PORT      "GPIOD"
    #define LED_PIN       (12U)
    #define CPU_FREQ_MHZ  (168U)
#elif defined(STM32F1XX)
    #define LED_PORT      "GPIOC"
    #define LED_PIN       (13U)
    #define CPU_FREQ_MHZ  (72U)
#else
    #error "Desteklenmeyen platform — STM32F4XX veya STM32F1XX tanimlanmali"
#endif

/* Derleyici spesifik attribute soyutlama */
#ifdef __GNUC__
    #define PACKED        __attribute__((packed))
    #define ALIGNED(n)    __attribute__((aligned(n)))
    #define WEAK          __attribute__((weak))
    #define NO_RETURN     __attribute__((noreturn))
#else
    #define PACKED
    #define ALIGNED(n)
    #define WEAK
    #define NO_RETURN
#endif

/* HEADER GUARD YAPISI */

/* Her .h dosyasinin basinda olmali — coklu include onler */
/* Asagidaki yapi gercekte ayri bir .h dosyasinda olur    */

/* #ifndef SENSOR_H                                       */
/* #define SENSOR_H                                       */
/*                                                        */
/*   #include <stdint.h>                                  */
/*                                                        */
/*   typedef struct { ... } Sensor_t;                     */
/*   void sensor_init(void);                              */
/*                                                        */
/* #endif  / * SENSOR_H * /                               */

/* FONKSIYON TANIMLARI */

static void makro_tanimlama(void)
{
    /* Sabit makrolar */
    printf("Versiyon          : %u.%u.%u\n",
           (uint32_t)VERSIYON_MAJOR,
           (uint32_t)VERSIYON_MINOR,
           (uint32_t)VERSIYON_PATCH);

    printf("Flash boyutu      : %u byte\n", (uint32_t)FLASH_BOY);
    printf("RAM boyutu        : %u byte\n", (uint32_t)RAM_BOY);

    /* Bit makrolari */
    uint8_t reg = 0x00U;
    BIT_SET(reg, BIT(3U));
    printf("BIT_SET(3)        : 0x%02X\n", (uint32_t)reg); /* 0x08 */

    BIT_CLR(reg, BIT(3U));
    printf("BIT_CLR(3)        : 0x%02X\n", (uint32_t)reg); /* 0x00 */

    BIT_TGL(reg, BIT(5U));
    printf("BIT_TGL(5)        : 0x%02X\n", (uint32_t)reg); /* 0x20 */

    /* Fonksiyon benzeri makrolar */
    uint8_t a = 45U;
    uint8_t b = 78U;
    printf("MAX(45,78)        : %u\n", (uint32_t)MAX(a, b));      /* 78 */
    printf("MIN(45,78)        : %u\n", (uint32_t)MIN(a, b));      /* 45 */
    printf("SINIRLA(200,0,100): %u\n", (uint32_t)SINIRLA(200U, 0U, 100U)); /* 100 */

    /* Dizi boyutu */
    uint8_t dizi[8U] = {0U};
    printf("ARRAY_SIZE        : %u\n", (uint32_t)ARRAY_SIZE(dizi)); /* 8 */

    /* Birim donusum */
    printf("500ms tick        : %u\n", (uint32_t)MS_TO_TICK(500U));
}

static void kosullu_derleme(void)
{
    printf("Optimize seviyesi : %u\n", (uint32_t)OPTIMIZE_LEVEL);

    /* DEBUG_AKTIF tanimli oldugu icin bu yazdirilir */
    DBG_PRINT("Debug mesaji      : goruluyorsa DEBUG_AKTIF tanimli\n");

    /* #if ile sayisal karsilastirma */
    #if (VERSIYON_MAJOR >= 1U)
        printf("Versiyon          : 1.0 veya uzeri\n");
    #else
        printf("Versiyon          : 1.0 altı\n");
    #endif
}

static void header_guard(void)
{
    /* Header guard ornegi — aciklama olarak gosterildi */
    printf("Header guard      : ifndef/define/endif yapisi\n");
    printf("Pragma once       : MISRA uyumlu degil — ifndef kullan\n");
}

static void platform_soyutlama(void)
{
    printf("LED port          : %s\n",   LED_PORT);
    printf("LED pin           : %u\n",   (uint32_t)LED_PIN);
    printf("CPU frekans       : %u MHz\n", (uint32_t)CPU_FREQ_MHZ);

    #ifdef __GNUC__
        printf("Derleyici         : GCC\n");
    #else
        printf("Derleyici         : diger\n");
    #endif
}

static void yaygin_hatalar(void)
{
    /* 1) Parametresiz makro — yan etki riski */
    {
        /* #define KARE(x)  x*x              — YANLIS */
        /* KARE(a+b) → a+b*a+b — yanlis oncelik        */
        /* #define KARE(x)  ((x)*(x))        — DOGRU   */
        uint8_t a      = 3U;
        uint8_t b      = 4U;
        uint8_t yanlis = (a + b) * (a + b); /* parantez eksik simule */
        uint8_t dogru  = ((a + b) * (a + b));
        printf("Makro parantez    : yanlis=%u dogru=%u\n",
               (uint32_t)yanlis, (uint32_t)dogru); /* ikisi esit bu ornekte */
        (void)yanlis;
        (void)dogru;
    }

    /* 2) Coklu degerlendirme — makro parametresi iki kez calisir */
    {
        /* #define MAX(a,b) ((a)>(b)?(a):(b))           */
        /* MAX(i++, j++) — i veya j iki kez artar        */
        /* cozum: fonksiyon kullan veya degiskene al     */
        uint8_t x = 5U;
        uint8_t y = 3U;
        uint8_t tmp_x = x; /* once degiskene al */
        uint8_t tmp_y = y;
        uint8_t maks  = MAX(tmp_x, tmp_y); /* guvenli */
        printf("MAX(5,3)          : %u\n", (uint32_t)maks); /* 5 */
    }

    /* 3) #undef kullanimi — Rule 20.5 ihlali */
    {
        /* #define GECICI 10U */
        /* #undef  GECICI     — MISRA ihlali */
        /* cozum: farkli isim kullan veya scope ile sinirla */
        printf("undef             : Rule 20.5 — kullanma\n");
    }

    /* 4) Recursive makro — tanimsiz davranis */
    {
        /* #define A  (A + 1U)  — sonsuz genisleme */
        /* cozum: makro kendi kendini cagirmasin    */
        printf("Recursive makro   : tanimsiz — kullanma\n");
    }
}

/* MAIN */

int main(void)
{
    printf("/* MAKRO TANIMLAMA */\n");
    makro_tanimlama();

    printf("\n/* KOSULLU DERLEME */\n");
    kosullu_derleme();

    printf("\n/* HEADER GUARD */\n");
    header_guard();

    printf("\n/* PLATFORM SOYUTLAMA */\n");
    platform_soyutlama();

    printf("\n/* YAYGIN HATALAR */\n");
    yaygin_hatalar();

    return 0;
}