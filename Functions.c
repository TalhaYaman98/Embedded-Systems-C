#include <stdint.h>
#include <stdio.h>

typedef float float32_t; /* PC ortami icin — STM32'de arm_math.h icerir */

/*
 * Fonksiyonlar 
 * MISRA-C:2012 Uygulamalari
 *
 * Kullanilan kurallar:
 *   Rule  5.3  — ic scope degiskeni dis scope ile ayni ismi tasiyamaz
 *   Rule  7.2  — unsigned sabit literaller U suffix tasimali
 *   Rule  8.1  — tipler acikca belirtilmeli
 *   Rule  8.4  — fonksiyon prototype ile tanim uyumlu olmali
 *   Rule  8.7  — sadece bir dosyadan erisilen fonksiyon static olmali
 *   Rule 15.4  — dongu icinde tek break olmali
 *   Rule 15.5  — fonksiyonun tek return noktasi olmali
 *   Rule 17.1  — stdarg.h kullanimi yasak
 *   Rule 17.3  — implicit fonksiyon tanimi yasak
 *   Rule 17.7  — non-void fonksiyon donus degeri kullanilmali
 */

/* PROTOTIP BILDIRIMLERI */

/* Tum fonksiyonlar kullanimdan once bildirilmeli — implicit tanim yasak */
static uint8_t  topla(uint8_t a, uint8_t b);
static void     led_toggle(uint8_t * const durum);
static uint8_t  arayi_topla(const uint8_t * const dizi, uint8_t boy);
static uint16_t adc_oku(void);
static void     sensor_guncelle(uint16_t ham, float32_t * const volt_out, uint8_t * const gecerli_out);
static uint8_t  sinirla(uint8_t deger, uint8_t min, uint8_t maks);
static void     islem_uygula(uint8_t * const dizi, uint8_t boy, uint8_t (*islem)(uint8_t));
static uint8_t  iki_kat(uint8_t x);
static uint8_t  bir_artir(uint8_t x);

/* FONKSIYON TANIMLAMA */

/* Deger donduren fonksiyon — parametreler degerle geciliyor */
static uint8_t topla(uint8_t a, uint8_t b)
{
    uint16_t sonuc = (uint16_t)a + (uint16_t)b; /* once genislet — overflow onlendi */
    uint8_t  ret;

    if (sonuc > (uint16_t)0xFFU)
    {
        ret = 0xFFU; /* sinir asiminda maksimum deger don — saturasyon */
    }
    else
    {
        ret = (uint8_t)sonuc; /* guvenli — aralik kontrolu yapildi */
    }

    return ret; /* tek return noktasi */
}

/* DONUS TIPLERI */

/* void — sadece islem yapar, deger dondurmez */
static void led_toggle(uint8_t * const durum) /* const pointer — adres degistirilemez */
{
    *durum ^= 1U; /* XOR ile toggle — 0→1, 1→0 */
}

/* Dizi isleme — const pointer ile salt okunur erisim */
static uint8_t arayi_topla(const uint8_t * const dizi, uint8_t boy)
{
    uint8_t  i;
    uint16_t toplam = 0U; /* ara toplam icin genisletilmis tip */
    uint8_t  ret;

    for (i = 0U; i < boy; i++)
    {
        toplam += (uint16_t)dizi[i]; /* her adimda genisletilmis tipte topla */
    }

    ret = (toplam > (uint16_t)0xFFU) ? 0xFFU : (uint8_t)toplam; /* saturasyon */

    return ret; /* tek return noktasi */
}

/* PARAMETRE GECISI */

/* By value — kopya olusturulur, orijinal degismez */
static uint8_t sinirla(uint8_t deger, uint8_t min, uint8_t maks)
{
    uint8_t ret;

    if (deger < min)
    {
        ret = min; /* alt sinir */
    }
    else if (deger > maks)
    {
        ret = maks; /* ust sinir */
    }
    else
    {
        ret = deger; /* aralik icinde */
    }

    return ret; /* tek return noktasi */
}

/* By pointer — orijinal veri uzerinde calisir, kopyalama yok */
static void sensor_guncelle(uint16_t ham, float32_t * const volt_out, uint8_t * const gecerli_out)
{
    if (ham <= (uint16_t)4095U) /* gecerli ADC araligi kontrolu */
    {
        *volt_out    = (float32_t)ham / 4095.0f * 3.3f; /* gerilim hesabi */
        *gecerli_out = 1U;                               /* gecerli okuma  */
    }
    else
    {
        *volt_out    = 0.0f; /* gecersiz — sifirla */
        *gecerli_out = 0U;   /* hata isareti       */
    }
}

/* ADC simülasyonu — void parametre acikca yazilmali */
static uint16_t adc_oku(void)
{
    return 2048U; /* STM32'de: return HAL_ADC_GetValue(&hadc1); */
}

/* INLINE FONKSIYON */

/* #define makro yerine static inline — tip guvenligi var, debug'da gorunur */
static inline uint8_t max_uint8(uint8_t a, uint8_t b)
{
    return (a > b) ? a : b; /* uclu operator — tek satir, inline icin uygun */
}

static inline uint8_t min_uint8(uint8_t a, uint8_t b)
{
    return (a < b) ? a : b;
}

/* CALLBACK — FONKSIYON POINTERI */

/* Callback tanimlari — fonksiyon imzasi typedef ile netlestirildi */
typedef uint8_t (*IslemFonk_t)(uint8_t); /* uint8_t alan, uint8_t donduren fonksiyon tipi */

/* Callback alan fonksiyon — dizi uzerinde verilen islemi uygular */
static void islem_uygula(uint8_t * const dizi, uint8_t boy, uint8_t (*islem)(uint8_t))
{
    uint8_t i;

    for (i = 0U; i < boy; i++)
    {
        dizi[i] = islem(dizi[i]); /* her elemana callback uygula */
    }
}

/* Callback olarak kullanilacak fonksiyonlar */
static uint8_t iki_kat(uint8_t x)
{
    uint16_t sonuc = (uint16_t)x * 2U;                              /* genislet */
    return (sonuc > (uint16_t)0xFFU) ? (uint8_t)0xFFU : (uint8_t)sonuc; /* saturasyon */
}

static uint8_t bir_artir(uint8_t x)
{
    return (x < (uint8_t)0xFFU) ? (uint8_t)(x + 1U) : (uint8_t)0xFFU; /* saturasyon */
}

/* OZYINELEME */

/* Gomulede recursive KULLANMA — stack tasması riski */
/* Asagidaki sadece kavram gostermek icin yazildi — gercek projede iteratif kullan */
static uint8_t faktoriyel_iteratif(uint8_t n) /* recursive yerine iteratif tercih */
{
    uint8_t i;
    uint8_t sonuc = 1U;

    for (i = 1U; i <= n; i++)
    {
        sonuc *= i; /* her adimda carp */
    }

    return sonuc; /* tek return noktasi */
}

/* MAIN */

int main(void)
{
    /* Fonksiyon tanimlama ve donus tipi */
    {
        uint8_t a      = 200U;
        uint8_t b      = 100U;
        uint8_t toplam = topla(a, b); /* donus degeri kullanilmali */

        printf("Toplam (saturasyon): %u\n", (uint32_t)toplam); /* 255 */

        uint8_t led = 0U;
        led_toggle(&led);
        printf("LED durumu         : %u\n", (uint32_t)led); /* 1 */
    }

    /* Parametre gecisi */
    {
        uint8_t dizi[5] = {10U, 20U, 200U, 30U, 40U};
        uint8_t boyut   = (uint8_t)(sizeof(dizi) / sizeof(dizi[0U]));
        uint8_t toplam  = arayi_topla(dizi, boyut);

        printf("Dizi toplami       : %u\n", (uint32_t)toplam);

        uint8_t sinirli = sinirla(250U, 0U, 100U);
        printf("Sinirlanmis deger  : %u\n", (uint32_t)sinirli); /* 100 */

        uint16_t  ham     = adc_oku();
        float32_t volt    = 0.0f;
        uint8_t   gecerli = 0U;
        sensor_guncelle(ham, &volt, &gecerli);
        printf("ADC voltaj         : %.3f V  gecerli=%u\n", (double)volt, (uint32_t)gecerli);
    }

    /* Inline fonksiyon */
    {
        uint8_t buyuk = max_uint8(45U, 78U);
        uint8_t kucuk = min_uint8(45U, 78U);
        printf("Max                : %u\n", (uint32_t)buyuk); /* 78 */
        printf("Min                : %u\n", (uint32_t)kucuk); /* 45 */
    }

    /* Callback */
    {
        uint8_t      dizi[4]    = {10U, 20U, 30U, 40U};
        uint8_t      boyut      = (uint8_t)(sizeof(dizi) / sizeof(dizi[0U]));
        IslemFonk_t  secilen    = iki_kat; /* fonksiyon pointer atamasi */

        islem_uygula(dizi, boyut, secilen);
        printf("Callback (iki_kat) : %u %u %u %u\n",
               (uint32_t)dizi[0U], (uint32_t)dizi[1U],
               (uint32_t)dizi[2U], (uint32_t)dizi[3U]); /* 20 40 60 80 */

        islem_uygula(dizi, boyut, bir_artir);
        printf("Callback (bir_art) : %u %u %u %u\n",
               (uint32_t)dizi[0U], (uint32_t)dizi[1U],
               (uint32_t)dizi[2U], (uint32_t)dizi[3U]); /* 21 41 61 81 */
    }

    /* Ozyineleme — iteratif alternatif */
    {
        uint8_t sonuc = faktoriyel_iteratif(5U);
        printf("5! (iteratif)      : %u\n", (uint32_t)sonuc); /* 120 */
    }

    return 0;
}