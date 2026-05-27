#include <stdint.h>
#include <stdio.h>

/*
 * Watchdog
 * MISRA-C:2012 Uygulamalari
 *
 * Kullanilan kurallar:
 *   Rule  7.2  — unsigned sabit literaller U suffix tasimali
 *   Rule  8.1  — tipler acikca belirtilmeli
 *   Rule  8.7  — sadece bir dosyadan erisilen nesne static olmali
 *   Rule 14.4  — if kosulu esansiyel boolean tipinde olmali
 *   Rule 15.5  — fonksiyonun tek return noktasi olmali
 *
 * Platform notu:
 *   STM32 spesifik registerlar ve HAL fonksiyonlari yorum icinde
 *   gosterilmistir. PC ortaminda simule edilmis karsılıkları calisir.
 *
 * Konu:
 *   STM32'de iki watchdog donanimi bulunur:
 *     1) IWDG (Independent Watchdog) — bagimsiz RC osilatoru ile calisir
 *        sistem saati dussa bile calisir, reset uretir
 *     2) WWDG (Window Watchdog) — APB saat kaynagi, pencere tabanli
 *        erken besleme de reset uretir — zamanlama hassasiyeti gerektirir
 */

/* PROTOTIP BILDIRIMLERI */

static void iwdg_ornegi(void);
static void wwdg_ornegi(void);
static void wdg_gorev_izleme(void);
static void wdg_reset_nedeni(void);
static void yaygin_hatalar(void);

/* SABIT TANIMLAR */

/* IWDG zaman asimi hesabi:
 * t = (4 * 2^prescaler * reload) / LSI_frekans
 * LSI = 32000 Hz (yaklasik — fabrika kalibrasyonu gerekebilir)
 * Prescaler = 64 → PR = 4
 * Reload    = 1000
 * t = (4 * 64 * 1000) / 32000 = 8 saniye                               */
#define IWDG_PRESCALER    (4U)    /* PR register degeri — 64x bolme       */
#define IWDG_RELOAD       (1000U) /* RLR register degeri — 0..4095 arasi  */
#define IWDG_TIMEOUT_MS   (8000U) /* hesaplanan zaman asimi — ms          */

/* WWDG zaman asimi hesabi:
 * t = (4096 * 2^wdgtb * (T[5:0]+1)) / PCLK1
 * PCLK1 = 42 MHz (STM32F407 varsayilan)
 * WDGTB = 3 → 8x bolme
 * T     = 0x3F (63)
 * t = (4096 * 8 * 64) / 42000000 ≈ 50ms                                */
#define WWDG_PRESCALER    (3U)    /* WDGTB — 8x bolme                    */
#define WWDG_WINDOW       (0x50U) /* W[6:0] — erken besleme siniri        */
#define WWDG_COUNTER      (0x7FU) /* T[6:0] — baslangic sayac degeri      */
#define WWDG_TIMEOUT_MS   (50U)   /* yaklasik zaman asimi                 */

/* Gorev izleme sabitleri */
#define GOREV_ADET        (4U)    /* izlenen gorev sayisi                 */
#define GOREV_TIMEOUT_MS  (100U)  /* her gorev icin maksimum sure         */

/* SIMULASYON DEGISKENLERI */

/* STM32'de bu degerler donanim registerlarindan okunur                   */
/* PC simulasyonu icin yazilim karsiliklari kullanilir                    */
static uint32_t g_sim_tick       = 0U;   /* simule edilmis sistem zamani  */
static uint8_t  g_iwdg_beslendi  = 0U;   /* IWDG besleme durumu           */
static uint8_t  g_wwdg_beslendi  = 0U;   /* WWDG besleme durumu           */
static uint8_t  g_reset_nedeni   = 0U;   /* son reset nedeni              */

/* Reset nedeni bitleri — STM32 RCC_CSR register bitleri ile uyumlu      */
#define RESET_IWDG_BIT    (0x20U) /* IWDGRSTF — IWDG reset bayragi       */
#define RESET_WWDG_BIT    (0x10U) /* WWDGRSTF — WWDG reset bayragi       */
#define RESET_POR_BIT     (0x08U) /* PORRSTF  — guc acilisi reset        */
#define RESET_PIN_BIT     (0x04U) /* PINRSTF  — NRST pin reset           */
#define RESET_SW_BIT      (0x02U) /* SFTRSTF  — yazilim reset            */

/* GOREV IZLEME YAPISI */

/* Her goreve ait izleme bilgisi — watchdog besleme kontrolu icin        */
typedef struct {
    uint8_t  aktif;          /* 1 = gorev izleniyor, 0 = pasif           */
    uint8_t  beslendi;       /* mevcut periyotta beslendi mi?             */
    uint32_t son_besleme_ms; /* son besleme zamani                        */
    uint32_t timeout_ms;     /* bu gorev icin maksimum sure               */
    uint8_t  isim[16U];      /* gorev adi — debug icin                   */
} GorevIzle_t;

static GorevIzle_t g_gorevler[GOREV_ADET] = {
    /* aktif  beslendi  son_besleme  timeout          isim              */
    {  1U,    0U,       0U,          GOREV_TIMEOUT_MS, "Ana_Dongu"      },
    {  1U,    0U,       0U,          GOREV_TIMEOUT_MS, "UART_Isleyici"  },
    {  1U,    0U,       0U,          GOREV_TIMEOUT_MS, "Sensor_Okuma"   },
    {  0U,    0U,       0U,          GOREV_TIMEOUT_MS, "Pasif_Gorev"    }
};

/* IWDG SIMULASYON FONKSİYONLARI */

/* IWDG baslat — gercek STM32 kodu yorum icinde                          */
static void iwdg_baslat(void)
{
    /* STM32 HAL kodu:                                                    */
    /* hiwdg.Instance       = IWDG;                                       */
    /* hiwdg.Init.Prescaler = IWDG_PRESCALER_64;                         */
    /* hiwdg.Init.Reload    = IWDG_RELOAD;                               */
    /* HAL_IWDG_Init(&hiwdg);                                            */
    /*                                                                    */
    /* Register seviyesi:                                                 */
    /* IWDG->KR  = 0x5555U; — yazma korumasini ac                       */
    /* IWDG->PR  = IWDG_PRESCALER;                                       */
    /* IWDG->RLR = IWDG_RELOAD;                                         */
    /* IWDG->KR  = 0xAAAAU; — reload — sayaci yenile                    */
    /* IWDG->KR  = 0xCCCCU; — baslat                                    */

    g_iwdg_beslendi = 0U;
    printf("IWDG baslat       : prescaler=%u reload=%u timeout=%ums\n",
           (uint32_t)IWDG_PRESCALER,
           (uint32_t)IWDG_RELOAD,
           (uint32_t)IWDG_TIMEOUT_MS);
}

/* IWDG besle (kick/refresh) — zaman asimi dolmadan cagirilmali          */
static void iwdg_besle(void)
{
    /* STM32 HAL kodu:                                                    */
    /* HAL_IWDG_Refresh(&hiwdg);                                         */
    /*                                                                    */
    /* Register seviyesi:                                                 */
    /* IWDG->KR = 0xAAAAU; — reload komutu — sayaci sifirla             */

    g_iwdg_beslendi = 1U;
    printf("IWDG beslendi     : tick=%u\n", (uint32_t)g_sim_tick);
}

/* WWDG SIMULASYON FONKSİYONLARI */

/* WWDG baslat — pencere tabanli, erken besleme de reset uretir          */
static void wwdg_baslat(void)
{
    /* STM32 HAL kodu:                                                    */
    /* hwwdg.Instance       = WWDG;                                       */
    /* hwwdg.Init.Prescaler = WWDG_PRESCALER_8;                          */
    /* hwwdg.Init.Window    = WWDG_WINDOW;                               */
    /* hwwdg.Init.Counter   = WWDG_COUNTER;                              */
    /* hwwdg.Init.EWIMode   = WWDG_EWI_ENABLE; — erken uyari kesmesi    */
    /* HAL_WWDG_Init(&hwwdg);                                            */
    /*                                                                    */
    /* Register seviyesi:                                                 */
    /* WWDG->CFR = (WWDG_PRESCALER << 7U) | WWDG_WINDOW;                */
    /* WWDG->CR  = WWDG_CR_WDGA | WWDG_COUNTER; — aktive et             */

    g_wwdg_beslendi = 0U;
    printf("WWDG baslat       : pencere=0x%02X sayac=0x%02X timeout=%ums\n",
           (uint32_t)WWDG_WINDOW,
           (uint32_t)WWDG_COUNTER,
           (uint32_t)WWDG_TIMEOUT_MS);
    printf("  Besleme araligi : sayac 0x%02X..0x%02X arasinda olmali\n",
           (uint32_t)WWDG_WINDOW,
           (uint32_t)WWDG_COUNTER);
}

/* WWDG besle — sadece sayac pencere degerinin altına dustugunde beslenmeli */
static uint8_t wwdg_besle(uint8_t guncel_sayac)
{
    uint8_t ret;

    /* Pencere kontrolu — erken besleme reset uretir                     */
    if (guncel_sayac > WWDG_WINDOW) /* sayac hala pencere ustunde        */
    {
        /* Bu noktada besleme yapilirsa WWDG reset atar — erken besleme  */
        printf("WWDG erken besleme: sayac=0x%02X > pencere=0x%02X RESET!\n",
               (uint32_t)guncel_sayac, (uint32_t)WWDG_WINDOW);
        ret = 0U; /* basarisiz — reset olurdu                            */
    }
    else /* sayac pencere icerisinde — besleme yapilabilir               */
    {
        /* STM32 HAL kodu:                                                */
        /* HAL_WWDG_Refresh(&hwwdg);                                     */
        /*                                                                */
        /* Register seviyesi:                                             */
        /* WWDG->CR = WWDG_CR_WDGA | WWDG_COUNTER; — sayaci yenile      */

        g_wwdg_beslendi = 1U;
        printf("WWDG beslendi     : sayac=0x%02X pencere=0x%02X\n",
               (uint32_t)guncel_sayac, (uint32_t)WWDG_WINDOW);
        ret = 1U; /* basarili */
    }

    return ret;
}

/* IWDG ORNEGI */

static void iwdg_ornegi(void)
{
    uint8_t i;

    iwdg_baslat();

    /* Ana dongu simülasyonu — her iterasyonda IWDG beslenmeli           */
    /* Gercek projede ana while(1) dongusu icinde besleme yapilir        */
    for (i = 0U; i < 3U; i++)
    {
        g_sim_tick += 1000U; /* 1 saniye ilerledi                         */

        /* IWDG zaman asimi dolmadan besle — 8 saniye asılmamalı         */
        iwdg_besle();

        printf("  Ana dongu %u     : tick=%u\n",
               (uint32_t)(i + 1U), (uint32_t)g_sim_tick);
    }

    /* Beslenmezse ne olur — simule et                                   */
    g_sim_tick += (uint32_t)IWDG_TIMEOUT_MS + 1U; /* zaman asimi asildi */
    if (g_iwdg_beslendi == 0U)
    {
        printf("IWDG zaman asimi  : RESET! tick=%u\n", (uint32_t)g_sim_tick);
        g_reset_nedeni |= RESET_IWDG_BIT; /* reset nedeni kaydet         */
    }
    else
    {
        g_iwdg_beslendi = 0U; /* bir sonraki periyot icin sifirla         */
    }
}

/* WWDG ORNEGI */

static void wwdg_ornegi(void)
{
    wwdg_baslat();

    /* Dogru zamanlama — sayac pencere altına dustukten sonra besle      */
    printf("Dogru zamanlama   :\n");
    (void)wwdg_besle(0x45U); /* 0x45 < 0x50 (WINDOW) — gecerli aralik   */

    /* Yanlis zamanlama — sayac hala pencere ustunde                     */
    printf("Yanlis zamanlama  :\n");
    (void)wwdg_besle(0x60U); /* 0x60 > 0x50 (WINDOW) — cok erken        */
}

/* GOREV IZLEME */

/* Her gorev calistiginda bu fonksiyonu cagir — beslendigini isaretler   */
static void gorev_beslendi_isaretله(uint8_t gorev_idx)
{
    if (gorev_idx < GOREV_ADET) /* gecersiz indeks koruması              */
    {
        g_gorevler[gorev_idx].beslendi       = 1U;
        g_gorevler[gorev_idx].son_besleme_ms = g_sim_tick;
    }
    else
    {
        /* gecersiz indeks — islem yapma                                  */
    }
}

/* Tum aktif gorevler beslendi mi kontrol et — hepsi beslendiyse IWDG besle */
static uint8_t tum_gorevler_beslendi_mi(void)
{
    uint8_t i;
    uint8_t hepsi = 1U; /* varsayilan: hepsi beslendi                    */

    for (i = 0U; i < GOREV_ADET; i++)
    {
        if ((g_gorevler[i].aktif == 1U) &&   /* aktif gorev              */
            (g_gorevler[i].beslendi == 0U))  /* beslenmedi               */
        {
            hepsi = 0U; /* en az bir aktif gorev beslenmedi               */
            printf("  Eksik gorev     : %s beslenmedi\n",
                   g_gorevler[i].isim);
        }
        else
        {
            /* bu gorev tamam */
        }
    }

    return hepsi;
}

/* Bir sonraki periyot icin tum besleme bayraklarını sifirla             */
static void gorev_bayraklari_sifirla(void)
{
    uint8_t i;
    for (i = 0U; i < GOREV_ADET; i++)
    {
        g_gorevler[i].beslendi = 0U; /* yeni periyot icin hazirla         */
    }
}

static void wdg_gorev_izleme(void)
{
    printf("Gorev izleme      :\n");

    /* Tum aktif gorevleri besle — gercekte her gorev kendi dongusu icinde */
    gorev_beslendi_isaretله(0U); /* Ana_Dongu      calisti               */
    gorev_beslendi_isaretله(1U); /* UART_Isleyici  calisti               */
    gorev_beslendi_isaretله(2U); /* Sensor_Okuma   calisti               */
    /* gorev 3 pasif — besleme gerekmez                                   */

    /* Periyot sonunda kontrol — tum aktif gorevler calisti mi?          */
    if (tum_gorevler_beslendi_mi() == 1U)
    {
        iwdg_besle(); /* tum gorevler saglikli — IWDG besle               */
        gorev_bayraklari_sifirla(); /* bir sonraki periyot icin hazirla   */
    }
    else
    {
        /* en az bir gorev calismadigı — IWDG besleme yapilmaz           */
        /* IWDG zaman asimi dolunca sistem resetlenir                    */
        printf("  Gorev hatası    : IWDG beslenmedi — reset gelecek\n");
    }
}

/* RESET NEDENI TESPITI */

static void wdg_reset_nedeni(void)
{
    /* Sistem her basladiginda reset nedenini kontrol et                  */
    /* STM32 HAL kodu:                                                    */
    /* if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) — IWDG resetinden mi?  */
    /* if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST)) — WWDG resetinden mi?  */
    /* __HAL_RCC_CLEAR_RESET_FLAGS(); — bayraklari temizle               */

    /* Simule edilmis reset nedeni kontrolu */
    g_reset_nedeni = RESET_IWDG_BIT; /* onceki IWDG resetini simule et   */

    printf("Reset nedeni      :\n");

    if ((g_reset_nedeni & RESET_IWDG_BIT) != 0U)
    {
        printf("  IWDG reset      : watchdog beslenemedi — dongu takili?\n");
        /* Hata loguna yaz, kalici bellegde sakla (EEPROM/Flash)         */
    }
    else
    {
        /* IWDG reseti degil */
    }

    if ((g_reset_nedeni & RESET_WWDG_BIT) != 0U)
    {
        printf("  WWDG reset      : pencere ihlali — zamanlama sorunu?\n");
    }
    else
    {
        /* WWDG reseti degil */
    }

    if ((g_reset_nedeni & RESET_POR_BIT) != 0U)
    {
        printf("  Guc acilisi     : normal baslama\n");
    }
    else
    {
        /* POR reseti degil */
    }

    if ((g_reset_nedeni & RESET_SW_BIT) != 0U)
    {
        printf("  Yazilim reset   : HAL_NVIC_SystemReset() cagirildi\n");
    }
    else
    {
        /* SW reseti degil */
    }
}

/* YAYGIN HATALAR */

static void yaygin_hatalar(void)
{
    /* 1) Watchdog'u sadece belirli yerde beslemek                       */
    {
        /* Yanlis: sadece main() basinda beslemek — alt fonksiyon        */
        /* takılırsa watchdog beslenemez ama main() hala donuyor gibi    */
        /* gorunebilir                                                    */
        /* Cozum: gorev izleme paterni — her gorev kendi bayrağını       */
        /* setler, hepsi setlenince watchdog beslenir                    */
        printf("Tek nokta besleme : gorev izleme paterni kullan\n");
    }

    /* 2) WWDG'yi cok erken beslemek — reset uretir                      */
    {
        /* WWDG sayac pencere degerinin ustundeyken besleme yapilirsa    */
        /* donanim bunu protokol ihlali sayar ve reset atar              */
        /* Cozum: sayac degerini oku, pencere altına dustukten sonra    */
        /* besle                                                          */
        printf("WWDG erken besleme: sayac pencere altına dusunce besle\n");
    }

    /* 3) Kesme servisinde watchdog beslemek                             */
    {
        /* ISR'den watchdog beslemek ana dongunun takili olup olmadığini */
        /* maskeleyebilir — ISR calisıyor ama main() dongu donmuş olabilir */
        /* Cozum: watchdog sadece main() dongununden veya gorev izleme   */
        /* mekanizmasindan beslenmeli                                    */
        printf("ISR'den besleme   : ana dongudan besleme yapmali\n");
    }

    /* 4) Debug modunda watchdog — duraklatma                            */
    {
        /* Debugger ile adim adim ilerleme sirasinda watchdog resetler   */
        /* STM32CubeIDE'de: Debug Configuration → Peripheral → IWDG/WWDG */
        /* Freeze when core is halted secenegini etkinlestir             */
        /* DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;             */
        printf("Debug modu        : DBGMCU freeze secenegini ac\n");
    }
}

/* MAIN */

int main(void)
{
    printf("/* IWDG ORNEGI */\n");
    iwdg_ornegi();

    printf("\n/* WWDG ORNEGI */\n");
    wwdg_ornegi();

    printf("\n/* GOREV IZLEME */\n");
    wdg_gorev_izleme();

    printf("\n/* RESET NEDENI */\n");
    wdg_reset_nedeni();

    printf("\n/* YAYGIN HATALAR */\n");
    yaygin_hatalar();

    return 0;
}