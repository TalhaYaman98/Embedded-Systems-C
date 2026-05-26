#include <stdint.h>
#include <stdio.h>

typedef float float32_t; /* PC ortami icin — STM32'de arm_math.h icerir */

/*
 * Degiskenler
 * MISRA-C:2012 Uygulamalari
 *
 * Kullanilan kurallar:
 *   Rule  5.3  — ic scope degiskeni dis scope ile ayni ismi tasiyamaz
 *   Rule  6.1  — bit-field tipleri acikca belirtilmeli
 *   Rule  7.2  — unsigned sabit literaller U suffix tasimali
 *   Rule  8.1  — tipler acikca belirtilmeli
 *   Rule  8.7  — sadece bir dosyadan erisilen nesne static olmali
 *   Rule 10.3  — atama hedef tip ile uyumlu olmali
 *   Rule 10.4  — aritmetik operandlar ayni esansiyel tipte olmali
 *   Rule 14.2  — for dongusu iyi tanimlanmis olmali
 *   Rule 14.4  — if/while kosulu esansiyel boolean tipinde olmali
 */

/* TEMEL TIPLER */

/* Gomulede KULLANMA — platform bagimli, boyutu garantili degil */
/* int   sayac;    */
/* char  karakter; */

/* Gomulede KULLAN — sabit boyutlu, tasınabilir */
static uint8_t   byte_deger = 0xAAU;   /* 8-bit isaretsiz, U suffix zorunlu   */
static int8_t    sicaklik   = -25;     /* 8-bit isaretli, negatif deger        */
static uint16_t  adc_sonuc  = 0x0FFFU; /* 12-bit ADC icin ideal               */
static int16_t   hiz        = -1500;   /* isaretli 16-bit                      */
static uint32_t  timestamp  = 0U;      /* HAL_GetTick() donus tipi             */
static int32_t   konum      = -999999; /* encoder sayaci gibi                  */

/* INITIALIZATION */

/* Global/static degiskenler .bss segmentinde sifir baslatilir */
static uint8_t glob_a = 0U; /* garantili sifir, yine de acik baslatma iyi pratik */
static uint8_t stat_b = 0U; /* sadece bu dosyadan erislebilir                    */

static void fonksiyon(void)
{
    /* Yerel degiskenler stack'te — garbage value riski, acikca baslatilmali */
    uint8_t  yerel_y = 0U;                          /* acik baslatma zorunlu        */
    uint32_t sayac_f = 0U;                          /* global sayac ile isim catismasi onlendi */
    uint8_t  buf2[8] = {0U};                        /* tum elemanlar sifir          */
    uint8_t  buf3[8] = {1U, 2U, 3U, 0U, 0U, 0U, 0U, 0U}; /* geri kalan acik sifir */

    (void)yerel_y;
    (void)sayac_f;
    (void)buf2;
    (void)buf3;
}

/* SCOPE */

/* Modul genelinde kullanilan degiskenler — static ile dosya kapsaminda gizlendi */
static uint8_t paylasilan = 0U; /* diger .c dosyalarindan erisim engellendi */
static uint8_t gizli      = 0U; /* sadece bu dosya icinde gorunur           */

static void foo(void)
{
    uint8_t x = 5U; /* dis kapsam degiskeni */
    {
        uint8_t x_ic = 10U; /* ic kapsam — dis x ile ayni isim kullanilmadi */
        (void)x_ic;
    }
    (void)x;
}

static void bar(uint8_t len)
{
    uint8_t i; /* dongu degiskeni blok disinda tanimlandi */
    for (i = 0U; i < len; i++) /* dongu sayaci acik artis */
    {
        /* islem */
    }
}

/* SABIT TANIMLAMA */

/* #define — tip bilgisi yok, sadece metin ikamesi yapar */
#define MAX_DENEME  (3U)       /* parantez ve U suffix zorunlu */
#define PI_VAL      (3.14f)    /* float literal — double degil */
#define KARE(x)     ((x) * (x)) /* her parametre parantezli — oncelik hatasi onlendi */

/* const — tipli, debugger'da gorunur, Flash'a gider */
const uint8_t   MAX_DENEME_C = 3U;    /* #define yerine tercih edilmeli */
const float32_t PI_C         = 3.14f; /* float32_t ile tip guvenligi    */

/* enum — ilgili sabitleri gruplar, tip guvenligi saglar */
typedef enum {
    DURUM_BOSTA     = 0, /* baslangic durumu          */
    DURUM_CALISIYOR = 1, /* degerler acikca yazilmali */
    DURUM_HATA      = 2, /* hata durumu               */
    DURUM_COUNT     = 3  /* eleman sayisi — dizi boyutu icin kullan */
} MakineDurumu_t;

/* STRUCT */

/* Ilgili degiskenler struct ile gruplandı — dagitik global degisken yerine */
typedef struct {
    uint16_t  ham_deger; /* ADC ham okuma                    */
    float32_t voltaj;    /* hesaplanmis gerilim degeri        */
    uint8_t   gecerli;   /* 1 = gecerli okuma, 0 = hata       */
} AdcSensor_t;

static void sensor_isle(AdcSensor_t * const s) /* const pointer — s adresi degistirilemez */
{
    if (s->gecerli == 1U) /* boolean kontrolu == ile yapilmali */
    {
        /* s->voltaj kullan */
    }
}

/* MAIN */

int main(void)
{
    /* Temel tip ciktilari — cast ile printf uyumsuzlugu onlendi */
    printf("Sicaklik : %d C\n", (int32_t)sicaklik);
    printf("Hiz      : %d\n",   (int32_t)hiz);
    printf("Konum    : %ld\n",  konum);
    printf("ADC      : 0x%X\n", (uint32_t)adc_sonuc);

    /* Type casting */
    {
        uint8_t  a      = 200U;
        uint8_t  b      = 100U;
        uint16_t toplam = (uint16_t)a + (uint16_t)b;  /* once genislet — overflow onlendi */
        uint8_t  sonuc  = (uint8_t)(toplam & 0xFFU);  /* bilerek kesme — acik cast zorunlu */
        uint16_t dogru  = toplam;                      /* genisletilmis tipte dogru sonuc   */

        printf("\nOverflow : %u\n", (uint32_t)sonuc); /* 44  — overflow gosterimi */
        printf("Dogru    : %u\n",   (uint32_t)dogru); /* 300 — dogru sonuc        */

        uint16_t  adc_val = 2048U;
        float32_t volt    = (float32_t)adc_val / 4095.0f * 3.3f; /* ADC ham → gerilim donusumu */
        printf("Voltaj   : %.3f V\n", (double)volt);

        int32_t fark = (int32_t)b - (int32_t)a; /* isaretsiz cikarma — once isaretliye cast */
        printf("Fark     : %ld\n", fark);        /* -100 beklenen sonuc                      */

        uint8_t px = 0xFFU;
        uint8_t py = (uint8_t)((uint16_t)px + 1U); /* integer promotion acikca yapildi */
        printf("Promotion: %u\n", (uint32_t)py);   /* 0 — wraparound gosterimi         */
    }

    /* Enum kullanimi */
    {
        MakineDurumu_t durum = DURUM_BOSTA; /* enum tipli degisken — int degil */
        printf("\nDurum    : %d\n", (int32_t)durum);
    }

    /* Struct kullanimi */
    {
        AdcSensor_t sensor1 = {0U, 0.0f, 0U}; /* tum alanlar acikca baslatildi */

        sensor1.ham_deger = 2048U;                                         /* ADC simülasyonu     */
        sensor1.voltaj    = (float32_t)sensor1.ham_deger / 4095.0f * 3.3f; /* gerilim hesabi      */
        sensor1.gecerli   = 1U;                                             /* okuma gecerli isareti */

        printf("Sensor1  : ham=%u  volt=%.3f  gecerli=%u\n",
               (uint32_t)sensor1.ham_deger,
               (double)sensor1.voltaj,
               (uint32_t)sensor1.gecerli);

        sensor_isle(&sensor1);
    }

    /* Kullanilmayan fonksiyon ve degisken uyarilari susturuldu 
    (void)foo;    fonksiyon adı = pointer — void'e cast, uyarı susuyor 
    (void)foo();  bu farklı — fonksiyonu ÇAĞIRIR, dönüş değerini atar
    
    (void)x — bir değişken veya fonksiyonu "kullandım" olarak işaretlemenin C standardına uygun yolu.
    Neden gerekiyor: Derleyici, tanımlanıp hiç kullanılmayan değişken veya fonksiyon görünce uyarı verir:
    */
    (void)fonksiyon;
    (void)foo;
    (void)bar;
    (void)glob_a;
    (void)stat_b;
    (void)paylasilan;
    (void)gizli;
    (void)byte_deger;
    (void)timestamp;

    return 0;
}