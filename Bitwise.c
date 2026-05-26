#include <stdint.h>
#include <stdio.h>

/*
 * Bitwise
 * MISRA-C:2012 Uygulamalari
 *
 * Kullanilan kurallar:
 *   Rule  7.2  — unsigned sabit literaller U suffix tasimali
 *   Rule  8.1  — tipler acikca belirtilmeli
 *   Rule  8.7  — sadece bir dosyadan erisilen nesne static olmali
 *   Rule 10.1  — operand esansiyel unsigned tipte olmali
 *   Rule 10.3  — atama hedef tip ile uyumlu olmali
 *   Rule 10.4  — aritmetik operandlar ayni esansiyel tipte olmali
 *   Rule 12.2  — shift miktari tip genisliginden kucuk olmali
 *   Rule 14.4  — if kosulu esansiyel boolean tipinde olmali
 */

/* PROTOTIP BILDIRIMLERI */

static void temel_operatorler(void);
static void bit_set_clear_toggle(void);
static void bit_okuma(void);
static void bit_maskeleme(void);
static void bit_aritmetigi(void);
static void union_bit_erisim(void);
static void yaygin_hatalar(void);

/* REGISTER BIT TANIMLARI */

/* Bit pozisyonlari macro ile tanimla — sihirli sayi kullanma */
#define BIT_0    (1U)        /* 0b00000001 */
#define BIT_1    (1U << 1U)  /* 0b00000010 */
#define BIT_2    (1U << 2U)  /* 0b00000100 */
#define BIT_3    (1U << 3U)  /* 0b00001000 */
#define BIT_4    (1U << 4U)  /* 0b00010000 */
#define BIT_5    (1U << 5U)  /* 0b00100000 */
#define BIT_6    (1U << 6U)  /* 0b01000000 */
#define BIT_7    (1U << 7U)  /* 0b10000000 */

/* GPIO register bit tanimlari — STM32 tarzi modelleme */
#define GPIO_PIN_0    (1U << 0U)
#define GPIO_PIN_1    (1U << 1U)
#define GPIO_PIN_2    (1U << 2U)
#define GPIO_PIN_3    (1U << 3U)

/* Status register flag tanimlari */
#define FLAG_READY    (1U << 0U)  /* bit 0 — hazir         */
#define FLAG_BUSY     (1U << 1U)  /* bit 1 — mesgul        */
#define FLAG_ERROR    (1U << 2U)  /* bit 2 — hata          */
#define FLAG_OVERFLOW (1U << 3U)  /* bit 3 — tasma         */

/* UNION BIT ERISIM YAPISI */

/* Register modellemesi — hem tam deger hem bit alanlari ile erisim */
typedef union {
    uint8_t tum;          /* 8-bitin tamami */
    struct {
        uint8_t hazir    : 1U; /* bit 0 */
        uint8_t mesgul   : 1U; /* bit 1 */
        uint8_t hata     : 1U; /* bit 2 */
        uint8_t tasma    : 1U; /* bit 3 */
        uint8_t reserved : 4U; /* bit 4-7 — kullanilmiyor */
    } bit;
} StatusReg_t;

/* TEMEL OPERATORLER */

static void temel_operatorler(void)
{
    uint8_t a = 0xACU; /* 0b10101100 */
    uint8_t b = 0x3FU; /* 0b00111111 */
    uint8_t sonuc;

    /* AND — her iki bitte de 1 ise sonuc 1 */
    sonuc = a & b;
    printf("AND  0xAC & 0x3F : 0x%02X\n", (uint32_t)sonuc); /* 0x2C */

    /* OR — herhangi birinde 1 ise sonuc 1 */
    sonuc = a | b;
    printf("OR   0xAC | 0x3F : 0x%02X\n", (uint32_t)sonuc); /* 0xBF */

    /* XOR — sadece biri 1 ise sonuc 1 — toggle icin kullanilir */
    sonuc = a ^ b;
    printf("XOR  0xAC ^ 0x3F : 0x%02X\n", (uint32_t)sonuc); /* 0x93 */

    /* NOT — tum bitleri tersle */
    sonuc = (uint8_t)(~a);  /* cast zorunlu — integer promotion onlendi */
    printf("NOT  ~0xAC       : 0x%02X\n", (uint32_t)sonuc); /* 0x53 */

    /* Sol kaydirma — 2 ile carpma gibi */
    sonuc = (uint8_t)(a << 1U); /* tip genisliginden kucuk kaydirma — Rule 12.2 */
    printf("SHL  0xAC << 1   : 0x%02X\n", (uint32_t)sonuc); /* 0x58 */

    /* Sag kaydirma — 2'ye bolme gibi */
    sonuc = (uint8_t)(a >> 1U);
    printf("SHR  0xAC >> 1   : 0x%02X\n", (uint32_t)sonuc); /* 0x56 */
}

/* BIT SET / CLEAR / TOGGLE */

static void bit_set_clear_toggle(void)
{
    uint8_t reg = 0x00U; /* baslangic — tum bitler 0 */

    /* Bit set — OR ile istenen biti 1 yap */
    reg |= FLAG_READY;                               /* bit 0 set */
    printf("Set   FLAG_READY  : 0x%02X\n", (uint32_t)reg); /* 0x01 */

    reg |= FLAG_BUSY;                                /* bit 1 set */
    printf("Set   FLAG_BUSY   : 0x%02X\n", (uint32_t)reg); /* 0x03 */

    /* Birden fazla bit ayni anda set */
    reg |= (FLAG_ERROR | FLAG_OVERFLOW);
    printf("Set   multi flag  : 0x%02X\n", (uint32_t)reg); /* 0x0F */

    /* Bit clear — AND + NOT ile istenen biti 0 yap */
    reg &= (uint8_t)(~FLAG_BUSY);                   /* bit 1 clear */
    printf("Clear FLAG_BUSY   : 0x%02X\n", (uint32_t)reg); /* 0x0D */

    /* Birden fazla bit ayni anda clear */
    reg &= (uint8_t)(~(FLAG_ERROR | FLAG_OVERFLOW));
    printf("Clear multi flag  : 0x%02X\n", (uint32_t)reg); /* 0x01 */

    /* Bit toggle — XOR ile istenen bitin degerini tersle */
    reg ^= FLAG_READY;                               /* 1 → 0 */
    printf("Toggle FLAG_READY : 0x%02X\n", (uint32_t)reg); /* 0x00 */

    reg ^= FLAG_READY;                               /* 0 → 1 */
    printf("Toggle FLAG_READY : 0x%02X\n", (uint32_t)reg); /* 0x01 */
}

/* BIT OKUMA */

static void bit_okuma(void)
{
    uint8_t reg = 0x05U; /* 0b00000101 — bit0 ve bit2 set */

    /* Tek bit okuma — maskeleme sonucu sifir degilse bit set */
    if ((reg & FLAG_READY) != 0U)    /* Rule 14.4 — != 0U ile karsilastir */
    {
        printf("FLAG_READY set    : evet\n");
    }

    if ((reg & FLAG_BUSY) == 0U)     /* bit clear kontrolu */
    {
        printf("FLAG_BUSY         : temiz\n");
    }

    /* Bit degerini 0 veya 1 olarak al */
    uint8_t ready_bit = (uint8_t)((reg & FLAG_READY) >> 0U); /* bit 0 */
    uint8_t error_bit = (uint8_t)((reg & FLAG_ERROR) >> 2U); /* bit 2 → 0 veya 1 */
    printf("ready_bit         : %u\n", (uint32_t)ready_bit); /* 1 */
    printf("error_bit         : %u\n", (uint32_t)error_bit); /* 1 */

    /* Birden fazla flag kontrolu */
    if ((reg & (FLAG_READY | FLAG_ERROR)) == (FLAG_READY | FLAG_ERROR))
    {
        printf("Ready ve Error    : ikisi de set\n");
    }
}

/* BIT MASKELEME */

/* Alan tanimlari — bir byte icindeki bit gruplari */
#define NIBBLE_LOW_MASK   (0x0FU)       /* bit 0-3 */
#define NIBBLE_HIGH_MASK  (0xF0U)       /* bit 4-7 */
#define NIBBLE_HIGH_SHIFT (4U)

/* Cihaz adresi — bit 4-6 */
#define DEV_ADDR_MASK     (0x70U)       /* 0b01110000 */
#define DEV_ADDR_SHIFT    (4U)

/* Veri alani — bit 0-3 */
#define DATA_MASK         (0x0FU)
#define DATA_SHIFT        (0U)

static void bit_maskeleme(void)
{
    uint8_t paket = 0x3AU; /* 0b00111010 — addr=3, data=10 */

    /* Alt nibble oku — bit 0-3 */
    uint8_t alt_nibble = (uint8_t)(paket & NIBBLE_LOW_MASK);
    printf("Alt nibble        : 0x%X\n", (uint32_t)alt_nibble); /* 0xA */

    /* Ust nibble oku — bit 4-7 */
    uint8_t ust_nibble = (uint8_t)((paket & NIBBLE_HIGH_MASK) >> NIBBLE_HIGH_SHIFT);
    printf("Ust nibble        : 0x%X\n", (uint32_t)ust_nibble); /* 0x3 */

    /* Alan okuma — adres bitlerini cikart */
    uint8_t adres = (uint8_t)((paket & DEV_ADDR_MASK) >> DEV_ADDR_SHIFT);
    printf("Cihaz adresi      : %u\n",   (uint32_t)adres); /* 3 */

    /* Alan yazma — mevcut bitleri koru, sadece hedef alani degistir */
    uint8_t yeni_adres = 5U;
    paket &= (uint8_t)(~DEV_ADDR_MASK);                          /* alani temizle */
    paket |= (uint8_t)((yeni_adres << DEV_ADDR_SHIFT) & DEV_ADDR_MASK); /* yeni degeri yaz */
    printf("Yeni paket        : 0x%02X\n", (uint32_t)paket);    /* 0x5A */

    /* Yeni adresi dogrula */
    adres = (uint8_t)((paket & DEV_ADDR_MASK) >> DEV_ADDR_SHIFT);
    printf("Yeni adres        : %u\n",   (uint32_t)adres); /* 5 */
}

/* BIT ARITMETIGI */

static void bit_aritmetigi(void)
{
    uint8_t  deger  = 12U;
    uint8_t  sonuc8;
    uint32_t sonuc32;

    /* Sol kaydirma = 2'nin kuvveti ile carpma */
    sonuc8 = (uint8_t)(deger << 1U); /* 12 * 2  = 24  */
    printf("12 << 1 (*2)      : %u\n", (uint32_t)sonuc8);

    sonuc8 = (uint8_t)(deger << 2U); /* 12 * 4  = 48  */
    printf("12 << 2 (*4)      : %u\n", (uint32_t)sonuc8);

    /* Sag kaydirma = 2'nin kuvveti ile bolme */
    sonuc8 = (uint8_t)(deger >> 1U); /* 12 / 2  = 6   */
    printf("12 >> 1 (/2)      : %u\n", (uint32_t)sonuc8);

    sonuc8 = (uint8_t)(deger >> 2U); /* 12 / 4  = 3   */
    printf("12 >> 2 (/4)      : %u\n", (uint32_t)sonuc8);

    /* Tek/cift kontrolu — bit 0 kontrolu bolmeden daha hizli */
    if ((deger & 1U) == 0U)
    {
        printf("%u cift sayidir\n", (uint32_t)deger);
    }

    /* 2'nin kuvveti kontrolu — gomulde buffer boyutu dogrulamada kullanilir */
    uint8_t test = 16U;
    if ((test != 0U) && (((uint8_t)(test & (uint8_t)(test - 1U))) == 0U))
    {
        printf("%u 2'nin kuvvetidir\n", (uint32_t)test);
    }

    /* Swap — gecici degisken olmadan XOR ile */
    uint8_t x = 0xAAU;
    uint8_t y = 0x55U;
    x ^= y;  /* x = x XOR y */
    y ^= x;  /* y = y XOR (x XOR y) = orijinal x */
    x ^= y;  /* x = (x XOR y) XOR orijinal x = orijinal y */
    printf("XOR swap x        : 0x%02X\n", (uint32_t)x); /* 0x55 */
    printf("XOR swap y        : 0x%02X\n", (uint32_t)y); /* 0xAA */

    /* Buyuk sayi icin kaydirma — uint32_t kullan */
    sonuc32 = (uint32_t)1U << 16U; /* 65536 — uint8_t'ye sigmaz */
    printf("1 << 16           : %u\n", (uint32_t)sonuc32);
}

/* UNION BIT ERISIM */

static void union_bit_erisim(void)
{
    StatusReg_t status;
    status.tum = 0x00U; /* tum bitleri sifirla */

    /* Bit alani ile yaz */
    status.bit.hazir  = 1U;
    status.bit.mesgul = 0U;
    status.bit.hata   = 1U;
    printf("Union (bit yazma) : 0x%02X\n", (uint32_t)status.tum); /* 0x05 */

    /* Tam deger ile yaz — tum bitleri ayni anda guncelle */
    status.tum = FLAG_READY | FLAG_OVERFLOW;
    printf("Union (tam yazma) : 0x%02X\n", (uint32_t)status.tum); /* 0x09 */

    /* Bit alani ile oku */
    if (status.bit.hazir == 1U)   /* Rule 14.4 — == 1U ile karsilastir */
    {
        printf("Hazir biti        : set\n");
    }

    if (status.bit.tasma == 1U)
    {
        printf("Tasma biti        : set\n");
    }

    /* Register degerini yazdir */
    printf("Status register   : 0x%02X\n", (uint32_t)status.tum);
}

/* YAYGIN HATALAR */

static void yaygin_hatalar(void)
{
    /* 1) Isaretli tip ile bitwise — Rule 10.1 ihlali */
    {
        /* int8_t a = -1;        */
        /* uint8_t b = a & 0xF0; — YANLIS: isaretli tipte bitwise */
        uint8_t a = 0xFFU;       /* isaretli yerine uint8_t kullan */
        uint8_t b = a & 0xF0U;
        printf("Isaretli onlendi  : 0x%02X\n", (uint32_t)b); /* 0xF0 */
    }

    /* 2) Shift tasma — Rule 12.2 ihlali */
    {
        /* uint8_t x = 1U << 8U;  — YANLIS: 8-bit tipte 8 kaydirma tanimsiz */
        uint8_t  x8  = (uint8_t)((uint16_t)1U << 7U); /* max 7 — guvenli */
        uint32_t x32 = 1U << 16U;                      /* genis tip kullan */
        printf("Shift guvenli     : 0x%02X\n", (uint32_t)x8);
        printf("Genis shift       : 0x%08X\n", (uint32_t)x32);
    }

    /* 3) NOT sonucu cast edilmemesi */
    {
        uint8_t a      = 0xAAU;
        /* uint8_t b = ~a;  — YANLIS: ~a int dondurebilir, cast gerekli */
        uint8_t b      = (uint8_t)(~a); /* cast zorunlu */
        printf("NOT cast          : 0x%02X\n", (uint32_t)b); /* 0x55 */
    }

    /* 4) Operator onceligi — parantez kullanilmamasi */
    {
        uint8_t reg  = 0x05U;
        /* if (reg & FLAG_READY == 1U)  — YANLIS: == onceligi & den yuksek */
        if ((reg & FLAG_READY) == 1U)   /* parantez zorunlu */
        {
            printf("Oncelik dogru     : evet\n");
        }
    }
}

/* MAIN */

int main(void)
{
    printf("/* TEMEL OPERATORLER */\n");
    temel_operatorler();

    printf("\n/* BIT SET / CLEAR / TOGGLE */\n");
    bit_set_clear_toggle();

    printf("\n/* BIT OKUMA */\n");
    bit_okuma();

    printf("\n/* BIT MASKELEME */\n");
    bit_maskeleme();

    printf("\n/* BIT ARITMETIGI */\n");
    bit_aritmetigi();

    printf("\n/* UNION BIT ERISIM */\n");
    union_bit_erisim();

    printf("\n/* YAYGIN HATALAR */\n");
    yaygin_hatalar();

    return 0;
}