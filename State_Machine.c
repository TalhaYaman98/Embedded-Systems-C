#include <stdint.h>
#include <stdio.h>

/*
 * State Machine
 * MISRA-C:2012 Uygulamalari
 *
 * Kullanilan kurallar:
 *   Rule  7.2  — unsigned sabit literaller U suffix tasimali
 *   Rule  8.1  — tipler acikca belirtilmeli
 *   Rule  8.7  — sadece bir dosyadan erisilen nesne static olmali
 *   Rule 14.4  — if kosulu esansiyel boolean tipinde olmali
 *   Rule 15.2  — switch default etiketi olmali
 *   Rule 15.3  — switch default son etiket olmali
 *   Rule 15.4  — switch case fall-through olmamali
 *   Rule 15.5  — fonksiyonun tek return noktasi olmali
 *
 * Konu:
 *   Gercek bir gomulu sistem urununde state machine kac farkli sekilde
 *   yazilabilir? Bu dosyada uc yontem karsilastirilir:
 *     1) switch/case tabanli      — basit, okunabilir
 *     2) fonksiyon pointer tabanli — genisletilebilir, buyuk projelere uygun
 *     3) tablo tabanli            — gecis kurallari merkezi, bakim kolayligi
 */

/* PROTOTIP BILDIRIMLERI */

static void sm_switch_ornegi(void);
static void sm_fonksiyon_ptr_ornegi(void);
static void sm_tablo_ornegi(void);
static void yaygin_hatalar(void);

/* DURUM VE OLAY TANIMLARI */

/* Durum makinesi durumlari — her durum bir calisma modunu temsil eder    */
typedef enum {
    SM_BOSTA        = 0U, /* baslangic durumu — islem bekliyor            */
    SM_BASLATILIYOR = 1U, /* baslama sekansı calisıyor                    */
    SM_CALISIYOR    = 2U, /* normal operasyon                             */
    SM_HATA         = 3U, /* hata durumu — kurtarma bekleniyor            */
    SM_DURUYOR      = 4U, /* durma sekansı calisıyor                      */
    SM_DURUM_ADET   = 5U  /* toplam durum sayisi — dizi boyutu icin       */
} Durum_t;

/* Olaylar — durumu tetikleyen dis veya ic sinyaller                      */
typedef enum {
    OL_BASLAT       = 0U, /* kullanici baslat komutu                      */
    OL_HAZIR        = 1U, /* donanim hazir sinyali                        */
    OL_HATA         = 2U, /* hata tespit edildi                           */
    OL_SIFIRLA      = 3U, /* hata sonrasi sifirlama komutu                */
    OL_DURDUR       = 4U, /* kullanici durdur komutu                      */
    OL_TAMAMLANDI   = 5U, /* durma sekansı tamamlandi                     */
    OL_OLAY_ADET    = 6U  /* toplam olay sayisi — tablo boyutu icin       */
} Olay_t;

/* Durum makinesi baglam yapisi — tum SM degiskenlerini bir arada tutar   */
typedef struct {
    Durum_t  guncel;          /* mevcut aktif durum                       */
    Durum_t  onceki;          /* bir onceki durum — debug icin            */
    uint32_t durum_sure_ms;   /* mevcut durumda gecen sure                */
    uint32_t gecis_sayisi;    /* toplam durum gecisi sayisi               */
    uint8_t  hata_kodu;       /* son hata kodu                            */
} SmBaglam_t;

/* YONTEM 1 — SWITCH/CASE TABANLI */

/* En yaygin yontem — kucuk/orta boyutlu durum makineleri icin idealdir  */
/* Her durum bir case blogu, her olay icinde if/else ile gecis yapilir   */

static Durum_t sm_switch_gec(Durum_t guncel, Olay_t olay)
{
    Durum_t sonraki = guncel; /* varsayilan: ayni durumda kal             */

    switch (guncel)
    {
        case SM_BOSTA:
        {
            if (olay == OL_BASLAT) /* baslat komutu geldi                 */
            {
                sonraki = SM_BASLATILIYOR;
                printf("  Gecis: BOSTA -> BASLATILIYOR\n");
            }
            else
            {
                /* diger olaylari bu durumda yoksay                       */
            }
            break;
        }
        case SM_BASLATILIYOR:
        {
            if (olay == OL_HAZIR) /* donanim hazir                        */
            {
                sonraki = SM_CALISIYOR;
                printf("  Gecis: BASLATILIYOR -> CALISIYOR\n");
            }
            else if (olay == OL_HATA) /* baslama sirasinda hata           */
            {
                sonraki = SM_HATA;
                printf("  Gecis: BASLATILIYOR -> HATA\n");
            }
            else
            {
                /* bekleniyor */
            }
            break;
        }
        case SM_CALISIYOR:
        {
            if (olay == OL_HATA) /* calisma sirasinda hata                */
            {
                sonraki = SM_HATA;
                printf("  Gecis: CALISIYOR -> HATA\n");
            }
            else if (olay == OL_DURDUR) /* durdur komutu                  */
            {
                sonraki = SM_DURUYOR;
                printf("  Gecis: CALISIYOR -> DURUYOR\n");
            }
            else
            {
                /* normal operasyon devam                                 */
            }
            break;
        }
        case SM_HATA:
        {
            if (olay == OL_SIFIRLA) /* hata sonrasi sifirlama             */
            {
                sonraki = SM_BOSTA;
                printf("  Gecis: HATA -> BOSTA\n");
            }
            else
            {
                /* hata durumunda kal — sifirlama bekleniyor              */
            }
            break;
        }
        case SM_DURUYOR:
        {
            if (olay == OL_TAMAMLANDI) /* durma sekansı bitti             */
            {
                sonraki = SM_BOSTA;
                printf("  Gecis: DURUYOR -> BOSTA\n");
            }
            else
            {
                /* durma sekansı devam ediyor                             */
            }
            break;
        }
        default:
        {
            /* bilinmeyen durum — baslangica don                          */
            sonraki = SM_BOSTA;
            printf("  Gecis: BILINMEYEN -> BOSTA\n");
            break;
        }
    }

    return sonraki; /* tek return noktasi                                  */
}

static void sm_switch_ornegi(void)
{
    SmBaglam_t sm;
    sm.guncel        = SM_BOSTA;
    sm.onceki        = SM_BOSTA;
    sm.durum_sure_ms = 0U;
    sm.gecis_sayisi  = 0U;
    sm.hata_kodu     = 0U;

    /* Olay dizisi — gercek projede ISR, sensor veya komut uretir        */
    Olay_t olaylar[] = {
        OL_BASLAT,
        OL_HAZIR,
        OL_DURDUR,
        OL_TAMAMLANDI
    };
    uint8_t olay_adet = (uint8_t)(sizeof(olaylar) / sizeof(olaylar[0U]));
    uint8_t i;

    printf("Switch/case SM    :\n");
    for (i = 0U; i < olay_adet; i++)
    {
        sm.onceki  = sm.guncel;
        sm.guncel  = sm_switch_gec(sm.guncel, olaylar[i]);
        if (sm.guncel != sm.onceki) /* durum degisti                      */
        {
            sm.gecis_sayisi++;
        }
        else
        {
            /* ayni durumda kalindi */
        }
    }
    printf("  Gecis sayisi    : %u\n", (uint32_t)sm.gecis_sayisi); /* 4  */
}

/* YONTEM 2 — FONKSIYON POINTER TABANLI */

/* Her durum icin ayri isleyici fonksiyon — buyuk SM'lerde okunabilirlik  */
/* Yeni durum eklemek sadece yeni fonksiyon + pointer atamasi gerektirir  */

typedef Durum_t (*DurumIsleyici_t)(Olay_t olay); /* isleyici fonksiyon tipi */

/* Her durum icin isleyici fonksiyon */
static Durum_t isleyici_bosta(Olay_t olay)
{
    Durum_t sonraki = SM_BOSTA;

    if (olay == OL_BASLAT)
    {
        sonraki = SM_BASLATILIYOR;
        printf("  [BOSTA] baslat  -> BASLATILIYOR\n");
    }
    else
    {
        /* diger olaylar yoksayilir */
    }

    return sonraki;
}

static Durum_t isleyici_baslatiliyor(Olay_t olay)
{
    Durum_t sonraki = SM_BASLATILIYOR;

    if (olay == OL_HAZIR)
    {
        sonraki = SM_CALISIYOR;
        printf("  [BASLATILIYOR] hazir -> CALISIYOR\n");
    }
    else if (olay == OL_HATA)
    {
        sonraki = SM_HATA;
        printf("  [BASLATILIYOR] hata  -> HATA\n");
    }
    else
    {
        /* bekleniyor */
    }

    return sonraki;
}

static Durum_t isleyici_calisiyor(Olay_t olay)
{
    Durum_t sonraki = SM_CALISIYOR;

    if (olay == OL_HATA)
    {
        sonraki = SM_HATA;
        printf("  [CALISIYOR] hata   -> HATA\n");
    }
    else if (olay == OL_DURDUR)
    {
        sonraki = SM_DURUYOR;
        printf("  [CALISIYOR] durdur -> DURUYOR\n");
    }
    else
    {
        /* normal operasyon */
    }

    return sonraki;
}

static Durum_t isleyici_hata(Olay_t olay)
{
    Durum_t sonraki = SM_HATA;

    if (olay == OL_SIFIRLA)
    {
        sonraki = SM_BOSTA;
        printf("  [HATA] sifirla     -> BOSTA\n");
    }
    else
    {
        /* hata durumunda kal */
    }

    return sonraki;
}

static Durum_t isleyici_duruyor(Olay_t olay)
{
    Durum_t sonraki = SM_DURUYOR;

    if (olay == OL_TAMAMLANDI)
    {
        sonraki = SM_BOSTA;
        printf("  [DURUYOR] tamamlandi -> BOSTA\n");
    }
    else
    {
        /* durma sekansı devam */
    }

    return sonraki;
}

/* Durum isleyici tablosu — durum enum degeri indeks olarak kullanilir    */
/* Yeni durum eklemek: enum'a ekle + tabloya isleyici ekle               */
static const DurumIsleyici_t g_isleyiciler[SM_DURUM_ADET] = {
    isleyici_bosta,         /* SM_BOSTA        = 0 */
    isleyici_baslatiliyor,  /* SM_BASLATILIYOR = 1 */
    isleyici_calisiyor,     /* SM_CALISIYOR    = 2 */
    isleyici_hata,          /* SM_HATA         = 3 */
    isleyici_duruyor        /* SM_DURUYOR      = 4 */
};

static void sm_fonksiyon_ptr_ornegi(void)
{
    Durum_t guncel  = SM_BOSTA;
    Durum_t sonraki;

    /* Hata senaryosu — CALISIYOR'dan HATA'ya, sonra sifirlama           */
    Olay_t olaylar[] = {
        OL_BASLAT,
        OL_HAZIR,
        OL_HATA,
        OL_SIFIRLA
    };
    uint8_t olay_adet = (uint8_t)(sizeof(olaylar) / sizeof(olaylar[0U]));
    uint8_t i;

    printf("Fonksiyon ptr SM  :\n");
    for (i = 0U; i < olay_adet; i++)
    {
        if (guncel < SM_DURUM_ADET) /* gecersiz indeks koruması           */
        {
            sonraki = g_isleyiciler[guncel](olaylar[i]); /* isleyiciyi cagir */
            guncel  = sonraki;
        }
        else
        {
            guncel = SM_BOSTA; /* gecersiz durum — baslangica don         */
        }
    }
}

/* YONTEM 3 — TABLO TABANLI */

/* Tum gecis kurallari merkezi bir tabloda — en bakim dostu yontem        */
/* Hangi durumda hangi olay hangi duruma gecirir — tek bakista gorulur   */

typedef struct {
    Durum_t  kaynak;   /* mevcut durum                                    */
    Olay_t   olay;     /* tetikleyen olay                                 */
    Durum_t  hedef;    /* gecilecek yeni durum                            */
} GecisKurali_t;

/* Gecis tablosu — tum SM davranisi burada tanimli                        */
static const GecisKurali_t g_gecis_tablosu[] = {
    /* kaynak durum        olay            hedef durum        */
    { SM_BOSTA,        OL_BASLAT,      SM_BASLATILIYOR },
    { SM_BASLATILIYOR, OL_HAZIR,       SM_CALISIYOR    },
    { SM_BASLATILIYOR, OL_HATA,        SM_HATA         },
    { SM_CALISIYOR,    OL_HATA,        SM_HATA         },
    { SM_CALISIYOR,    OL_DURDUR,      SM_DURUYOR      },
    { SM_HATA,         OL_SIFIRLA,     SM_BOSTA        },
    { SM_DURUYOR,      OL_TAMAMLANDI,  SM_BOSTA        }
};

#define GECIS_ADET  ((uint8_t)(sizeof(g_gecis_tablosu) / sizeof(g_gecis_tablosu[0U])))

/* Tabloda eslesme ara — bulunamazsa guncel durumda kal                   */
static Durum_t tablo_gec(Durum_t guncel, Olay_t olay)
{
    Durum_t sonraki = guncel; /* varsayilan: ayni durumda kal             */
    uint8_t i;

    for (i = 0U; i < GECIS_ADET; i++)
    {
        /* kaynak durum ve olay eslesiyorsa gecisi uygula                 */
        if ((g_gecis_tablosu[i].kaynak == guncel) &&
            (g_gecis_tablosu[i].olay   == olay))
        {
            sonraki = g_gecis_tablosu[i].hedef;
            printf("  Tablo gecis     : %u -> %u (olay:%u)\n",
                   (uint32_t)guncel, (uint32_t)sonraki, (uint32_t)olay);
            break; /* eslesen kural bulundu, aramaya devam etme           */
        }
        else
        {
            /* bu kural eslesmedi, devam et                               */
        }
    }

    return sonraki; /* tek return noktasi                                  */
}

static void sm_tablo_ornegi(void)
{
    Durum_t guncel = SM_BOSTA;

    Olay_t olaylar[] = {
        OL_BASLAT,
        OL_HAZIR,
        OL_DURDUR,
        OL_TAMAMLANDI
    };
    uint8_t olay_adet = (uint8_t)(sizeof(olaylar) / sizeof(olaylar[0U]));
    uint8_t i;

    printf("Tablo tabanli SM  :\n");
    for (i = 0U; i < olay_adet; i++)
    {
        guncel = tablo_gec(guncel, olaylar[i]);
    }
    printf("  Son durum       : %u\n", (uint32_t)guncel); /* SM_BOSTA = 0 */
}

/* YAYGIN HATALAR */

static void yaygin_hatalar(void)
{
    /* 1) Gecersiz durum degeri — enum siniri disinda                     */
    {
        /* Durum_t d = (Durum_t)99U; — gecersiz, tablo erisiminde cokme  */
        /* Cozum: her gecisten once aralik kontrolu yap                   */
        Durum_t d = SM_CALISIYOR;
        if (d < SM_DURUM_ADET) /* gecerli aralik kontrolu                 */
        {
            printf("Durum gecerli     : %u\n", (uint32_t)d);
        }
        else
        {
            printf("Durum gecersiz    : baslangica don\n");
        }
    }

    /* 2) Default case eksikligi — bilinmeyen durumda tanimsiz davranis   */
    {
        /* switch(durum) { case A: ... case B: ... }                      */
        /* Yeni durum eklenir ama switch guncellenmezse sessiz hata       */
        /* Cozum: default case + assert veya hata logu                   */
        printf("Default case      : her switch'te olmali\n");
    }

    /* 3) Durum gecisi sirasinda yan etki — onceki islem tamamlanmadan   */
    {
        /* Gecis sirasinda donanim hazir olmayabilir                      */
        /* Cozum: entry/exit aksiyonlari ile gecis kontrollu yapilmali   */
        printf("Gecis yan etkisi  : entry/exit aksiyonu kullan\n");
    }

    /* 4) Cok fazla durum — okunabilirlik bozulur                        */
    {
        /* 10+ durum varsa hiyerarsik SM (HSM) dusun                     */
        /* Alt durumlar ust durumun davranisini miras alir                */
        printf("Cok durum         : 10+ ise hiyerarsik SM dusun\n");
    }
}

/* MAIN */

int main(void)
{
    printf("/* SWITCH/CASE TABANLI SM */\n");
    sm_switch_ornegi();

    printf("\n/* FONKSIYON POINTER TABANLI SM */\n");
    sm_fonksiyon_ptr_ornegi();

    printf("\n/* TABLO TABANLI SM */\n");
    sm_tablo_ornegi();

    printf("\n/* YAYGIN HATALAR */\n");
    yaygin_hatalar();

    return 0;
}