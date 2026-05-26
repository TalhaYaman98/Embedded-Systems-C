#include <stdint.h>
#include <stdio.h>

/*
 * Kontrol Yapilari
 * MISRA-C:2012 Uygulamalari
 *
 * Kullanilan kurallar:
 *   Rule  7.2  — unsigned sabit literaller U suffix tasimali
 *   Rule  8.1  — tipler acikca belirtilmeli
 *   Rule  8.7  — sadece bir dosyadan erisilen nesne static olmali
 *   Rule 10.1  — operand esansiyel unsigned tipte olmali
 *   Rule 14.1  — dongu govdesi en az bir kez calisabilir olmali
 *   Rule 14.2  — for dongusu iyi tanimlanmis olmali
 *   Rule 14.3  — kontrol ifadesi degismez olmamali
 *   Rule 14.4  — if/while kosulu esansiyel boolean tipinde olmali
 *   Rule 15.2  — switch default etiketi olmali
 *   Rule 15.3  — switch default son etiket olmali
 *   Rule 15.4  — switch case fall-through olmamali
 *   Rule 15.5  — fonksiyonun tek return noktasi olmali
 *   Rule 15.6  — if/else/for/while govdesi her zaman blok olmali
 *   Rule 15.7  — if-else zinciri else ile bitmeli
 */

/* PROTOTIP BILDIRIMLERI */

static void if_else(void);
static void switch_yapi(void);
static void for_dongusu(void);
static void while_dongusu(void);
static void do_while_dongusu(void);
static void ic_ice_dongu(void);
static void yaygin_hatalar(void);

/* DURUM TANIMLARI */

typedef enum {
    MOD_BOSTA      = 0U,
    MOD_CALISIYOR  = 1U,
    MOD_HATA       = 2U,
    MOD_COUNT      = 3U
} Mod_t;

/* IF / ELSE */

static void if_else(void)
{
    uint8_t sicaklik = 85U;
    uint8_t ret      = 0U;

    /* Tek if — govde her zaman blok olmali */
    if (sicaklik > 100U)   /* Rule 14.4 — kosul boolean esansiyel tipte */
    {
        ret = 3U; /* kritik */
    }
    else if (sicaklik > 80U)
    {
        ret = 2U; /* uyari */
    }
    else if (sicaklik > 60U)
    {
        ret = 1U; /* normal ustu */
    }
    else
    {
        ret = 0U; /* normal — Rule 15.7: zincir else ile bitmeli */
    }

    printf("Sicaklik seviyesi : %u\n", (uint32_t)ret); /* 2 */

    /* Aralik kontrolu */
    uint8_t adc = 128U;
    if ((adc >= 10U) && (adc <= 245U)) /* parantez — operator onceligi */
    {
        printf("ADC gecerli aralik: evet\n");
    }
    else
    {
        printf("ADC gecerli aralik: hayir\n");
    }

    /* Boolean flag kontrolu */
    uint8_t hazir = 1U;
    if (hazir == 1U)   /* Rule 14.4 — if(hazir) degil, == 1U ile karsilastir */
    {
        printf("Sistem hazir      : evet\n");
    }
    else
    {
        printf("Sistem hazir      : hayir\n");
    }
}

/* SWITCH */

static void switch_yapi(void)
{
    Mod_t mod = MOD_CALISIYOR;

    switch (mod)
    {
        case MOD_BOSTA:
        {
            printf("Mod               : bosta\n");
            break; /* Rule 15.4 — her case break ile bitmeli */
        }
        case MOD_CALISIYOR:
        {
            printf("Mod               : calisiyor\n");
            break;
        }
        case MOD_HATA:
        {
            printf("Mod               : hata\n");
            break;
        }
        default:
        {
            /* Rule 15.2 — default olmali */
            /* Rule 15.3 — default en sonda olmali */
            printf("Mod               : bilinmiyor\n");
            break;
        }
    }

    /* Birden fazla case ayni islemi yapiyorsa */
    uint8_t kanal = 2U;
    switch (kanal)
    {
        case 1U:
        case 2U:
        case 3U:
        {
            /* fall-through KASITLI — sadece bu sekilde kabul edilir */
            printf("Kanal             : gecerli (%u)\n", (uint32_t)kanal);
            break;
        }
        default:
        {
            printf("Kanal             : gecersiz\n");
            break;
        }
    }
}

/* FOR DONGUSU */

static void for_dongusu(void)
{
    uint8_t i;
    uint8_t dizi[8] = {10U, 20U, 30U, 40U, 50U, 60U, 70U, 80U};
    uint8_t boyut   = (uint8_t)(sizeof(dizi) / sizeof(dizi[0U]));
    uint8_t toplam  = 0U;

    /* Klasik for — Rule 14.2: baslangic, kosul, artis net olmali */
    for (i = 0U; i < boyut; i++)
    {
        toplam += dizi[i]; /* dizi toplami */
    }
    printf("Dizi toplami      : %u\n", (uint32_t)toplam); /* 360 */

    /* Geriye dogru sayim */
    for (i = boyut; i > 0U; i--)
    {
        /* i-- sonra 0U kontrolu — underflow onlendi */
        printf("%u ", (uint32_t)dizi[i - 1U]);
    }
    printf("\n");

    /* Adim atlamali for */
    for (i = 0U; i < boyut; i += 2U)
    {
        printf("dizi[%u]=%u ", (uint32_t)i, (uint32_t)dizi[i]);
    }
    printf("\n");
}

/* WHILE DONGUSU */

static void while_dongusu(void)
{
    uint8_t  sayac   = 0U;
    uint8_t  timeout = 10U;
    uint8_t  ret     = 0U;

    /* Timeout korumal while — sonsuz dongu riski onlendi */
    while ((sayac < 5U) && (timeout > 0U))
    {
        sayac++;
        timeout--;
        printf("Sayac             : %u\n", (uint32_t)sayac);
    }

    /* Timeout kontrolu */
    if (timeout == 0U)
    {
        ret = 1U; /* timeout olustu */
        printf("Timeout           : olustu\n");
    }
    else
    {
        ret = 0U; /* normal cikis */
        printf("Timeout           : olmadi\n");
    }

    (void)ret;
}

/* DO-WHILE DONGUSU */

static void do_while_dongusu(void)
{
    uint8_t deneme  = 0U;
    uint8_t basarili = 0U;

    /* do-while — govde en az bir kez calisir */
    /* Gomulde: ilk okuma, sonra kontrol gerektiren durumlar */
    do
    {
        deneme++;
        printf("Deneme            : %u\n", (uint32_t)deneme);

        if (deneme == 3U) /* 3. denemede basarili */
        {
            basarili = 1U;
        }
        else
        {
            basarili = 0U;
        }

    } while ((basarili == 0U) && (deneme < 5U));

    printf("Sonuc             : %s\n", (basarili == 1U) ? "basarili" : "basarisiz");
}

/* IC ICE DONGU */

static void ic_ice_dongu(void)
{
    uint8_t satir;
    uint8_t sutun;
    uint8_t matris[4U][4U];
    uint8_t deger = 0U;

    /* Matris doldurma — ic ice for */
    for (satir = 0U; satir < 4U; satir++)
    {
        for (sutun = 0U; sutun < 4U; sutun++)
        {
            matris[satir][sutun] = deger;
            deger++;
        }
    }

    /* Matris yazdir */
    for (satir = 0U; satir < 4U; satir++)
    {
        for (sutun = 0U; sutun < 4U; sutun++)
        {
            printf("%3u", (uint32_t)matris[satir][sutun]);
        }
        printf("\n");
    }
}

/* YAYGIN HATALAR */

static void yaygin_hatalar(void)
{
    /* 1) Atama vs karsilastirma */
    {
        uint8_t x = 5U;
        /* if (x = 10U) { }  — YANLIS: atama, karsilastirma degil */
        if (x == 10U)         /* dogru karsilastirma */
        {
            printf("x on            : evet\n");
        }
        else
        {
            printf("x on            : hayir\n");
        }
    }

    /* 2) Off-by-one — sinir hatasi */
    {
        uint8_t dizi[5U] = {1U, 2U, 3U, 4U, 5U};
        uint8_t i;
        /* for (i=0; i<=5; i++) — YANLIS: dizi[5] gecersiz erisim */
        for (i = 0U; i < 5U; i++) /* dogru: < kullan, <= degil */
        {
            printf("%u ", (uint32_t)dizi[i]);
        }
        printf("\n");
    }

    /* 3) Sonsuz dongu — timeout olmadan */
    {
        uint8_t timeout = 100U;
        uint8_t flag    = 0U;

        /* while(flag == 0U) { } — YANLIS: flag hic degismeyebilir */
        while ((flag == 0U) && (timeout > 0U)) /* timeout korumal */
        {
            timeout--;
            if (timeout == 50U)
            {
                flag = 1U; /* simule edilmis flag set */
            }
        }
        printf("Dongu cikis       : timeout=%u\n", (uint32_t)timeout);
    }

    /* 4) switch fall-through — kasitsiz */
    {
        uint8_t durum = 1U;
        switch (durum)
        {
            case 1U:
            {
                printf("Case 1            : islendi\n");
                break; /* break olmadan case 2 de calisirdi */
            }
            case 2U:
            {
                printf("Case 2            : islendi\n");
                break;
            }
            default:
            {
                break;
            }
        }
    }
}

/* MAIN */

int main(void)
{
    printf("/* IF / ELSE */\n");
    if_else();

    printf("\n/* SWITCH */\n");
    switch_yapi();

    printf("\n/* FOR DONGUSU */\n");
    for_dongusu();

    printf("\n/* WHILE DONGUSU */\n");
    while_dongusu();

    printf("\n/* DO-WHILE DONGUSU */\n");
    do_while_dongusu();

    printf("\n/* IC ICE DONGU */\n");
    ic_ice_dongu();

    printf("\n/* YAYGIN HATALAR */\n");
    yaygin_hatalar();

    return 0;
}