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

/* Versiyon numaralari — Major.Minor.Patch formatinda */
#define VERSIYON_MAJOR    (1U)  /* buyuk surum — API degisiklikleri        */
#define VERSIYON_MINOR    (0U)  /* kucuk surum — yeni ozellik eklendi      */
#define VERSIYON_PATCH    (2U)  /* yama — hata duzeltmesi                  */

/* Bit islem makrolari — n parametresi shift miktarini belirler */
#define BIT(n)            (1U << (n))       /* n. biti 1 yapar: BIT(3) = 0x08  */
#define BIT_MASK(n)       (BIT(n) - 1U)     /* n bitlik maske: BIT_MASK(4)=0x0F */

/* ADC sinir degerleri — 12-bit cozunurluk icin */
#define ADC_MAX           (4095U)  /* 2^12 - 1 — maksimum ADC cikisi       */
#define ADC_MIN           (0U)     /* minimum ADC cikisi                    */

/* Sicaklik sinir degerleri — isaretli tip, U suffix yok */
#define SICAKLIK_MAX      (125)    /* maksimum olcum araligi (C)            */
#define SICAKLIK_MIN      (-40)    /* minimum olcum araligi (C)             */

/* Bellek boyutu hesaplamalari — okunabilirlik icin KB makrosu kullanildi */
#define KB                (1024U)        /* 1 kilobyte = 1024 byte          */
#define FLASH_BOY         (64U * KB)     /* STM32F103C8 flash boyutu        */
#define RAM_BOY           (20U * KB)     /* STM32F103C8 SRAM boyutu         */

/* FONKSIYON BENZERI MAKROLAR */

/* Her parametre ayri parantez icinde — operator onceligi hatasinı onler */
/* Rule 20.7: (a) ve (b) parantezi zorunlu — MAX(x+1, y) gibi kullanim guvenli */
#define MAX(a, b)            (((a) > (b)) ? (a) : (b))   /* buyugu dondur  */
#define MIN(a, b)            (((a) < (b)) ? (a) : (b))   /* kucugu dondur  */
#define SINIRLA(x, lo, hi)   (MAX((lo), MIN((x), (hi)))) /* araliga kisitla */

/* Register bit islem makrolari — donanim suruculerinde yaygin kullanim */
#define BIT_SET(reg, bit)    ((reg) |=  (bit))   /* biti 1 yap              */
#define BIT_CLR(reg, bit)    ((reg) &= ~(bit))   /* biti 0 yap              */
#define BIT_TGL(reg, bit)    ((reg) ^=  (bit))   /* bit degerini tersle     */
#define BIT_RD(reg,  bit)    (((reg) & (bit)) != 0U) /* bit okunur, boolean */

/* Zaman birimi donusumleri — HAL_GetTick() 1ms cozunurluk saglar */
#define MS_TO_TICK(ms)    ((ms) * 1U)        /* milisaniye → tick           */
#define TICK_TO_MS(tick)  ((tick) / 1U)      /* tick → milisaniye           */

/* Aci donusumu — float hesaplama, sadece gerektiginde kullan */
#define DEG_TO_RAD(deg)   ((deg) * 3.14159f / 180.0f)

/* Dizi boyutu hesaplama — sabit kodlama yerine bu makro kullan            */
/* sizeof(arr)/sizeof(arr[0]) — tip degisirse otomatik guncellenir         */
#define ARRAY_SIZE(arr)   ((uint8_t)(sizeof(arr) / sizeof((arr)[0U])))

/* KOSULLU DERLEME */

/* Platform ve build tanimlari — normalde Makefile veya IDE'den gelir      */
/* -DSTM32F4XX ve -DDEBUG_AKTIF seklinde derleyiciye gecilir               */
#define STM32F4XX   /* hedef mikrodenetleyici ailesi                        */
#define DEBUG_AKTIF /* debug build — release icin bu satiri kaldir          */

/* DBG_PRINT — debug buildde printf, release buildde bos kalir             */
/* release buildde derleyici bu satirlari tamamen kaldirir, sifir maliyet  */
#ifdef DEBUG_AKTIF
    #define DBG_PRINT(fmt, ...)  printf(fmt, ##__VA_ARGS__) /* debug aktif  */
#else
    #define DBG_PRINT(fmt, ...)  /* bos — release buildde kod uretilmez     */
#endif

/* Optimizasyon seviyesi — debug ve release icin farkli deger */
#ifdef DEBUG_AKTIF
    #define OPTIMIZE_LEVEL  (0U)  /* -O0: optimizasyon yok, debug kolayligi */
#else
    #define OPTIMIZE_LEVEL  (2U)  /* -O2: release icin hiz optimizasyonu    */
#endif

/* PLATFORM SOYUTLAMA */

/* Farkli STM32 serilerinde pin ve frekans tanimlarini soyutla             */
/* Tek bir kaynak dosya birden fazla platformu destekleyebilir             */
#ifdef STM32F4XX
    #define LED_PORT      "GPIOD"   /* STM32F4 Discovery board LED portu   */
    #define LED_PIN       (12U)     /* PD12 — yesil LED                    */
    #define CPU_FREQ_MHZ  (168U)    /* maksimum sistem saati frekans       */
#elif defined(STM32F1XX)
    #define LED_PORT      "GPIOC"   /* STM32F103 Blue Pill LED portu       */
    #define LED_PIN       (13U)     /* PC13 — dahili LED                   */
    #define CPU_FREQ_MHZ  (72U)     /* maksimum sistem saati frekans       */
#else
    #error "Desteklenmeyen platform — STM32F4XX veya STM32F1XX tanimlanmali"
#endif

/* Derleyici spesifik attribute soyutlamasi                                */
/* GCC, IAR, Keil farkli syntax kullanir — tek noktadan yonetim saglar    */
#ifdef __GNUC__
    #define PACKED        __attribute__((packed))     /* struct padding kaldir */
    #define ALIGNED(n)    __attribute__((aligned(n))) /* n-byte hizalama       */
    #define WEAK          __attribute__((weak))       /* override edilebilir   */
    #define NO_RETURN     __attribute__((noreturn))   /* donmeyen fonksiyon    */
#else
    #define PACKED        /* diger derleyiciler icin bos birak */
    #define ALIGNED(n)
    #define WEAK
    #define NO_RETURN
#endif

/* HEADER GUARD YAPISI */

/* Her .h dosyasinin basinda olmali — ayni baslik iki kez include edilirse */
/* ikinci include atlanir, coklu tanim hatalari onlenir                    */

/* #ifndef SENSOR_H        — tanimli degil mi?                             */
/* #define SENSOR_H        — tanimla, bir dahaki include atlanir           */
/*                                                                         */
/*   #include <stdint.h>                                                   */
/*                                                                         */
/*   typedef struct { ... } Sensor_t;  — tip tanimlari                    */
/*   void sensor_init(void);           — fonksiyon prototipleri           */
/*                                                                         */
/* #endif  / * SENSOR_H * /  — guard sonu, dosya adi ile eslestir         */

/* FONKSIYON TANIMLARI */

static void makro_tanimlama(void)
{
    /* Versiyon bilgisi — her uc alan ayri makro, bagimsiz guncellenebilir */
    printf("Versiyon          : %u.%u.%u\n",
           (uint32_t)VERSIYON_MAJOR,
           (uint32_t)VERSIYON_MINOR,
           (uint32_t)VERSIYON_PATCH);

    /* Bellek boyutu — KB makrosu ile okunabilir tanim */
    printf("Flash boyutu      : %u byte\n", (uint32_t)FLASH_BOY); /* 65536 */
    printf("RAM boyutu        : %u byte\n", (uint32_t)RAM_BOY);   /* 20480 */

    /* Bit islem makrolarinin kullanimi — donanim register ornegi */
    uint8_t reg = 0x00U; /* baslangic — tum bitler sifir */

    BIT_SET(reg, BIT(3U)); /* bit 3'u set et — 0x00 → 0x08 */
    printf("BIT_SET(3)        : 0x%02X\n", (uint32_t)reg);

    BIT_CLR(reg, BIT(3U)); /* bit 3'u temizle — 0x08 → 0x00 */
    printf("BIT_CLR(3)        : 0x%02X\n", (uint32_t)reg);

    BIT_TGL(reg, BIT(5U)); /* bit 5'i tersle — 0x00 → 0x20 */
    printf("BIT_TGL(5)        : 0x%02X\n", (uint32_t)reg);

    /* MAX/MIN/SINIRLA — kopyala-yapistir yerine makro ile kod tekrari onlenir */
    uint8_t a = 45U;
    uint8_t b = 78U;
    printf("MAX(45,78)        : %u\n", (uint32_t)MAX(a, b));           /* 78  */
    printf("MIN(45,78)        : %u\n", (uint32_t)MIN(a, b));           /* 45  */
    printf("SINIRLA(200,0,100): %u\n", (uint32_t)SINIRLA(200U, 0U, 100U)); /* 100 */

    /* ARRAY_SIZE — dizi boyutu degisirse makro otomatik guncellenir */
    uint8_t dizi[8U] = {0U};
    printf("ARRAY_SIZE        : %u\n", (uint32_t)ARRAY_SIZE(dizi)); /* 8 */

    /* Zaman donusumu — HAL tabanli gecikme hesaplamalarinda kullanilir */
    printf("500ms tick        : %u\n", (uint32_t)MS_TO_TICK(500U)); /* 500 */
}

static void kosullu_derleme(void)
{
    /* Optimizasyon seviyesi — debug/release build farki gorulur */
    printf("Optimize seviyesi : %u\n", (uint32_t)OPTIMIZE_LEVEL); /* 0 */

    /* DBG_PRINT — DEBUG_AKTIF tanimli oldugu icin yazdirilir              */
    /* release buildde bu satir derlenmez, kod boyutu artmaz               */
    DBG_PRINT("Debug mesaji      : DEBUG_AKTIF tanimli ise gorulur\n");

    /* #if ile derleme zamani sayisal karsilastirma — runtime degil        */
    /* kosul saglanmazsa blok derlenmez, sifir maliyet                     */
    #if (VERSIYON_MAJOR >= 1U)
        printf("Versiyon          : 1.0 veya uzeri\n");
    #else
        printf("Versiyon          : 1.0 altı\n");
    #endif
}

static void header_guard(void)
{
    /* #pragma once MISRA uyumlu degil — #ifndef/#define/#endif kullan     */
    /* Bazi derleyiciler pragma once desteklemez, tasınabilirlik azalir    */
    printf("Header guard      : ifndef/define/endif yapisi kullan\n");
    printf("Pragma once       : MISRA uyumlu degil — ifndef tercih et\n");
}

static void platform_soyutlama(void)
{
    /* Hedef platforma gore derlenen degerler yazdirilir                   */
    /* STM32F4XX tanimli oldugu icin GPIOD ve 168MHz gorulur               */
    printf("LED port          : %s\n",     LED_PORT);
    printf("LED pin           : %u\n",     (uint32_t)LED_PIN);
    printf("CPU frekans       : %u MHz\n", (uint32_t)CPU_FREQ_MHZ);

    /* Derleyici tespiti — GCC'de __GNUC__ otomatik tanimlanir */
    #ifdef __GNUC__
        printf("Derleyici         : GCC\n");
    #else
        printf("Derleyici         : diger\n");
    #endif
}

static void yaygin_hatalar(void)
{
    /* 1) Parametresiz makro — operator onceligi hatasi                    */
    {
        /* #define KARE(x)  x*x   — YANLIS                                 */
        /* KARE(2+3) → 2+3*2+3 = 11, beklenen 25                          */
        /* #define KARE(x)  ((x)*(x)) — DOGRU, her parametre parantezli   */
        uint8_t a     = 3U;
        uint8_t b     = 4U;
        uint8_t dogru = ((a + b) * (a + b)); /* parantez ile dogru oncelik */
        printf("Makro parantez    : %u\n", (uint32_t)dogru); /* 49 */
    }

    /* 2) Coklu degerlendirme — makro parametresi iki kez calisabilir     */
    {
        /* MAX(i++, j++) — i veya j iki kez artirilir, yan etki olusur    */
        /* cozum: once gecici degiskene ata, sonra makroya gec            */
        uint8_t x     = 5U;
        uint8_t y     = 3U;
        uint8_t tmp_x = x; /* once gecici degiskene al — yan etki onlendi */
        uint8_t tmp_y = y;
        uint8_t maks  = MAX(tmp_x, tmp_y); /* guvenli kullanim             */
        printf("MAX(5,3)          : %u\n", (uint32_t)maks); /* 5 */
    }

    /* 3) #undef kullanimi — Rule 20.5 ihlali                             */
    {
        /* #define GECICI 10U                                              */
        /* #undef  GECICI — tanimsizlastirma MISRA'da yasak               */
        /* cozum: farkli isim kullan veya scope ile yasam suresi kisalt   */
        printf("undef             : Rule 20.5 ihlali — kullanma\n");
    }

    /* 4) Recursive makro — tanimsiz davranis, derleyici donguye girer    */
    {
        /* #define A  (A + 1U) — A, kendini cagirır, sonsuz genisleme     */
        /* cozum: makro kendi ismini icermemeli                           */
        printf("Recursive makro   : tanimsiz davranis — kullanma\n");
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